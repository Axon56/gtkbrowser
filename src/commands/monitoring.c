#include "monitoring.h"
#include "cmd_utils.h"
#include "../page.h"

char *cmd_monitoring(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "net-log") == 0) {
        page_start_network_log(state->web_view);
        return cmd_json_result("status", "network_logging_started");
    }
    if (strcmp(cmd, "net-stop") == 0) {
        page_stop_network_log(state->web_view);
        return cmd_json_result("status", "network_logging_stopped");
    }
    if (strcmp(cmd, "net-requests") == 0) {
        return page_get_network_log(state->web_view);
    }
    if (strcmp(cmd, "perf-timing") == 0) {
        return page_performance_timing(state->web_view);
    }
    if (strcmp(cmd, "perf-memory") == 0) {
        return page_performance_memory(state->web_view);
    }
    if (strcmp(cmd, "a11y-audit") == 0) {
        return page_accessibility_audit(state->web_view);
    }
    if (strcmp(cmd, "ssl") == 0) {
        return page_ssl_info(state->web_view);
    }
    if (strcmp(cmd, "downloads") == 0) {
        return page_get_downloads(state->web_view);
    }
    if (strcmp(cmd, "dismiss") == 0) {
        return page_dismiss_overlays(state->web_view);
    }
    if (strcmp(cmd, "force-elements") == 0) {
        return page_get_elements(state->web_view);
    }
    return NULL;
}
