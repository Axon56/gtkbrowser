#ifndef INPUT_H
#define INPUT_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "page.h"

// Native click at coordinates — fires real GTK events
void input_click(WebKitWebView *web_view, int x, int y);
void input_double_click(WebKitWebView *web_view, int x, int y);
void input_right_click(WebKitWebView *web_view, int x, int y);
void input_mouse_down(WebKitWebView *web_view, int x, int y);
void input_mouse_up(WebKitWebView *web_view, int x, int y);
void input_mouse_move(WebKitWebView *web_view, int x, int y);
void input_hover(WebKitWebView *web_view, int x, int y);

// Type text — fires key press/release for each character
void input_type_text(WebKitWebView *web_view, const char *text);

// Press a special key
void input_key_press(WebKitWebView *web_view, const char *key_name);

// Type into a specific element found by selector
void input_type_into(WebKitWebView *web_view, const char *selector, const char *text);

// Scroll
void input_scroll(WebKitWebView *web_view, int delta_x, int delta_y);
void input_scroll_to(WebKitWebView *web_view, const char *selector);

// Focus an element
void input_focus(WebKitWebView *web_view, const char *selector);

#endif // INPUT_H
