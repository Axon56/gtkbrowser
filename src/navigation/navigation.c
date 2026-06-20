#include "navigation.h"
#include "page.h"
#include <webkit2/webkit2.h>

// === Navigation History ===

void page_go_back(WebKitWebView *web_view) {
    if (!web_view) return;
    webkit_web_view_go_back(web_view);
}

void page_go_forward(WebKitWebView *web_view) {
    if (!web_view) return;
    webkit_web_view_go_forward(web_view);
}

int page_get_history_length(WebKitWebView *web_view) {
    if (!web_view) return 0;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    return webkit_back_forward_list_get_length(bfl);
}

int page_get_history_index(WebKitWebView *web_view) {
    if (!web_view) return 0;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    int len = webkit_back_forward_list_get_length(bfl);
    WebKitBackForwardListItem *current = webkit_back_forward_list_get_current_item(bfl);
    if (!current) return 0;
    // Count how many back items exist
    GList *back_list = webkit_back_forward_list_get_back_list_with_limit(bfl, len);
    int idx = g_list_length(back_list);
    g_list_free(back_list);
    return idx;
}

void page_goto_history(WebKitWebView *web_view, int index) {
    if (!web_view) return;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    WebKitBackForwardListItem *item = webkit_back_forward_list_get_nth_item(bfl, index);
    if (item) {
        webkit_web_view_go_to_back_forward_list_item(web_view, item);
    }
}

// === Viewport ===

void page_set_viewport(WebKitWebView *web_view, int width, int height) {
    if (!web_view) return;

    // Set viewport meta tag for responsive design
    char *js = g_strdup_printf(
        "(function(){"
        "  var meta = document.querySelector('meta[name=viewport]');"
        "  if(!meta) {"
        "    meta = document.createElement('meta');"
        "    meta.name = 'viewport';"
        "    document.head.appendChild(meta);"
        "  }"
        "  meta.content = 'width=%d, initial-scale=1.0, minimum-scale=0.1';"
        "  return 'viewport_set';"
        "})()", width);

    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);

    // Set mobile user agent to trigger responsive design
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);
    webkit_settings_set_user_agent_with_application_details(
        settings, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Mobile/15E148 Safari/604.1", "AxonSurf");

    // Reload the page so responsive CSS kicks in
    webkit_web_view_reload(web_view);
}
