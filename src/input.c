#include "input.h"
#include <string.h>
#include <stdlib.h>

// Helper: get the default pointer device
static GdkDevice *get_default_device(void) {
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    return gdk_seat_get_pointer(seat);
}

// Helper: convert key name to GDK keyval
static guint key_name_to_val(const char *name) {
    if (!name) return 0;

    if (strcmp(name, "Return") == 0 || strcmp(name, "Enter") == 0)
        return GDK_KEY_Return;
    if (strcmp(name, "Tab") == 0) return GDK_KEY_Tab;
    if (strcmp(name, "Escape") == 0 || strcmp(name, "Esc") == 0)
        return GDK_KEY_Escape;
    if (strcmp(name, "Backspace") == 0) return GDK_KEY_BackSpace;
    if (strcmp(name, "Delete") == 0) return GDK_KEY_Delete;
    if (strcmp(name, "Space") == 0) return GDK_KEY_space;
    if (strcmp(name, "ArrowUp") == 0) return GDK_KEY_Up;
    if (strcmp(name, "ArrowDown") == 0) return GDK_KEY_Down;
    if (strcmp(name, "ArrowLeft") == 0) return GDK_KEY_Left;
    if (strcmp(name, "ArrowRight") == 0) return GDK_KEY_Right;
    if (strcmp(name, "Home") == 0) return GDK_KEY_Home;
    if (strcmp(name, "End") == 0) return GDK_KEY_End;
    if (strcmp(name, "PageUp") == 0) return GDK_KEY_Page_Up;
    if (strcmp(name, "PageDown") == 0) return GDK_KEY_Page_Down;
    if (strcmp(name, "F5") == 0) return GDK_KEY_F5;
    if (strcmp(name, "Ctrl+A") == 0) return GDK_KEY_a;
    if (strcmp(name, "Ctrl+C") == 0) return GDK_KEY_c;
    if (strcmp(name, "Ctrl+V") == 0) return GDK_KEY_v;

    if (strlen(name) == 1) {
        return gdk_unicode_to_keyval((guint32)name[0]);
    }

    return 0;
}

// Helper: create a GDK button event
static GdkEvent *make_button_event(WebKitWebView *web_view, int x, int y,
                                    GdkEventType type, guint button) {
    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(web_view));
    if (!gdk_window) return NULL;

    GdkEvent *event = gdk_event_new(type);
    event->button.window = g_object_ref(gdk_window);
    event->button.send_event = TRUE;
    event->button.time = GDK_CURRENT_TIME;
    event->button.x = (gdouble)x;
    event->button.y = (gdouble)y;
    gint wx = 0, wy = 0;
    gdk_window_get_position(gdk_window, &wx, &wy);
    event->button.x_root = (gdouble)(x + wx);
    event->button.y_root = (gdouble)(y + wy);
    event->button.button = button;
    event->button.state = 0;
    event->button.device = get_default_device();

    return event;
}

void input_click(WebKitWebView *web_view, int x, int y) {
    input_mouse_move(web_view, x, y);

    GdkEvent *press = make_button_event(web_view, x, y,
                                         GDK_BUTTON_PRESS, 1);
    if (press) {
        gtk_main_do_event(press);
        gdk_event_free(press);
    }

    GdkEvent *release = make_button_event(web_view, x, y,
                                           GDK_BUTTON_RELEASE, 1);
    if (release) {
        gtk_main_do_event(release);
        gdk_event_free(release);
    }
}

void input_double_click(WebKitWebView *web_view, int x, int y) {
    input_mouse_move(web_view, x, y);

    GdkEvent *event = make_button_event(web_view, x, y,
                                         GDK_2BUTTON_PRESS, 1);
    if (event) {
        gtk_main_do_event(event);
        gdk_event_free(event);
    }

    GdkEvent *release = make_button_event(web_view, x, y,
                                           GDK_BUTTON_RELEASE, 1);
    if (release) {
        gtk_main_do_event(release);
        gdk_event_free(release);
    }
}

void input_right_click(WebKitWebView *web_view, int x, int y) {
    input_mouse_move(web_view, x, y);

    GdkEvent *press = make_button_event(web_view, x, y,
                                         GDK_BUTTON_PRESS, 3);
    if (press) {
        gtk_main_do_event(press);
        gdk_event_free(press);
    }

    GdkEvent *release = make_button_event(web_view, x, y,
                                           GDK_BUTTON_RELEASE, 3);
    if (release) {
        gtk_main_do_event(release);
        gdk_event_free(release);
    }
}

void input_mouse_down(WebKitWebView *web_view, int x, int y) {
    GdkEvent *press = make_button_event(web_view, x, y,
                                         GDK_BUTTON_PRESS, 1);
    if (press) {
        gtk_main_do_event(press);
        gdk_event_free(press);
    }
}

void input_mouse_up(WebKitWebView *web_view, int x, int y) {
    GdkEvent *release = make_button_event(web_view, x, y,
                                           GDK_BUTTON_RELEASE, 1);
    if (release) {
        gtk_main_do_event(release);
        gdk_event_free(release);
    }
}

void input_mouse_move(WebKitWebView *web_view, int x, int y) {
    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(web_view));
    if (!gdk_window) return;

    GdkEvent *event = gdk_event_new(GDK_MOTION_NOTIFY);
    event->motion.window = g_object_ref(gdk_window);
    event->motion.send_event = TRUE;
    event->motion.time = GDK_CURRENT_TIME;
    event->motion.x = (gdouble)x;
    event->motion.y = (gdouble)y;
    event->motion.state = 0;
    event->motion.device = get_default_device();

    gtk_main_do_event(event);
    gdk_event_free(event);
}

void input_type_text(WebKitWebView *web_view, const char *text) {
    if (!text) return;

    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(web_view));
    if (!gdk_window) return;

    GdkKeymap *keymap = gdk_keymap_get_for_display(
        gdk_display_get_default());

    while (*text) {
        guint32 ch = g_utf8_get_char(text);
        guint keyval = gdk_unicode_to_keyval(ch);
        guint16 hardware_keycode = 0;
        guint state = 0;

        GdkKeymapKey *keys;
        gint n_keys;
        if (gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys)) {
            if (n_keys > 0) {
                hardware_keycode = keys[0].keycode;
                if (keys[0].level > 0) {
                    state |= GDK_SHIFT_MASK;
                }
            }
            g_free(keys);
        }

        if (hardware_keycode == 0) {
            text = g_utf8_next_char(text);
            continue;
        }

        // Key press
        GdkEvent *press = gdk_event_new(GDK_KEY_PRESS);
        press->key.window = g_object_ref(gdk_window);
        press->key.send_event = TRUE;
        press->key.time = GDK_CURRENT_TIME;
        press->key.state = state;
        press->key.keyval = keyval;
        press->key.hardware_keycode = hardware_keycode;
        press->key.group = 0;
        press->key.length = 0;
        press->key.string = NULL;
        gtk_main_do_event(press);
        gdk_event_free(press);

        // Key release
        GdkEvent *release = gdk_event_new(GDK_KEY_RELEASE);
        release->key.window = g_object_ref(gdk_window);
        release->key.send_event = TRUE;
        release->key.time = GDK_CURRENT_TIME;
        release->key.state = state;
        release->key.keyval = keyval;
        release->key.hardware_keycode = hardware_keycode;
        release->key.group = 0;
        release->key.length = 0;
        release->key.string = NULL;
        gtk_main_do_event(release);
        gdk_event_free(release);

        text = g_utf8_next_char(text);
    }
}

void input_key_press(WebKitWebView *web_view, const char *key_name) {
    guint keyval = key_name_to_val(key_name);
    if (keyval == 0) return;

    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(web_view));
    if (!gdk_window) return;

    GdkKeymap *keymap = gdk_keymap_get_for_display(
        gdk_display_get_default());
    GdkKeymapKey *keys;
    gint n_keys;
    guint16 keycode = 0;
    guint state = 0;

    if (gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys)) {
        if (n_keys > 0) {
            keycode = keys[0].keycode;
            if (keys[0].level > 0) state |= GDK_SHIFT_MASK;
        }
        g_free(keys);
    }

    if (g_str_has_prefix(key_name, "Ctrl+")) {
        state |= GDK_CONTROL_MASK;
    }

    if (keycode == 0) return;

    // Press
    GdkEvent *press = gdk_event_new(GDK_KEY_PRESS);
    press->key.window = g_object_ref(gdk_window);
    press->key.send_event = TRUE;
    press->key.time = GDK_CURRENT_TIME;
    press->key.state = state;
    press->key.keyval = keyval;
    press->key.hardware_keycode = keycode;
    press->key.group = 0;
    press->key.length = 0;
    press->key.string = NULL;
    gtk_main_do_event(press);
    gdk_event_free(press);

    // Release
    GdkEvent *release = gdk_event_new(GDK_KEY_RELEASE);
    release->key.window = g_object_ref(gdk_window);
    release->key.send_event = TRUE;
    release->key.time = GDK_CURRENT_TIME;
    release->key.state = state;
    release->key.keyval = keyval;
    release->key.hardware_keycode = keycode;
    release->key.group = 0;
    release->key.length = 0;
    release->key.string = NULL;
    gtk_main_do_event(release);
    gdk_event_free(release);
}

void input_type_into(WebKitWebView *web_view, const char *selector,
                      const char *text) {
    char *js_focus = g_strdup_printf(
        "(function(){ "
        "  var el = document.querySelector('%s'); "
        "  if(el) { el.focus(); el.click(); return true; } "
        "  return false; "
        "})()", selector);

    char *res = page_eval_js(web_view, js_focus);
    g_free(res);
    g_free(js_focus);

    input_type_text(web_view, text);
}

void input_scroll(WebKitWebView *web_view, int delta_x, int delta_y) {
    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(web_view));
    if (!gdk_window) return;

    GdkEventScroll *scroll = (GdkEventScroll *)gdk_event_new(GDK_SCROLL);
    scroll->window = g_object_ref(gdk_window);
    scroll->send_event = TRUE;
    scroll->time = GDK_CURRENT_TIME;
    scroll->x = 100;
    scroll->y = 100;
    scroll->state = 0;
    scroll->delta_x = delta_x;
    scroll->delta_y = delta_y;
    scroll->device = get_default_device();

    gtk_main_do_event((GdkEvent *)scroll);
    gdk_event_free((GdkEvent *)scroll);
}

void input_scroll_to(WebKitWebView *web_view, const char *selector) {
    char *js = g_strdup_printf(
        "(function(){ "
        "  var el = document.querySelector('%s'); "
        "  if(el) { el.scrollIntoView({behavior:'auto',block:'center'}); return true; } "
        "  return false; "
        "})()", selector);

    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

void input_focus(WebKitWebView *web_view, const char *selector) {
    char *js = g_strdup_printf(
        "(function(){ "
        "  var el = document.querySelector('%s'); "
        "  if(el) { el.focus(); return true; } "
        "  return false; "
        "})()", selector);

    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

void input_hover(WebKitWebView *web_view, int x, int y) {
    input_mouse_move(web_view, x, y);
    // Stay for a realistic hover duration
    g_usleep(200000); // 200ms hover delay
}
