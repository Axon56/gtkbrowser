#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <webkit2/webkit2.h>

// Find element by selector and return its bounding rect as JSON
char *page_find_element(WebKitWebView *web_view, const char *selector);

// Get all interactive elements as JSON array
char *page_get_elements(WebKitWebView *web_view);

// Find element by accessibility role selector (e.g. "Button:Submit")
char *page_find_role_element(WebKitWebView *web_view, const char *role_selector);

// Click element by accessibility role selector
int page_click_role(WebKitWebView *web_view, const char *role_selector);

// Type into element by accessibility role selector
int page_type_role(WebKitWebView *web_view, const char *role_selector, const char *text);

// Get accessibility tree as JSON
char *page_get_accessibility_tree(WebKitWebView *web_view);

// Dismiss overlays (cookie banners, chat widgets, popups)
char *page_dismiss_overlays(WebKitWebView *web_view);

// Force elements scan (with delay for SPA render)
char *page_force_elements(WebKitWebView *web_view);

#endif // ELEMENTS_H
