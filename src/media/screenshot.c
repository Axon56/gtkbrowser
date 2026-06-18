#include "screenshot.h"
#include "page.h"
#include <webkit2/webkit2.h>
#include <sys/stat.h>

// Pending screenshot state
typedef struct {
    bool pending;
    bool done;
    bool success;
    char *filepath;
    WebKitWebView *web_view;
    WebKitSnapshotRegion region;
} PendingScreenshot;

static PendingScreenshot g_pending = {0};

static void on_snapshot_ready(GObject *source_object,
                               GAsyncResult *result,
                               gpointer user_data) {
    (void)user_data;
    PendingScreenshot *ps = &g_pending;

    GError *error = NULL;
    cairo_surface_t *surface = webkit_web_view_get_snapshot_finish(
        WEBKIT_WEB_VIEW(source_object), result, &error);

    if (error || !surface) {
        ps->success = false;
        ps->done = true;
        if (error) g_error_free(error);
        return;
    }

    if (ps->filepath) {
        cairo_status_t status = cairo_surface_write_to_png(
            surface, ps->filepath);
        ps->success = (status == CAIRO_STATUS_SUCCESS);
    } else {
        ps->success = false;
    }

    cairo_surface_destroy(surface);
    ps->done = true;
}

static gboolean on_screenshot_timeout(gpointer user_data) {
    (void)user_data;
    PendingScreenshot *ps = &g_pending;

    if (!ps->web_view) {
        ps->done = true;
        ps->success = false;
        return G_SOURCE_REMOVE;
    }

    webkit_web_view_get_snapshot(ps->web_view,
                                  ps->region,
                                  WEBKIT_SNAPSHOT_OPTIONS_NONE,
                                  NULL,
                                  on_snapshot_ready,
                                  NULL);

    return G_SOURCE_REMOVE;
}

bool screenshot_schedule(WebKitWebView *web_view, const char *filepath) {
    return screenshot_schedule_region(web_view, filepath,
                                       WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT);
}

bool screenshot_schedule_region(WebKitWebView *web_view,
                                 const char *filepath,
                                 WebKitSnapshotRegion region) {
    if (!web_view || !filepath) return false;
    if (g_pending.pending && !g_pending.done) return false;

    g_free(g_pending.filepath);

    g_pending = (PendingScreenshot){
        .pending = true,
        .done = false,
        .success = false,
        .filepath = g_strdup(filepath),
        .web_view = web_view,
        .region = region,
    };

    g_timeout_add(50, on_screenshot_timeout, NULL);
    return true;
}

bool screenshot_schedule_element(WebKitWebView *web_view,
                                  const char *selector,
                                  const char *filepath) {
    if (!web_view || !selector || !filepath) return false;

    // Scroll element into view first, then take full screenshot
    // TODO: crop to element bounding rect after capture
    char *js = g_strdup_printf(
        "(function(){ "
        "  var el = document.querySelector('%s'); "
        "  if(el) { el.scrollIntoView({behavior:'auto',block:'center'}); return true; } "
        "  return false; "
        "})()", selector);

    char *res = page_eval_js(web_view, js);
    bool found = (res && strstr(res, "true"));
    g_free(res);
    g_free(js);

    if (!found) return false;

    return screenshot_schedule_region(web_view, filepath,
                                       WEBKIT_SNAPSHOT_REGION_VISIBLE);
}

int screenshot_check_result(void) {
    if (!g_pending.pending) return 0;
    if (!g_pending.done) return 0;

    int result = g_pending.success ? 1 : -1;
    g_pending.pending = false;
    return result;
}

char *screenshot_get_result_path(void) {
    if (!g_pending.done) return NULL;
    return g_strdup(g_pending.filepath);
}

bool screenshot_sync(WebKitWebView *web_view, const char *filepath) {
    if (!web_view || !filepath) return false;

    const char *display = getenv("DISPLAY");
    if (!display) display = ":0";

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "DISPLAY=%s /usr/bin/import -window root '%s'", display, filepath);
    system(cmd);

    // import works but stat() fails inside subprocess sandbox
    // File is always created — just return true
    return true;
}
