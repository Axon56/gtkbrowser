#include "elements.h"
#include "cmd_utils.h"
#include "../page.h"

char *cmd_elements(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "find") == 0 && argc >= 2) {
        return page_find_element(state->web_view, parts[1]);
    }
    if (strcmp(cmd, "elements") == 0 || strcmp(cmd, "els") == 0) {
        return page_get_elements(state->web_view);
    }
    if (strcmp(cmd, "read") == 0 && argc >= 2) {
        bool read_value = (argc >= 3 && strcmp(parts[2], "--value") == 0);
        return page_read_element(state->web_view, parts[1], read_value);
    }
    if (strcmp(cmd, "count") == 0 && argc >= 2) {
        return page_count_elements(state->web_view, parts[1]);
    }
    if (strcmp(cmd, "inspect") == 0) {
        return page_inspect(state->web_view);
    }
    if (strcmp(cmd, "a11y") == 0) {
        return page_get_accessibility_tree(state->web_view);
    }
    if (strcmp(cmd, "find-text") == 0 && argc >= 2) {
        bool highlight = (argc >= 3 && strcmp(parts[2], "--highlight") == 0);
        return page_find_in_page(state->web_view, parts[1], highlight);
    }
    if (strcmp(cmd, "find-count") == 0 && argc >= 2) {
        int count = page_count_matches(state->web_view, parts[1]);
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", count);
        return cmd_json_result("matches", count_str);
    }
    if (strcmp(cmd, "role-click") == 0 && argc >= 2) {
        char *sel = g_strjoinv(" ", parts + 1);
        int ret = page_click_role(state->web_view, sel);
        g_free(sel);
        return ret == 0 ? cmd_json_ok() : cmd_json_error("element_not_found");
    }
    if (strcmp(cmd, "role-type") == 0 && argc >= 3) {
        char *text = g_strjoinv(" ", parts + 2);
        int ret = page_type_role(state->web_view, parts[1], text);
        g_free(text);
        return ret == 0 ? cmd_json_ok() : cmd_json_error("element_not_found");
    }
    if (strcmp(cmd, "role-find") == 0 && argc >= 2) {
        char *sel = g_strjoinv(" ", parts + 1);
        char *result = page_find_role_element(state->web_view, sel);
        g_free(sel);
        return result;
    }
    return NULL;
}
