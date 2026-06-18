#include "screenshot.h"
#include "cmd_utils.h"
#include "../screenshot.h"
#include "../page.h"

char *cmd_screenshot(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "screenshot") == 0 && argc >= 2) {
        if (strcmp(parts[1], "fullpage") == 0 && argc >= 3) {
            bool ok = screenshot_sync(state->web_view, parts[2]);
            return ok ? cmd_json_ok() : cmd_json_error("screenshot_failed");
        } else if (strcmp(parts[1], "viewport") == 0 && argc >= 3) {
            bool ok = screenshot_sync(state->web_view, parts[2]);
            return ok ? cmd_json_ok() : cmd_json_error("screenshot_failed");
        } else if (strcmp(parts[1], "element") == 0 && argc >= 4) {
            screenshot_schedule_element(state->web_view, parts[2], parts[3]);
            return cmd_json_result("status", "scheduled");
        } else {
            bool ok = screenshot_sync(state->web_view, parts[1]);
            return ok ? cmd_json_ok() : cmd_json_error("screenshot_failed");
        }
    }
    if (strcmp(cmd, "pdf") == 0 && argc >= 2) {
        bool ok = page_export_pdf(state->web_view, parts[1]);
        return ok ? cmd_json_ok() : cmd_json_error("pdf_export_failed");
    }
    return NULL;
}
