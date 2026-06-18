#include "navigation.h"
#include "cmd_utils.h"
#include "../page.h"

char *cmd_navigation(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "goto") == 0 && argc >= 2) {
        char *url = g_strjoinv(" ", parts + 1);
        browser_goto(state, url);
        g_free(url);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "url") == 0) {
        char *url = browser_get_url(state);
        char *result = cmd_json_result("url", url);
        g_free(url);
        return result;
    }
    if (strcmp(cmd, "title") == 0) {
        char *title = browser_get_title(state);
        char *result = cmd_json_result("title", title);
        g_free(title);
        return result;
    }
    if (strcmp(cmd, "back") == 0) {
        page_go_back(state->web_view);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "forward") == 0) {
        page_go_forward(state->web_view);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "history") == 0) {
        int len = page_get_history_length(state->web_view);
        int idx = page_get_history_index(state->web_view);
        JsonBuilder *b = json_builder_new();
        json_builder_begin_object(b);
        json_builder_set_member_name(b, "length");
        json_builder_add_int_value(b, len);
        json_builder_set_member_name(b, "index");
        json_builder_add_int_value(b, idx);
        json_builder_end_object(b);
        JsonGenerator *gen = json_generator_new();
        json_generator_set_root(gen, json_builder_get_root(b));
        char *result = json_generator_to_data(gen, NULL);
        g_object_unref(gen);
        g_object_unref(b);
        return result;
    }
    if (strcmp(cmd, "history-goto") == 0 && argc >= 2) {
        page_goto_history(state->web_view, atoi(parts[1]));
        return cmd_json_ok();
    }
    if (strcmp(cmd, "close") == 0) {
        g_timeout_add(100, (GSourceFunc)gtk_main_quit, NULL);
        return cmd_json_ok();
    }
    return NULL;
}
