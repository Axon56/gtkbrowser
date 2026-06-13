#include "browser.h"
#include "command.h"
#include "profile.h"
#include "humanize.h"
#include "headless.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static BrowserState *g_state = NULL;
static GtkApplication *g_app = NULL;

static void handle_signal(int sig) {
    (void)sig;
    if (g_state) {
        command_cleanup(g_state);
    }
    headless_stop();
    if (g_app) {
        g_application_quit(G_APPLICATION(g_app));
    }
    exit(0);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    BrowserState *state = (BrowserState *)user_data;

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "GTKBrowser");
    gtk_window_set_default_size(GTK_WINDOW(state->window),
                                state->window_width, state->window_height);

    state->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(state->window), state->scrolled_window);

    state->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Connect auth handler for proxy authentication
    profile_connect_auth_handler(state->web_view);

    state->settings = webkit_settings_new();
    webkit_settings_set_javascript_can_open_windows_automatically(state->settings, FALSE);
    webkit_settings_set_allow_modal_dialogs(state->settings, FALSE);
    webkit_settings_set_enable_smooth_scrolling(state->settings, TRUE);
    webkit_web_view_set_settings(state->web_view, state->settings);

    // Set up cookie persistence
    WebKitWebContext *web_context = webkit_web_view_get_context(state->web_view);
    profile_init(web_context, state->_profile_path);

    // Set proxy if specified
    if (state->_proxy_uri) {
        profile_set_proxy(web_context, state->_proxy_uri);
    }

    // Set user agent if specified
    if (state->_user_agent) {
        webkit_settings_set_user_agent(state->settings, state->_user_agent);
    }

    gtk_container_add(GTK_CONTAINER(state->scrolled_window),
                      GTK_WIDGET(state->web_view));

    state->tabs = g_list_append(NULL, state->web_view);
    state->active_tab = 0;

    gtk_widget_show_all(state->window);

    // Init command interface after window is shown
    command_init(state, state->_socket_path);

    // Load initial URL
    if (state->current_url) {
        webkit_web_view_load_uri(state->web_view, state->current_url);
    }

    printf("GTKBrowser ready.\n");
    fflush(stdout);
}

static void print_usage(const char *prog) {
    printf("GTKBrowser — native-input browser automation\n\n");
    printf("Usage:\n");
    printf("  %s [options] [url]\n\n", prog);
    printf("Options:\n");
    printf("  --headless              Run without visible window (needs Xvfb)\n");
    printf("  --width  <px>           Window width (default: 1280)\n");
    printf("  --height <px>           Window height (default: 800)\n");
    printf("  --socket <path>         Command socket path (default: stdin)\n");
    printf("  --profile <name>        Profile name for cookies/session (default: default)\n");
    printf("  --humanize <0-100>      Set humanization level at launch\n");
    printf("  --proxy <uri>           Proxy (e.g. socks5://127.0.0.1:1080)\n");
    printf("  --ua <string>           Custom user agent\n");
    printf("  --help                  Show this help\n\n");
    printf("Humanize levels:\n");
    printf("  0      Robotic — instant, no delays\n");
    printf("  25     Fast — slight delays, no mouse movement\n");
    printf("  50     Moderate — realistic delays, basic mouse path\n");
    printf("  75     Slow — longer delays, smooth mouse curves\n");
    printf("  100    Human — full mimicry with pauses and jitter\n\n");
    printf("Examples:\n");
    printf("  %s --humanize 75 https://example.com\n", prog);
    printf("  %s --profile mybot --humanize 50\n", prog);
    printf("  %s --proxy socks5://127.0.0.1:1080 --ua 'Mozilla/5.0'\n", prog);
}

int main(int argc, char *argv[]) {
    bool headless = false;
    int width = 1280;
    int height = 800;
    const char *socket_path = NULL;
    const char *profile_name = "default";
    const char *proxy_uri = NULL;
    const char *user_agent = NULL;
    const char *initial_url = NULL;
    int humanize_level = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) {
            socket_path = argv[++i];
        } else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            profile_name = argv[++i];
        } else if (strcmp(argv[i], "--humanize") == 0 && i + 1 < argc) {
            humanize_level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--proxy") == 0 && i + 1 < argc) {
            proxy_uri = argv[++i];
        } else if (strcmp(argv[i], "--ua") == 0 && i + 1 < argc) {
            user_agent = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            initial_url = argv[i];
        }
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Auto-start Xvfb if headless and no display
    if (headless) {
        headless_auto_start();
    }

    // Initialize humanization
    humanize_init();
    if (humanize_level >= 0) {
        humanize_set_level(humanize_level);
    }

    // Set up profile path
    char *profile_path = profile_get_path(profile_name);

    // Create browser state
    g_state = g_new0(BrowserState, 1);
    g_state->headless = headless;
    g_state->window_width = width;
    g_state->window_height = height;
    g_state->socket_fd = -1;
    g_state->_socket_path = socket_path;
    g_state->_profile_path = profile_path;
    g_state->_proxy_uri = proxy_uri;
    g_state->_user_agent = user_agent;

    if (initial_url) {
        g_state->current_url = g_strdup(initial_url);
    }

    // Create and run GTK application
    g_app = gtk_application_new("com.axon.gtkbrowser",
                                 G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(g_app, "activate", G_CALLBACK(on_activate), g_state);

    int status = g_application_run(G_APPLICATION(g_app), 0, NULL);

    // Cleanup
    command_cleanup(g_state);
    headless_stop();
    g_object_unref(g_app);
    g_free(g_state->current_url);
    g_free(profile_path);
    g_free(g_state);

    return status;
}
