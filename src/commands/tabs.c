#include "tabs.h"
#include "cmd_utils.h"

char *cmd_tabs(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "tabs") == 0) {
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
        char *result = json_generator_to_data(gen, NULL);
        g_object_unref(gen);
        g_object_unref(b);
        g_free(url);
        g_free(title);
        return result;
    }
    if (strcmp(cmd, "tab") == 0 && argc >= 2) {
        int idx = atoi(parts[1]);
        browser_switch_tab(state, idx);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "newtab") == 0) {
        const char *url = argc >= 2 ? parts[1] : "about:blank";
        browser_add_tab(state, url);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "closetab") == 0 && argc >= 2) {
        int idx = atoi(parts[1]);
        browser_close_tab(state, idx);
        return cmd_json_ok();
    }
    return NULL;
}
