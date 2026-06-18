#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <webkit2/webkit2.h>

// Navigation history
void page_go_back(WebKitWebView *web_view);
void page_go_forward(WebKitWebView *web_view);
int page_get_history_length(WebKitWebView *web_view);
int page_get_history_index(WebKitWebView *web_view);
void page_goto_history(WebKitWebView *web_view, int index);

// Set viewport size
void page_set_viewport(WebKitWebView *web_view, int width, int height);

#endif // NAVIGATION_H
