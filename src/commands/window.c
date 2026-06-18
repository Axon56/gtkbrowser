#include "window.h"
#include "cmd_utils.h"
#include "../page.h"

char *cmd_window(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "resize") == 0 && argc >= 3) {
        int w = atoi(parts[1]);
        int h = atoi(parts[2]);
        browser_set_size(state, w, h);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "viewport") == 0 && argc >= 3) {
        int w = atoi(parts[1]);
        int h = atoi(parts[2]);
        browser_set_size(state, w, h);
        page_set_viewport(state->web_view, w, h);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "maximize") == 0) {
        browser_maximize(state);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "minimize") == 0) {
        browser_minimize(state);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "fullscreen") == 0) {
        browser_fullscreen(state);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "unfullscreen") == 0) {
        browser_unfullscreen(state);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "center") == 0) {
        browser_center(state);
        return cmd_json_ok();
    }
    return NULL;
}
