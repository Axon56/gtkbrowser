#ifndef MEDIA_H
#define MEDIA_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Checkbox/Radio
void page_check(WebKitWebView *web_view, const char *selector);
void page_uncheck(WebKitWebView *web_view, const char *selector);
bool page_is_checked(WebKitWebView *web_view, const char *selector);

// File upload
void page_upload_file(WebKitWebView *web_view, const char *selector, const char *filepath);

// Drag and drop simulation
void page_drag(WebKitWebView *web_view, int sx, int sy, int ex, int ey);

// PDF export
bool page_export_pdf(WebKitWebView *web_view, const char *filepath);

#endif // MEDIA_H
