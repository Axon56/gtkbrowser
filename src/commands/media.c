#include "media.h"
#include "cmd_utils.h"
#include "../page.h"
#include "../media/video.h"

char *cmd_media(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "clipboard") == 0 && argc >= 2) {
        if (strcmp(parts[1], "read") == 0) {
            char *result = clipboard_read();
            if (!result) result = cmd_json_result("clipboard", "");
            return result;
        } else if (strcmp(parts[1], "write") == 0 && argc >= 3) {
            char *text = g_strjoinv(" ", parts + 2);
            clipboard_write(text);
            g_free(text);
            return cmd_json_ok();
        } else {
            return cmd_json_error("usage: clipboard read|write <text>");
        }
    }
    if (strcmp(cmd, "record") == 0) {
        page_start_recording(state->web_view);
        return cmd_json_result("status", "recording_started");
    }
    if (strcmp(cmd, "stop-recording") == 0) {
        return page_stop_recording(state->web_view);
    }
    if (strcmp(cmd, "get-recording") == 0) {
        return page_get_recording(state->web_view);
    }
    if (strcmp(cmd, "record-video") == 0 && argc >= 2) {
        if (strcmp(parts[1], "start") == 0 && argc >= 3) {
            int fps = (argc >= 4) ? atoi(parts[3]) : 15;
            bool ok = video_start_record(parts[2], fps);
            return ok ? cmd_json_ok() : cmd_json_error("recording_failed");
        } else if (strcmp(parts[1], "stop") == 0) {
            bool ok = video_stop_record();
            return ok ? cmd_json_ok() : cmd_json_error("no_recording_active");
        } else if (strcmp(parts[1], "status") == 0) {
            bool active = video_is_recording();
            const char *path = video_get_output();
            JsonBuilder *b = json_builder_new();
            json_builder_begin_object(b);
            json_builder_set_member_name(b, "recording");
            json_builder_add_boolean_value(b, active);
            if (path) {
                json_builder_set_member_name(b, "output");
                json_builder_add_string_value(b, path);
            }
            json_builder_end_object(b);
            JsonGenerator *gen = json_generator_new();
            json_generator_set_root(gen, json_builder_get_root(b));
            char *result = json_generator_to_data(gen, NULL);
            g_object_unref(gen);
            g_object_unref(b);
            return result;
        } else {
            return cmd_json_error("usage: record-video start <file> [fps]|stop|status");
        }
    }
    if (strcmp(cmd, "check") == 0 && argc >= 2) {
        page_check(state->web_view, parts[1]);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "uncheck") == 0 && argc >= 2) {
        page_uncheck(state->web_view, parts[1]);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "is-checked") == 0 && argc >= 2) {
        bool checked = page_is_checked(state->web_view, parts[1]);
        return checked ? cmd_json_result("checked", "true") : cmd_json_result("checked", "false");
    }
    if (strcmp(cmd, "upload") == 0 && argc >= 3) {
        page_upload_file(state->web_view, parts[1], parts[2]);
        return cmd_json_ok();
    }
    return NULL;
}
