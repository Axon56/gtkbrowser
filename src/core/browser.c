#include "browser.h"
#include "../page.h"
#include <string.h>

static void on_url_entry_activate(GtkEntry *entry, gpointer user_data) {
    BrowserState *state = (BrowserState *)user_data;
    const char *url = gtk_entry_get_text(entry);
    if (url && url[0]) {
        webkit_web_view_load_uri(state->web_view, url);
    }
}

static void on_back_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    BrowserState *state = (BrowserState *)user_data;
    webkit_web_view_go_back(state->web_view);
}

static void on_forward_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    BrowserState *state = (BrowserState *)user_data;
    webkit_web_view_go_forward(state->web_view);
}

static void on_reload_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    BrowserState *state = (BrowserState *)user_data;
    webkit_web_view_reload(state->web_view);
}

static void on_uri_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    BrowserState *state = (BrowserState *)user_data;
    if (!state->url_entry) return;
    const char *uri = webkit_web_view_get_uri(web_view);
    if (uri) {
        gtk_entry_set_text(GTK_ENTRY(state->url_entry), uri);
    }
}

static void on_ready(GtkApplication *app, gpointer user_data) {
    BrowserState *state = (BrowserState *)user_data;

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "AxonSurf");
    gtk_window_set_default_size(GTK_WINDOW(state->window),
                                state->window_width, state->window_height);

    // Main vertical layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);

    // === Navigation bar ===
    GtkWidget *nav_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(nav_bar, 4);
    gtk_widget_set_margin_end(nav_bar, 4);
    gtk_widget_set_margin_top(nav_bar, 4);
    gtk_widget_set_margin_bottom(nav_bar, 4);
    gtk_box_pack_start(GTK_BOX(vbox), nav_bar, FALSE, FALSE, 0);

    // Back button
    GtkWidget *back_btn = gtk_button_new_with_label("◀");
    gtk_widget_set_size_request(back_btn, 36, 28);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), state);
    gtk_box_pack_start(GTK_BOX(nav_bar), back_btn, FALSE, FALSE, 0);

    // Forward button
    GtkWidget *fwd_btn = gtk_button_new_with_label("▶");
    gtk_widget_set_size_request(fwd_btn, 36, 28);
    g_signal_connect(fwd_btn, "clicked", G_CALLBACK(on_forward_clicked), state);
    gtk_box_pack_start(GTK_BOX(nav_bar), fwd_btn, FALSE, FALSE, 0);

    // Reload button
    GtkWidget *reload_btn = gtk_button_new_with_label("↻");
    gtk_widget_set_size_request(reload_btn, 36, 28);
    g_signal_connect(reload_btn, "clicked", G_CALLBACK(on_reload_clicked), state);
    gtk_box_pack_start(GTK_BOX(nav_bar), reload_btn, FALSE, FALSE, 0);

    // URL entry
    state->url_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->url_entry), "Enter URL...");
    g_signal_connect(state->url_entry, "activate", G_CALLBACK(on_url_entry_activate), state);
    gtk_box_pack_start(GTK_BOX(nav_bar), state->url_entry, TRUE, TRUE, 0);

    // === Tab bar ===
    GtkWidget *tab_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(tab_bar, 4);
    gtk_widget_set_margin_end(tab_bar, 4);
    gtk_widget_set_margin_bottom(tab_bar, 2);
    gtk_box_pack_start(GTK_BOX(vbox), tab_bar, FALSE, FALSE, 0);

    // Tab 1
    GtkWidget *tab1 = gtk_button_new_with_label("Tab 1");
    gtk_widget_set_size_request(tab1, 100, 24);
    gtk_box_pack_start(GTK_BOX(tab_bar), tab1, TRUE, TRUE, 0);

    // New tab button
    GtkWidget *new_tab_btn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(new_tab_btn, 28, 24);
    gtk_box_pack_start(GTK_BOX(tab_bar), new_tab_btn, FALSE, FALSE, 4);

    // === Web view ===
    state->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), state->scrolled_window, TRUE, TRUE, 0);

    state->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    state->settings = webkit_settings_new();
    webkit_settings_set_javascript_can_open_windows_automatically(state->settings, FALSE);
    webkit_settings_set_allow_modal_dialogs(state->settings, FALSE);
    webkit_settings_set_enable_smooth_scrolling(state->settings, TRUE);
    webkit_web_view_set_settings(state->web_view, state->settings);

    gtk_container_add(GTK_CONTAINER(state->scrolled_window), GTK_WIDGET(state->web_view));

    // Update URL bar when page changes
    g_signal_connect(state->web_view, "notify::uri", G_CALLBACK(on_uri_changed), state);

    state->tabs = g_list_append(NULL, state->web_view);
    state->active_tab = 0;

    gtk_widget_show_all(state->window);
}

BrowserState *browser_new(bool headless, int width, int height) {
    BrowserState *state = g_new0(BrowserState, 1);
    state->headless = headless;
    state->window_width = width > 0 ? width : 1280;
    state->window_height = height > 0 ? height : 800;
    state->active_tab = 0;
    state->socket_fd = -1;
    return state;
}

void browser_destroy(BrowserState *state) {
    if (!state) return;

    GList *l = state->tabs;
    while (l) {
        WebKitWebView *tab = WEBKIT_WEB_VIEW(l->data);
        gtk_widget_destroy(GTK_WIDGET(tab));
        l = l->next;
    }
    g_list_free(state->tabs);

    if (state->window) {
        gtk_widget_destroy(state->window);
    }

    g_free(state->current_url);
    g_free(state);
}

WebKitWebView *browser_add_tab(BrowserState *state, const char *url) {
    WebKitWebView *new_tab = WEBKIT_WEB_VIEW(webkit_web_view_new());
    webkit_web_view_set_settings(new_tab, state->settings);

    state->tabs = g_list_append(state->tabs, new_tab);
    int new_index = g_list_length(state->tabs) - 1;

    browser_switch_tab(state, new_index);

    if (url) {
        webkit_web_view_load_uri(new_tab, url);
    }

    return new_tab;
}

void browser_switch_tab(BrowserState *state, int index) {
    int count = g_list_length(state->tabs);
    if (index < 0 || index >= count) return;

    GtkWidget *current_child = gtk_bin_get_child(GTK_BIN(state->scrolled_window));
    if (current_child) {
        gtk_container_remove(GTK_CONTAINER(state->scrolled_window), current_child);
    }

    state->active_tab = index;
    WebKitWebView *tab = WEBKIT_WEB_VIEW(g_list_nth_data(state->tabs, index));
    state->web_view = tab;
    gtk_container_add(GTK_CONTAINER(state->scrolled_window), GTK_WIDGET(tab));
    gtk_widget_show(GTK_WIDGET(tab));
}

void browser_close_tab(BrowserState *state, int index) {
    int count = g_list_length(state->tabs);
    if (count <= 1 || index < 0 || index >= count) return;

    WebKitWebView *tab = WEBKIT_WEB_VIEW(g_list_nth_data(state->tabs, index));

    if (index == state->active_tab) {
        browser_switch_tab(state, index > 0 ? index - 1 : index + 1);
    } else if (index < state->active_tab) {
        state->active_tab--;
    }

    gtk_widget_destroy(GTK_WIDGET(tab));
    state->tabs = g_list_delete_link(state->tabs, g_list_nth(state->tabs, index));
}

int browser_tab_count(BrowserState *state) {
    return g_list_length(state->tabs);
}

void browser_goto(BrowserState *state, const char *url) {
    if (!url || !state->web_view) return;

    g_free(state->current_url);
    state->current_url = g_strdup(url);

    webkit_web_view_load_uri(state->web_view, url);
}

void browser_set_size(BrowserState *state, int width, int height) {
    if (!state->window) return;
    state->window_width = width;
    state->window_height = height;
    gtk_window_set_default_size(GTK_WINDOW(state->window), width, height);
}

char *browser_get_url(BrowserState *state) {
    if (!state->web_view) return NULL;
    const gchar *uri = webkit_web_view_get_uri(state->web_view);
    return g_strdup(uri ? uri : "");
}

char *browser_get_title(BrowserState *state) {
    if (!state->web_view) return NULL;
    const gchar *title = webkit_web_view_get_title(state->web_view);
    return g_strdup(title ? title : "");
}

void browser_maximize(BrowserState *state) {
    if (!state || !state->window) return;
    gtk_window_maximize(GTK_WINDOW(state->window));
}

void browser_minimize(BrowserState *state) {
    if (!state || !state->window) return;
    gtk_window_iconify(GTK_WINDOW(state->window));
}

void browser_fullscreen(BrowserState *state) {
    if (!state || !state->window) return;
    gtk_window_fullscreen(GTK_WINDOW(state->window));
}

void browser_unfullscreen(BrowserState *state) {
    if (!state || !state->window) return;
    gtk_window_unfullscreen(GTK_WINDOW(state->window));
}

void browser_center(BrowserState *state) {
    if (!state || !state->window) return;
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
}
