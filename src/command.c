#include "command.h"
#include "commands/navigation.h"
#include "commands/input.h"
#include "commands/elements.h"
#include "commands/screenshot.h"
#include "commands/tabs.h"
#include "commands/storage.h"
#include "commands/media.h"
#include "commands/monitoring.h"
#include "commands/extensions.h"
#include "commands/window.h"
#include "commands/core.h"
#include "commands/cmd_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <json-glib/json-glib.h>

char *command_process(BrowserState *state, const char *line) {
    if (!line || !*line) return NULL;

    char *clean = g_strdup(line);
    g_strstrip(clean);

    if (strlen(clean) == 0) {
        g_free(clean);
        return NULL;
    }

    char **parts = g_strsplit(clean, " ", -1);
    int argc = g_strv_length(parts);

    if (argc == 0) {
        g_strfreev(parts);
        g_free(clean);
        return NULL;
    }

    const char *cmd = parts[0];
    char *result = NULL;

    // Handle --tab flag: switch to specified tab, execute, switch back
    int target_tab = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(parts[i], "--tab") == 0 && i + 1 < argc) {
            target_tab = atoi(parts[i + 1]);
            break;
        }
    }
    int saved_tab = -1;
    if (target_tab >= 0 && target_tab != state->active_tab) {
        saved_tab = state->active_tab;
        browser_switch_tab(state, target_tab);
    }

    // Dispatch to command modules
    if (!result) result = cmd_navigation(state, cmd, argc, parts);
    if (!result) result = cmd_input(state, cmd, argc, parts);
    if (!result) result = cmd_elements(state, cmd, argc, parts);
    if (!result) result = cmd_screenshot(state, cmd, argc, parts);
    if (!result) result = cmd_tabs(state, cmd, argc, parts);
    if (!result) result = cmd_storage(state, cmd, argc, parts);
    if (!result) result = cmd_media(state, cmd, argc, parts);
    if (!result) result = cmd_monitoring(state, cmd, argc, parts);
    if (!result) result = cmd_extensions(state, cmd, argc, parts);
    if (!result) result = cmd_window(state, cmd, argc, parts);
    if (!result) result = cmd_core(state, cmd, argc, parts);

    if (!result) result = cmd_json_error("unknown_command");

    // Switch back to original tab if --tab was used
    if (saved_tab >= 0) {
        browser_switch_tab(state, saved_tab);
    }

    g_strfreev(parts);
    g_free(clean);

    return result;
}

// === Persistent socket listener ===

static gboolean on_client_readable(GIOChannel *channel,
                                    GIOCondition condition,
                                    gpointer user_data) {
    BrowserState *state = (BrowserState *)user_data;

    if (condition & G_IO_HUP) {
        // Client disconnected — close this channel, stop watching
        g_io_channel_shutdown(channel, TRUE, NULL);
        g_io_channel_unref(channel);
        return FALSE;
    }

    gchar *line = NULL;
    gsize len = 0;
    GError *error = NULL;

    GIOStatus status = g_io_channel_read_line(channel, &line, &len,
                                               NULL, &error);

    if (status == G_IO_STATUS_NORMAL && line) {
        char *response = command_process(state, line);

        if (response) {
            gsize written = 0;
            g_io_channel_write_chars(channel, response, -1,
                                      &written, &error);
            g_io_channel_write_chars(channel, "\n", -1,
                                      &written, &error);
            g_io_channel_flush(channel, NULL);
            g_free(response);
        }
        g_free(line);
    } else if (status == G_IO_STATUS_EOF) {
        g_io_channel_shutdown(channel, TRUE, NULL);
        g_io_channel_unref(channel);
        return FALSE;
    }

    return TRUE;
}

// Callback to accept new connections
static gboolean on_server_readable(GIOChannel *channel,
                                    GIOCondition condition,
                                    gpointer user_data) {
    (void)condition;
    BrowserState *state = (BrowserState *)user_data;

    int client_fd = accept(g_io_channel_unix_get_fd(channel), NULL, NULL);
    if (client_fd < 0) return TRUE; // keep listening

    GIOChannel *client_channel = g_io_channel_unix_new(client_fd);
    g_io_channel_set_encoding(client_channel, NULL, NULL);
    g_io_channel_set_buffer_size(client_channel, 8192);

    // Watch this client — when it disconnects, server keeps listening
    g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP,
                   on_client_readable, state);

    fprintf(stderr, "AxonBrowser: client connected (fd %d)\n", client_fd);
    return TRUE; // keep listening for more clients
}

int command_init(BrowserState *state, const char *socket_path) {
    if (!socket_path) {
        // Use stdin mode
        GIOChannel *stdin_channel = g_io_channel_unix_new(STDIN_FILENO);
        g_io_channel_set_encoding(stdin_channel, NULL, NULL);
        state->socket_watch_id = g_io_add_watch(stdin_channel,
                                                 G_IO_IN | G_IO_HUP,
                                                 on_client_readable,
                                                 state);
        return 0;
    }

    // Create persistent Unix socket
    struct sockaddr_un addr;
    state->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (state->socket_fd < 0) return -1;

    unlink(socket_path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(state->socket_fd, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        close(state->socket_fd);
        return -1;
    }

    if (listen(state->socket_fd, 5) < 0) {
        close(state->socket_fd);
        return -1;
    }

    // Server channel — keeps listening for multiple clients
    GIOChannel *server_channel = g_io_channel_unix_new(state->socket_fd);
    g_io_channel_set_encoding(server_channel, NULL, NULL);

    state->socket_watch_id = g_io_add_watch(server_channel,
                                             G_IO_IN,
                                             on_server_readable,
                                             state);

    fprintf(stderr, "AxonBrowser: listening on %s\n", socket_path);
    return 0;
}

void command_cleanup(BrowserState *state) {
    if (state->socket_watch_id > 0) {
        g_source_remove(state->socket_watch_id);
        state->socket_watch_id = 0;
    }
    if (state->socket_fd >= 0) {
        close(state->socket_fd);
        state->socket_fd = -1;
    }
}
