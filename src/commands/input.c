#include "input.h"
#include "cmd_utils.h"
#include "../input.h"
#include "../page.h"
#include "../humanize.h"

static char *handle_click_action(BrowserState *state, int argc, char **parts,
                                  void (*click_fn)(WebKitWebView*, int, int)) {
    char *first = parts[1];
    if (argc >= 3 && cmd_is_coord(first)) {
        int x = atoi(parts[1]);
        int y = atoi(parts[2]);
        click_fn(state->web_view, x, y);
        return cmd_json_ok();
    } else {
        int cx, cy;
        char *rect_json = page_find_element(state->web_view, first);
        if (cmd_parse_element_center(rect_json, &cx, &cy)) {
            click_fn(state->web_view, cx, cy);
            return cmd_json_ok();
        }
        return cmd_json_error("element_not_found");
    }
}

char *cmd_input(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "click") == 0 && argc >= 2) {
        return handle_click_action(state, argc, parts, humanize_click);
    }
    if (strcmp(cmd, "doubleclick") == 0 && argc >= 2) {
        return handle_click_action(state, argc, parts, humanize_double_click);
    }
    if (strcmp(cmd, "rightclick") == 0 && argc >= 2) {
        return handle_click_action(state, argc, parts, humanize_right_click);
    }
    if (strcmp(cmd, "mousedown") == 0 && argc >= 3) {
        int x = atoi(parts[1]);
        int y = atoi(parts[2]);
        input_mouse_down(state->web_view, x, y);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "mouseup") == 0 && argc >= 3) {
        int x = atoi(parts[1]);
        int y = atoi(parts[2]);
        input_mouse_up(state->web_view, x, y);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "type") == 0 && argc >= 2) {
        char *text = g_strjoinv(" ", parts + 1);
        humanize_type_text(state->web_view, text);
        g_free(text);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "key") == 0 && argc >= 2) {
        input_key_press(state->web_view, parts[1]);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "typeinto") == 0 && argc >= 3) {
        char *text = g_strjoinv(" ", parts + 2);
        input_type_into(state->web_view, parts[1], text);
        g_free(text);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "hover") == 0 && argc >= 2) {
        char *first = parts[1];
        if (argc >= 3 && cmd_is_coord(first)) {
            int x = atoi(parts[1]);
            int y = atoi(parts[2]);
            input_hover(state->web_view, x, y);
            return cmd_json_ok();
        } else {
            int cx, cy;
            char *rect_json = page_find_element(state->web_view, first);
            if (cmd_parse_element_center(rect_json, &cx, &cy)) {
                input_hover(state->web_view, cx, cy);
                return cmd_json_ok();
            }
            return cmd_json_error("element_not_found");
        }
    }
    if (strcmp(cmd, "drag") == 0 && argc >= 5) {
        int sx = atoi(parts[1]), sy = atoi(parts[2]);
        int ex = atoi(parts[3]), ey = atoi(parts[4]);
        page_drag(state->web_view, sx, sy, ex, ey);
        return cmd_json_ok();
    }
    return NULL;
}
