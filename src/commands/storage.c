#include "storage.h"
#include "cmd_utils.h"
#include "../page.h"

char *cmd_storage(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "ls-get") == 0 && argc >= 2) {
        return page_local_storage_get(state->web_view, parts[1]);
    }
    if (strcmp(cmd, "ls-set") == 0 && argc >= 3) {
        char *val = g_strjoinv(" ", parts + 2);
        page_local_storage_set(state->web_view, parts[1], val);
        g_free(val);
        return cmd_json_ok();
    }
    if (strcmp(cmd, "ls-all") == 0) {
        return page_local_storage_all(state->web_view);
    }
    if (strcmp(cmd, "ss-get") == 0 && argc >= 2) {
        return page_session_storage_get(state->web_view, parts[1]);
    }
    if (strcmp(cmd, "ss-set") == 0 && argc >= 3) {
        char *val = g_strjoinv(" ", parts + 2);
        page_session_storage_set(state->web_view, parts[1], val);
        g_free(val);
        return cmd_json_ok();
    }
    return NULL;
}
