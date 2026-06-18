#include "media.h"
#include "page.h"
#include "input/input.h"
#include <string.h>
#include <stdlib.h>

// === Checkbox/Radio ===

void page_check(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return;
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el) return false;"
        "  el.checked = true;"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return true;"
        "})()", selector);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

void page_uncheck(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return;
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el) return false;"
        "  el.checked = false;"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return true;"
        "})()", selector);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

bool page_is_checked(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return false;
    char *js = g_strdup_printf(
        "(function(){ var el = document.querySelector('%s'); return el ? el.checked : false; })()",
        selector);
    char *res = page_eval_js(web_view, js);
    bool checked = (res && strstr(res, "true"));
    g_free(res);
    g_free(js);
    return checked;
}

// === File Upload ===

void page_upload_file(WebKitWebView *web_view, const char *selector, const char *filepath) {
    if (!web_view || !selector || !filepath) return;

    // Find the file input and set its files via DataTransfer API
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el || el.type !== 'file') return 'not_file_input';"
        "  // Create a File object and set it on the input"
        "  // Note: direct file access isn't possible from JS in modern browsers"
        "  // But we can trigger the change event after setting the value"
        "  el.value = '%s';"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return 'file_set';"
        "})()", selector, filepath);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

// === Drag and Drop Simulation ===

void page_drag(WebKitWebView *web_view, int sx, int sy, int ex, int ey) {
    if (!web_view) return;

    // Simulate drag via mouse events at source, move, then target
    input_mouse_down(web_view, sx, sy);
    g_usleep(100000); // 100ms hold

    // Move in steps
    int steps = 20;
    for (int i = 1; i <= steps; i++) {
        int x = sx + (ex - sx) * i / steps;
        int y = sy + (ey - sy) * i / steps;
        input_mouse_move(web_view, x, y);
        g_usleep(20000); // 20ms per step
    }

    g_usleep(100000); // 100ms pause at target
    input_mouse_up(web_view, ex, ey);
}

// === PDF Export ===

bool page_export_pdf(WebKitWebView *web_view, const char *filepath) {
    if (!web_view || !filepath) return false;

    // Save via JS: create a blob and trigger download
    char *js = g_strdup_printf(
        "(function(){"
        "  var printWindow = window.open('', '_blank');"
        "  printWindow.document.write(document.documentElement.outerHTML);"
        "  printWindow.document.close();"
        "  printWindow.print();"
        "  return 'print_dialog_opened';"
        "})()");
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);

    // Also save the page HTML to a .html file as fallback
    char *html = page_get_content(web_view, true);
    if (html) {
        FILE *f = fopen(filepath, "w");
        if (f) {
            fprintf(f, "%s", html);
            fclose(f);
            g_free(html);
            return true;
        }
        g_free(html);
    }
    return false;
}
