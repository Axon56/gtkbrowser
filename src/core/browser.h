#ifndef BROWSER_H
#define BROWSER_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdbool.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *scrolled_window;
    GtkWidget *url_entry;
    WebKitWebView *web_view;
    WebKitWebContext *web_context;
    WebKitSettings *settings;

    GList *tabs;
    int active_tab;

    int socket_fd;
    guint socket_watch_id;

    char *current_url;
    bool headless;
    int window_width;
    int window_height;

    const char *_socket_path;
    char *_profile_path;
    const char *_proxy_uri;
    const char *_user_agent;
} BrowserState;

// Browser lifecycle
BrowserState *browser_new(bool headless, int width, int height);
void browser_destroy(BrowserState *state);
WebKitWebView *browser_add_tab(BrowserState *state, const char *url);
void browser_switch_tab(BrowserState *state, int index);
void browser_close_tab(BrowserState *state, int index);
int browser_tab_count(BrowserState *state);
void browser_goto(BrowserState *state, const char *url);
void browser_set_size(BrowserState *state, int width, int height);

// Page operations
char *browser_get_url(BrowserState *state);
char *browser_get_title(BrowserState *state);

// Window management
void browser_maximize(BrowserState *state);
void browser_minimize(BrowserState *state);
void browser_fullscreen(BrowserState *state);
void browser_unfullscreen(BrowserState *state);
void browser_center(BrowserState *state);

#endif // BROWSER_H
