#include "extensions.h"
#include "command.h"
#include "input.h"
#include "page.h"
#include "screenshot.h"
#include "humanize.h"
#include "video.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <json-glib/json-glib.h>

static char *json_result(const char *key, const char *value) {
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, key);
    json_builder_add_string_value(builder, value ? value : "");
    json_builder_end_object(builder);

    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, json_builder_get_root(builder));
    char *json = json_generator_to_data(gen, NULL);

    g_object_unref(gen);
    g_object_unref(builder);
    return json;
}

static char *json_ok(void) {
    return json_result("ok", "true");
}

static char *json_error(const char *msg) {
    return json_result("error", msg);
}

// Clean eval output — remove outer quotes if present
static char *clean_eval_result(char *raw) {
    if (!raw) return NULL;
    // If result is wrapped in extra quotes from JSON string, unwrap
    int len = strlen(raw);
    if (len >= 2 && raw[0] == '"' && raw[len-1] == '"') {
        char *inner = g_strndup(raw + 1, len - 2);
        // Unescape common JSON escapes
        g_strstrip(inner);
        g_free(raw);
        return inner;
    }
    return raw;
}

char *command_process(BrowserState *state, const char *line) {
    if (!line || !*line) return NULL;

    char *clean = g_strdup(line);
    g_strstrip(clean);

    if (strlen(clean) == 0) {
        g_free(clean);
        return NULL;
    }

    char **parts = g_strsplit(clean, " ", -1);
    int argc = g_strv_length(parts);

    if (argc == 0) {
        g_strfreev(parts);
        g_free(clean);
        return NULL;
    }

    const char *cmd = parts[0];
    char *result = NULL;

    // Handle --tab flag: switch to specified tab, execute, switch back
    int target_tab = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(parts[i], "--tab") == 0 && i + 1 < argc) {
            target_tab = atoi(parts[i + 1]);
            break;
        }
    }
    int saved_tab = -1;
    if (target_tab >= 0 && target_tab != state->active_tab) {
        saved_tab = state->active_tab;
        browser_switch_tab(state, target_tab);
    }

    // === Navigation ===
    if (strcmp(cmd, "goto") == 0 && argc >= 2) {
        char *url = g_strjoinv(" ", parts + 1);
        browser_goto(state, url);
        g_free(url);
        result = json_ok();
    }
    else if (strcmp(cmd, "url") == 0) {
        char *url = browser_get_url(state);
        result = json_result("url", url);
        g_free(url);
    }
    else if (strcmp(cmd, "title") == 0) {
        char *title = browser_get_title(state);
        result = json_result("title", title);
        g_free(title);
    }
    // === Input ===
    else if (strcmp(cmd, "click") == 0 && argc >= 2) {
        char *first = parts[1];
        if (argc >= 3 && ((first[0] >= '0' && first[0] <= '9') || first[0] == '-')) {
            int x = atoi(parts[1]);
            int y = atoi(parts[2]);
            humanize_click(state->web_view, x, y);
            result = json_ok();
        } else {
            char *rect_json = page_find_element(state->web_view, first);
            if (rect_json && strstr(rect_json, "\"error\"")) {
                result = json_result("error", "element_not_found");
            } else if (rect_json) {
                JsonParser *parser = json_parser_new();
                if (json_parser_load_from_data(parser, rect_json, -1, NULL)) {
                    JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
                    int cx = (int)json_object_get_int_member(obj, "x");
                    int cy = (int)json_object_get_int_member(obj, "y");
                    humanize_click(state->web_view, cx, cy);
                    result = json_ok();
                } else {
                    result = json_error("parse_error");
                }
                g_object_unref(parser);
            } else {
                result = json_error("element_not_found");
            }
            g_free(rect_json);
        }
    }
    else if (strcmp(cmd, "doubleclick") == 0 && argc >= 2) {
        char *first = parts[1];
        if (argc >= 3 && ((first[0] >= '0' && first[0] <= '9') || first[0] == '-')) {
            int x = atoi(parts[1]);
            int y = atoi(parts[2]);
            humanize_double_click(state->web_view, x, y);
            result = json_ok();
        } else {
            char *rect_json = page_find_element(state->web_view, first);
            if (rect_json && strstr(rect_json, "\"error\"")) {
                result = json_result("error", "element_not_found");
            } else if (rect_json) {
                JsonParser *parser = json_parser_new();
                if (json_parser_load_from_data(parser, rect_json, -1, NULL)) {
                    JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
                    int cx = (int)json_object_get_int_member(obj, "x");
                    int cy = (int)json_object_get_int_member(obj, "y");
                    humanize_double_click(state->web_view, cx, cy);
                    result = json_ok();
                } else {
                    result = json_error("parse_error");
                }
                g_object_unref(parser);
            } else {
                result = json_error("element_not_found");
            }
            g_free(rect_json);
        }
    }
    else if (strcmp(cmd, "rightclick") == 0 && argc >= 2) {
        char *first = parts[1];
        if (argc >= 3 && ((first[0] >= '0' && first[0] <= '9') || first[0] == '-')) {
            int x = atoi(parts[1]);
            int y = atoi(parts[2]);
            humanize_right_click(state->web_view, x, y);
            result = json_ok();
        } else {
            char *rect_json = page_find_element(state->web_view, first);
            if (rect_json && strstr(rect_json, "\"error\"")) {
                result = json_result("error", "element_not_found");
            } else if (rect_json) {
                JsonParser *parser = json_parser_new();
                if (json_parser_load_from_data(parser, rect_json, -1, NULL)) {
                    JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
                    int cx = (int)json_object_get_int_member(obj, "x");
                    int cy = (int)json_object_get_int_member(obj, "y");
                    humanize_right_click(state->web_view, cx, cy);
                    result = json_ok();
                } else {
                    result = json_error("parse_error");
                }
                g_object_unref(parser);
            } else {
                result = json_error("element_not_found");
            }
            g_free(rect_json);
        }
    }
    else if (strcmp(cmd, "mousedown") == 0 && argc >= 3) {
        int x = atoi(parts[1]);
        int y = atoi(parts[2]);
        input_mouse_down(state->web_view, x, y);
        result = json_ok();
    }
    else if (strcmp(cmd, "mouseup") == 0 && argc >= 3) {
        int x = atoi(parts[1]);
        int y = atoi(parts[2]);
        input_mouse_up(state->web_view, x, y);
        result = json_ok();
    }
    else if (strcmp(cmd, "type") == 0 && argc >= 2) {
        char *text = g_strjoinv(" ", parts + 1);
        humanize_type_text(state->web_view, text);
        g_free(text);
        result = json_ok();
    }
    else if (strcmp(cmd, "key") == 0 && argc >= 2) {
        input_key_press(state->web_view, parts[1]);
        result = json_ok();
    }
    else if (strcmp(cmd, "typeinto") == 0 && argc >= 3) {
        char *text = g_strjoinv(" ", parts + 2);
        input_type_into(state->web_view, parts[1], text);
        g_free(text);
        result = json_ok();
    }
    else if (strcmp(cmd, "humanize") == 0 && argc >= 2) {
        int level = atoi(parts[1]);
        humanize_set_level(level);
        HumanizeConfig *c = humanize_get_config();
        JsonBuilder *b = json_builder_new();
        json_builder_begin_object(b);
        json_builder_set_member_name(b, "level");
        json_builder_add_int_value(b, c->level);
        json_builder_set_member_name(b, "mouse_movement");
        json_builder_add_boolean_value(b, c->mouse_movement);
        json_builder_set_member_name(b, "typing_jitter");
        json_builder_add_boolean_value(b, c->typing_jitter);
        json_builder_end_object(b);
        JsonGenerator *gen = json_generator_new();
        json_generator_set_root(gen, json_builder_get_root(b));
        result = json_generator_to_data(gen, NULL);
        g_object_unref(gen);
        g_object_unref(b);
    }
    // === Hover ===
    else if (strcmp(cmd, "hover") == 0 && argc >= 2) {
        char *first = parts[1];
        if (argc >= 3 && ((first[0] >= '0' && first[0] <= '9') || first[0] == '-')) {
            int x = atoi(parts[1]);
            int y = atoi(parts[2]);
            input_hover(state->web_view, x, y);
            result = json_ok();
        } else {
            char *rect_json = page_find_element(state->web_view, first);
            if (rect_json && strstr(rect_json, "\"error\"")) {
                result = json_result("error", "element_not_found");
            } else if (rect_json) {
                JsonParser *parser = json_parser_new();
                if (json_parser_load_from_data(parser, rect_json, -1, NULL)) {
                    JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
                    int cx = (int)json_object_get_int_member(obj, "x");
                    int cy = (int)json_object_get_int_member(obj, "y");
                    input_hover(state->web_view, cx, cy);
                    result = json_ok();
                } else {
                    result = json_error("parse_error");
                }
                g_object_unref(parser);
            } else {
                result = json_error("element_not_found");
            }
            g_free(rect_json);
        }
    }
    // === Scroll ===
    else if (strcmp(cmd, "scroll") == 0 && argc >= 3) {
        int dx = atoi(parts[1]);
        int dy = atoi(parts[2]);
        input_scroll(state->web_view, dx, dy);
        result = json_ok();
    }
    else if (strcmp(cmd, "scrollto") == 0 && argc >= 2) {
        input_scroll_to(state->web_view, parts[1]);
        result = json_ok();
    }
    // === Focus ===
    else if (strcmp(cmd, "focus") == 0 && argc >= 2) {
        input_focus(state->web_view, parts[1]);
        result = json_ok();
    }
    // === Find/Elements ===
    else if (strcmp(cmd, "find") == 0 && argc >= 2) {
        result = page_find_element(state->web_view, parts[1]);
    }
    else if (strcmp(cmd, "elements") == 0 || strcmp(cmd, "els") == 0) {
        result = page_get_elements(state->web_view);
    }
    else if (strcmp(cmd, "content") == 0) {
        bool outer = (argc >= 2 && strcmp(parts[1], "outer") == 0);
        result = page_get_content(state->web_view, outer);
    }
    else if (strcmp(cmd, "text") == 0) {
        result = page_get_text(state->web_view);
    }
    else if (strcmp(cmd, "eval") == 0 && argc >= 2) {
        char *script = g_strjoinv(" ", parts + 1);
        char *raw = page_eval_js(state->web_view, script);
        result = clean_eval_result(raw);
        g_free(script);
    }
    else if (strcmp(cmd, "read") == 0 && argc >= 2) {
        bool read_value = (argc >= 3 && strcmp(parts[2], "--value") == 0);
        result = page_read_element(state->web_view, parts[1], read_value);
    }
    else if (strcmp(cmd, "count") == 0 && argc >= 2) {
        result = page_count_elements(state->web_view, parts[1]);
    }
    else if (strcmp(cmd, "inspect") == 0) {
        result = page_inspect(state->web_view);
    }
    else if (strcmp(cmd, "a11y") == 0) {
        result = page_get_accessibility_tree(state->web_view);
    }
    // === Role selectors ===
    else if (strcmp(cmd, "role-click") == 0 && argc >= 2) {
        char *sel = g_strjoinv(" ", parts + 1);
        int ret = page_click_role(state->web_view, sel);
        g_free(sel);
        result = ret == 0 ? json_ok() : json_error("element_not_found");
    }
    else if (strcmp(cmd, "role-type") == 0 && argc >= 3) {
        char *text = g_strjoinv(" ", parts + 2);
        int ret = page_type_role(state->web_view, parts[1], text);
        g_free(text);
        result = ret == 0 ? json_ok() : json_error("element_not_found");
    }
    else if (strcmp(cmd, "role-find") == 0 && argc >= 2) {
        char *sel = g_strjoinv(" ", parts + 1);
        result = page_find_role_element(state->web_view, sel);
        g_free(sel);
    }
    // === Frames ===
    else if (strcmp(cmd, "frames") == 0) {
        result = page_get_frames(state->web_view);
    }
    // === Screenshot (sync for socket mode) ===
    else if (strcmp(cmd, "screenshot") == 0 && argc >= 2) {
        // Take screenshot synchronously
        if (strcmp(parts[1], "fullpage") == 0 && argc >= 3) {
            bool ss1 = screenshot_sync(state->web_view, parts[2]);
            result = ss1 ? json_ok() : json_error("screenshot_failed");
        } else if (strcmp(parts[1], "viewport") == 0 && argc >= 3) {
            bool ss2 = screenshot_sync(state->web_view, parts[2]);
            result = ss2 ? json_ok() : json_error("screenshot_failed");
        } else if (strcmp(parts[1], "element") == 0 && argc >= 4) {
            int ret = screenshot_schedule_element(state->web_view, parts[2], parts[3]);
            result = result ? result : json_result("status", "scheduled");
        } else {
            bool ss3 = screenshot_sync(state->web_view, parts[1]);
            result = ss3 ? json_ok() : json_error("screenshot_failed");
        }
    }
    // === Tabs ===
    else if (strcmp(cmd, "tabs") == 0) {
        int count = browser_tab_count(state);
        char *url = browser_get_url(state);
        char *title = browser_get_title(state);
        JsonBuilder *b = json_builder_new();
        json_builder_begin_object(b);
        json_builder_set_member_name(b, "count");
        json_builder_add_int_value(b, count);
        json_builder_set_member_name(b, "active");
        json_builder_add_int_value(b, state->active_tab);
        json_builder_set_member_name(b, "url");
        json_builder_add_string_value(b, url ? url : "");
        json_builder_set_member_name(b, "title");
        json_builder_add_string_value(b, title ? title : "");
        json_builder_end_object(b);
        JsonGenerator *gen = json_generator_new();
        json_generator_set_root(gen, json_builder_get_root(b));
        result = json_generator_to_data(gen, NULL);
        g_object_unref(gen);
        g_object_unref(b);
        g_free(url);
        g_free(title);
    }
    else if (strcmp(cmd, "tab") == 0 && argc >= 2) {
        int idx = atoi(parts[1]);
        browser_switch_tab(state, idx);
        result = json_ok();
    }
    else if (strcmp(cmd, "newtab") == 0) {
        const char *url = argc >= 2 ? parts[1] : "about:blank";
        browser_add_tab(state, url);
        result = json_ok();
    }
    else if (strcmp(cmd, "closetab") == 0 && argc >= 2) {
        int idx = atoi(parts[1]);
        browser_close_tab(state, idx);
        result = json_ok();
    }
    // === Window ===
    else if (strcmp(cmd, "resize") == 0 && argc >= 3) {
        int w = atoi(parts[1]);
        int h = atoi(parts[2]);
        browser_set_size(state, w, h);
        result = json_ok();
    }
    else if (strcmp(cmd, "viewport") == 0 && argc >= 3) {
        int w = atoi(parts[1]);
        int h = atoi(parts[2]);
        browser_set_size(state, w, h);
        page_set_viewport(state->web_view, w, h);
        result = json_ok();
    }
    else if (strcmp(cmd, "maximize") == 0) {
        browser_maximize(state);
        result = json_ok();
    }
    else if (strcmp(cmd, "minimize") == 0) {
        browser_minimize(state);
        result = json_ok();
    }
    else if (strcmp(cmd, "fullscreen") == 0) {
        browser_fullscreen(state);
        result = json_ok();
    }
    else if (strcmp(cmd, "unfullscreen") == 0) {
        browser_unfullscreen(state);
        result = json_ok();
    }
    else if (strcmp(cmd, "center") == 0) {
        browser_center(state);
        result = json_ok();
    }
    // === Wait ===
    else if (strcmp(cmd, "waitfor") == 0 && argc >= 3) {
        int timeout = atoi(parts[2]);
        int ret = page_wait_for(state->web_view, parts[1], timeout);
        result = json_result("status", ret == 0 ? "found" : "timeout");
    }
    else if (strcmp(cmd, "waitload") == 0 && argc >= 2) {
        int timeout = atoi(parts[1]);
        int ret = page_wait_for_load(state->web_view, timeout);
        result = json_result("status", ret == 0 ? "loaded" : "failed");
    }
    else if (strcmp(cmd, "wait") == 0 && argc >= 2) {
        int timeout = 10000;
        bool disappear = false;
        char *target = parts[1];
        for (int i = 2; i < argc; i++) {
            if (strcmp(parts[i], "--disappear") == 0) disappear = true;
            else if (strcmp(parts[i], "--text") == 0 && i + 1 < argc) target = parts[++i];
            else if (strcmp(parts[i], "--url-contains") == 0 && i + 1 < argc) {
                int ret = page_wait_for_url(state->web_view, parts[++i], timeout);
                result = json_result("status", ret == 0 ? "found" : "timeout");
                goto done_wait;
            }
            else if (strcmp(parts[i], "--state") == 0 && i + 1 < argc) {
                int ret = page_wait_for_state(state->web_view, parts[i-1], parts[++i], timeout);
                result = json_result("status", ret == 0 ? "found" : "timeout");
                goto done_wait;
            }
        }
        { int ret = page_wait_for_text(state->web_view, target, disappear, timeout);
          result = json_result("status", ret == 0 ? "found" : "timeout"); }
        done_wait: ;
    }
    // === Click-and-wait / Submit-and-wait ===
    else if (strcmp(cmd, "click-and-wait") == 0 && argc >= 2) {
        char *sel = parts[1];
        char *wait_target = NULL;
        int wait_type = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(parts[i], "--url-contains") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 0;
            } else if (strcmp(parts[i], "--text") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 1;
            } else if (strcmp(parts[i], "--title-contains") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 2;
            }
        }
        char *rect_json = page_find_element(state->web_view, sel);
        if (rect_json && !strstr(rect_json, "\"error\"")) {
            JsonParser *p = json_parser_new();
            if (json_parser_load_from_data(p, rect_json, -1, NULL)) {
                JsonObject *o = json_node_get_object(json_parser_get_root(p));
                int cx = (int)json_object_get_int_member(o, "x");
                int cy = (int)json_object_get_int_member(o, "y");
                humanize_click(state->web_view, cx, cy);
            }
            g_object_unref(p);
        }
        g_free(rect_json);
        if (wait_target) {
            int ret = -1;
            if (wait_type == 0) ret = page_wait_for_url(state->web_view, wait_target, 10000);
            else if (wait_type == 1) ret = page_wait_for_text(state->web_view, wait_target, false, 10000);
            result = json_result("status", ret == 0 ? "done" : "timeout");
        } else {
            result = json_ok();
        }
    }
    else if (strcmp(cmd, "submit-and-wait") == 0 && argc >= 2) {
        char *sel = parts[1];
        char *wait_target = NULL;
        int wait_type = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(parts[i], "--url-contains") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 0;
            } else if (strcmp(parts[i], "--text") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 1;
            } else if (strcmp(parts[i], "--title-contains") == 0 && i + 1 < argc) {
                wait_target = parts[++i]; wait_type = 2;
            }
        }
        char *rect_json = page_find_element(state->web_view, sel);
        if (rect_json && !strstr(rect_json, "\"error\"")) {
            JsonParser *p = json_parser_new();
            if (json_parser_load_from_data(p, rect_json, -1, NULL)) {
                JsonObject *o = json_node_get_object(json_parser_get_root(p));
                int cx = (int)json_object_get_int_member(o, "x");
                int cy = (int)json_object_get_int_member(o, "y");
                humanize_click(state->web_view, cx, cy);
            }
            g_object_unref(p);
        }
        g_free(rect_json);
        if (wait_target) {
            int ret = -1;
            if (wait_type == 0) ret = page_wait_for_url(state->web_view, wait_target, 10000);
            else if (wait_type == 1) ret = page_wait_for_text(state->web_view, wait_target, false, 10000);
            result = json_result("status", ret == 0 ? "done" : "timeout");
        } else {
            result = json_ok();
        }
    }
    // === Dialogs ===
    else if (strcmp(cmd, "dialogs") == 0) {
        result = page_get_dialogs(state->web_view);
    }
    else if (strcmp(cmd, "dialog-clear") == 0) {
        result = page_clear_dialogs(state->web_view);
    }
    else if (strcmp(cmd, "dialog-auto") == 0 && argc >= 2) {
        bool auto_accept = (strcmp(parts[1], "accept") == 0);
        const char *val = (argc >= 3) ? parts[2] : NULL;
        page_set_dialog_auto(state->web_view, auto_accept, val);
        result = json_ok();
    }
    else if (strcmp(cmd, "dialog") == 0 && argc >= 2) {
        const char *action = parts[1];
        const char *value = (argc >= 3) ? parts[2] : NULL;
        result = page_handle_dialog(state->web_view, action, value);
    }
    // === Navigation history ===
    else if (strcmp(cmd, "back") == 0) {
        page_go_back(state->web_view);
        result = json_ok();
    }
    else if (strcmp(cmd, "forward") == 0) {
        page_go_forward(state->web_view);
        result = json_ok();
    }
    else if (strcmp(cmd, "history") == 0) {
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
        result = json_generator_to_data(gen, NULL);
        g_object_unref(gen);
        g_object_unref(b);
    }
    else if (strcmp(cmd, "history-goto") == 0 && argc >= 2) {
        page_goto_history(state->web_view, atoi(parts[1]));
        result = json_ok();
    }
    // === Find in page ===
    else if (strcmp(cmd, "find-text") == 0 && argc >= 2) {
        bool highlight = (argc >= 3 && strcmp(parts[2], "--highlight") == 0);
        result = page_find_in_page(state->web_view, parts[1], highlight);
    }
    else if (strcmp(cmd, "find-count") == 0 && argc >= 2) {
        int count = page_count_matches(state->web_view, parts[1]);
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", count);
        result = json_result("matches", count_str);
    }
    // === Local/Session Storage ===
    else if (strcmp(cmd, "ls-get") == 0 && argc >= 2) {
        result = page_local_storage_get(state->web_view, parts[1]);
    }
    else if (strcmp(cmd, "ls-set") == 0 && argc >= 3) {
        char *val = g_strjoinv(" ", parts + 2);
        page_local_storage_set(state->web_view, parts[1], val);
        g_free(val);
        result = json_ok();
    }
    else if (strcmp(cmd, "ls-all") == 0) {
        result = page_local_storage_all(state->web_view);
    }
    else if (strcmp(cmd, "ss-get") == 0 && argc >= 2) {
        result = page_session_storage_get(state->web_view, parts[1]);
    }
    else if (strcmp(cmd, "ss-set") == 0 && argc >= 3) {
        char *val = g_strjoinv(" ", parts + 2);
        page_session_storage_set(state->web_view, parts[1], val);
        g_free(val);
        result = json_ok();
    }
    // === Checkbox/Radio ===
    else if (strcmp(cmd, "check") == 0 && argc >= 2) {
        page_check(state->web_view, parts[1]);
        result = json_ok();
    }
    else if (strcmp(cmd, "uncheck") == 0 && argc >= 2) {
        page_uncheck(state->web_view, parts[1]);
        result = json_ok();
    }
    else if (strcmp(cmd, "is-checked") == 0 && argc >= 2) {
        bool checked = page_is_checked(state->web_view, parts[1]);
        result = checked ? json_result("checked", "true") : json_result("checked", "false");
    }
    // === File upload ===
    else if (strcmp(cmd, "upload") == 0 && argc >= 3) {
        page_upload_file(state->web_view, parts[1], parts[2]);
        result = json_ok();
    }
    // === Bug 7: Dismiss overlays ===
    else if (strcmp(cmd, "dismiss") == 0) {
        result = page_dismiss_overlays(state->web_view);
    }
    // === Bug 3: Force elements ===
    else if (strcmp(cmd, "force-elements") == 0) {
        result = page_get_elements(state->web_view);
    }
    // === Clipboard ===
    else if (strcmp(cmd, "clipboard") == 0 && argc >= 2) {
        if (strcmp(parts[1], "read") == 0) {
            result = clipboard_read();
            if (!result) result = json_result("clipboard", "");
        } else if (strcmp(parts[1], "write") == 0 && argc >= 3) {
            char *text = g_strjoinv(" ", parts + 2);
            clipboard_write(text);
            g_free(text);
            result = json_ok();
        } else {
            result = json_error("usage: clipboard read|write <text>");
        }
    }
    // === Session recording ===
    else if (strcmp(cmd, "record") == 0) {
        page_start_recording(state->web_view);
        result = json_result("status", "recording_started");
    }
    else if (strcmp(cmd, "stop-recording") == 0) {
        result = page_stop_recording(state->web_view);
    }
    else if (strcmp(cmd, "get-recording") == 0) {
        result = page_get_recording(state->web_view);
    }
    // === Performance monitoring ===
    else if (strcmp(cmd, "perf-timing") == 0) {
        result = page_performance_timing(state->web_view);
    }
    else if (strcmp(cmd, "perf-memory") == 0) {
        result = page_performance_memory(state->web_view);
    }
    // === Accessibility audit ===
    else if (strcmp(cmd, "a11y-audit") == 0) {
        result = page_accessibility_audit(state->web_view);
    }
    // === Network logging ===
    // === Network logging ===
    else if (strcmp(cmd, "net-log") == 0) {
        page_start_network_log(state->web_view);
        result = json_result("status", "network_logging_started");
    }
    else if (strcmp(cmd, "net-stop") == 0) {
        page_stop_network_log(state->web_view);
        result = json_result("status", "network_logging_stopped");
    }
    else if (strcmp(cmd, "net-requests") == 0) {
        result = page_get_network_log(state->web_view);
    }
    // === Downloads ===
    else if (strcmp(cmd, "downloads") == 0) {
        result = page_get_downloads(state->web_view);
    }
    // === Drag and drop ===
    else if (strcmp(cmd, "drag") == 0 && argc >= 5) {
        int sx = atoi(parts[1]), sy = atoi(parts[2]);
        int ex = atoi(parts[3]), ey = atoi(parts[4]);
        page_drag(state->web_view, sx, sy, ex, ey);
        result = json_ok();
    }
    // === SSL info ===
    else if (strcmp(cmd, "ssl") == 0) {
        result = page_ssl_info(state->web_view);
    }
    // === PDF export ===
    else if (strcmp(cmd, "pdf") == 0 && argc >= 2) {
        bool ok = page_export_pdf(state->web_view, parts[1]);
        result = ok ? json_ok() : json_error("pdf_export_failed");
    }
    // === Screen recording ===
    else if (strcmp(cmd, "record-video") == 0 && argc >= 2) {
        if (strcmp(parts[1], "start") == 0 && argc >= 3) {
            int fps = (argc >= 4) ? atoi(parts[3]) : 15;
            bool ok = video_start_record(parts[2], fps);
            result = ok ? json_ok() : json_error("recording_failed");
        } else if (strcmp(parts[1], "stop") == 0) {
            bool ok = video_stop_record();
            result = ok ? json_ok() : json_error("no_recording_active");
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
            result = json_generator_to_data(gen, NULL);
            g_object_unref(gen);
            g_object_unref(b);
        } else {
            result = json_error("usage: record-video start <file> [fps]|stop|status");
        }
    }
    // === Close browser ===
    else if (strcmp(cmd, "close") == 0) {
        // Signal the browser to exit
        result = json_ok();
        // Queue exit after response is sent
        g_timeout_add(100, (GSourceFunc)gtk_main_quit, NULL);
    }
    // === Extensions ===
    else if (strcmp(cmd, "extension-load") == 0 && argc >= 2) {
        int count = extensions_load_dir(parts[1]);
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", count);
        result = json_result("loaded", count_str);
    }
    else if (strcmp(cmd, "extension-file") == 0 && argc >= 2) {
        int count = extensions_load_file(parts[1]);
        result = count > 0 ? json_ok() : json_error("failed_to_load");
    }
    else if (strcmp(cmd, "extension-list") == 0) {
        result = extensions_list_json();
    }
    else if (strcmp(cmd, "extension-unload") == 0) {
        extensions_unload_all();
        result = json_ok();
    }
    else if (strcmp(cmd, "extension-inject") == 0) {
        // Direct injection: read extension files and eval them
        const char *uri = browser_get_url(state);
        int injected = 0;
        // Read each loaded extension's code directly and eval it
        for (int ei = 0; ei < extensions_count(); ei++) {
            char **files = NULL;
            // Get the filepath for this extension
            // We'll read the extension code from the stored Extension struct
            // For now, just eval the code we have
        }
        // Simpler approach: re-read and eval all .js files from extensions dir
        DIR *dir = opendir("/home/gtkbrowser/extensions");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                char *dot = strrchr(entry->d_name, '.');
                if (!dot || (strcmp(dot, ".js") != 0 && strcmp(dot + 1, ".user.js") != 0))
                    continue;
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "/home/gtkbrowser/extensions/%s", entry->d_name);
                FILE *f = fopen(fullpath, "r");
                if (!f) continue;
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                char *code = g_malloc(fsize + 1);
                fread(code, 1, fsize, f);
                code[fsize] = '\0';
                fclose(f);
                // Strip GES/UserScript header
                char *code_start = strstr(code, "==/GES==");
                if (!code_start) code_start = strstr(code, "==/UserScript==");
                if (code_start) {
                    code_start += 9;
                    while (*code_start == '\n' || *code_start == '\r') code_start++;
                }
                // Eval it
                char *res = page_eval_js(state->web_view, code_start ? code_start : code);
                if (res) g_free(res);
                g_free(code);
                injected++;
            }
            closedir(dir);
        }
        if (uri) g_free((char*)uri);
        result = json_result("injected", injected > 0 ? "true" : "false");
    }
    else if (strcmp(cmd, "extension-count") == 0) {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", extensions_count());
        result = json_result("count", count_str);
    }
    // === Help ===
    else if (strcmp(cmd, "help") == 0) {
        result = json_result("help",
            "goto click doubleclick rightclick type key typeinto hover "
            "scroll scrollto focus find eval text content a11y elements "
            "screenshot url title tabs newtab tab closetab resize viewport "
            "waitfor waitload wait read count inspect "
            "role-click role-type role-find frames dialog dialogs "
            "submit-and-wait click-and-wait dialog-auto dialog-clear "
            "maximize minimize fullscreen center close "
            "back forward history history-goto "
            "find-text find-count ls-get ls-set ls-all ss-get ss-set "
            "clipboard pdf "
            "mousedown mouseup humanize help");
    }
    else {
        result = json_error("unknown_command");
    }

    // Switch back to original tab if --tab was used
    if (saved_tab >= 0) {
        browser_switch_tab(state, saved_tab);
    }

    g_strfreev(parts);
    g_free(clean);

    return result;
}

// === Persistent socket listener ===

static gboolean on_client_readable(GIOChannel *channel,
                                    GIOCondition condition,
                                    gpointer user_data) {
    BrowserState *state = (BrowserState *)user_data;

    if (condition & G_IO_HUP) {
        // Client disconnected — close this channel, stop watching
        g_io_channel_shutdown(channel, TRUE, NULL);
        g_io_channel_unref(channel);
        return FALSE;
    }

    gchar *line = NULL;
    gsize len = 0;
    GError *error = NULL;

    GIOStatus status = g_io_channel_read_line(channel, &line, &len,
                                               NULL, &error);

    if (status == G_IO_STATUS_NORMAL && line) {
        char *response = command_process(state, line);

        if (response) {
            gsize written = 0;
            g_io_channel_write_chars(channel, response, -1,
                                      &written, &error);
            g_io_channel_write_chars(channel, "\n", -1,
                                      &written, &error);
            g_io_channel_flush(channel, NULL);
            g_free(response);
        }
        g_free(line);
    } else if (status == G_IO_STATUS_EOF) {
        g_io_channel_shutdown(channel, TRUE, NULL);
        g_io_channel_unref(channel);
        return FALSE;
    }

    return TRUE;
}

// Callback to accept new connections
static gboolean on_server_readable(GIOChannel *channel,
                                    GIOCondition condition,
                                    gpointer user_data) {
    (void)condition;
    BrowserState *state = (BrowserState *)user_data;

    int client_fd = accept(g_io_channel_unix_get_fd(channel), NULL, NULL);
    if (client_fd < 0) return TRUE; // keep listening

    GIOChannel *client_channel = g_io_channel_unix_new(client_fd);
    g_io_channel_set_encoding(client_channel, NULL, NULL);
    g_io_channel_set_buffer_size(client_channel, 8192);

    // Watch this client — when it disconnects, server keeps listening
    g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP,
                   on_client_readable, state);

    fprintf(stderr, "AxonBrowser: client connected (fd %d)\n", client_fd);
    return TRUE; // keep listening for more clients
}

int command_init(BrowserState *state, const char *socket_path) {
    if (!socket_path) {
        // Use stdin mode
        GIOChannel *stdin_channel = g_io_channel_unix_new(STDIN_FILENO);
        g_io_channel_set_encoding(stdin_channel, NULL, NULL);
        state->socket_watch_id = g_io_add_watch(stdin_channel,
                                                 G_IO_IN | G_IO_HUP,
                                                 on_client_readable,
                                                 state);
        return 0;
    }

    // Create persistent Unix socket
    struct sockaddr_un addr;
    state->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (state->socket_fd < 0) return -1;

    unlink(socket_path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(state->socket_fd, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        close(state->socket_fd);
        return -1;
    }

    if (listen(state->socket_fd, 5) < 0) {
        close(state->socket_fd);
        return -1;
    }

    // Server channel — keeps listening for multiple clients
    GIOChannel *server_channel = g_io_channel_unix_new(state->socket_fd);
    g_io_channel_set_encoding(server_channel, NULL, NULL);

    state->socket_watch_id = g_io_add_watch(server_channel,
                                             G_IO_IN,
                                             on_server_readable,
                                             state);

    fprintf(stderr, "AxonBrowser: listening on %s\n", socket_path);
    return 0;
}

void command_cleanup(BrowserState *state) {
    if (state->socket_watch_id > 0) {
        g_source_remove(state->socket_watch_id);
        state->socket_watch_id = 0;
    }
    if (state->socket_fd >= 0) {
        close(state->socket_fd);
        state->socket_fd = -1;
    }
}
