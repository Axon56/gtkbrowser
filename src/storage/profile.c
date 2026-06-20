#include "profile.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

static char *g_data_dir = NULL;
static char *g_proxy_user = NULL;
static char *g_proxy_pass = NULL;

static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0755);
    }
}

static char *get_home_dir(void) {
    const char *home = getenv("HOME");
    return home ? g_strdup(home) : g_strdup("/tmp");
}

char *profile_get_default_path(void) {
    char *home = get_home_dir();
    char *path = g_strdup_printf("%s/.local/share/axonsurf/default", home);
    g_free(home);
    return path;
}

char *profile_get_path(const char *name) {
    if (!name) return profile_get_default_path();
    char *home = get_home_dir();
    char *path = g_strdup_printf("%s/.local/share/axonsurf/%s", home, name);
    g_free(home);
    return path;
}

void profile_init(WebKitWebContext *context, const char *data_dir) {
    if (!context || !data_dir) return;

    g_free(g_data_dir);
    g_data_dir = g_strdup(data_dir);

    ensure_dir(data_dir);

    char *cookies_path = g_strdup_printf("%s/cookies.sqlite", data_dir);

    WebKitCookieManager *cookie_mgr =
        webkit_web_context_get_cookie_manager(context);

    webkit_cookie_manager_set_persistent_storage(
        cookie_mgr,
        cookies_path,
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);

    webkit_cookie_manager_set_accept_policy(
        cookie_mgr,
        WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    g_free(cookies_path);
}

char **profile_list(int *count) {
    char *home = get_home_dir();
    char *base = g_strdup_printf("%s/.local/share/axonsurf", home);
    g_free(home);

    ensure_dir(base);

    DIR *dir = opendir(base);
    if (!dir) {
        *count = 0;
        g_free(base);
        return NULL;
    }

    int capacity = 16;
    int n = 0;
    char **names = g_new(char *, capacity);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char *full = g_strdup_printf("%s/%s", base, entry->d_name);
        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (n >= capacity) {
                capacity *= 2;
                names = g_renew(char *, names, capacity);
            }
            names[n++] = g_strdup(entry->d_name);
        }
        g_free(full);
    }

    closedir(dir);
    g_free(base);

    *count = n;
    return names;
}

bool profile_delete(const char *name) {
    if (!name || strcmp(name, "default") == 0) return false;

    char *path = profile_get_path(name);
    char *cmd = g_strdup_printf("rm -rf '%s'", path);
    int ret = system(cmd);
    g_free(cmd);
    g_free(path);

    return (ret == 0);
}

void profile_set_user_agent(WebKitSettings *settings, const char *ua) {
    if (!settings || !ua) return;
    webkit_settings_set_user_agent_with_application_details(
        settings, "AxonSurf", "1.0");
}

// Parse proxy URI and extract host, port, user, pass
static void parse_proxy_uri(const char *uri, char **host, int *port,
                             char **user, char **pass) {
    *host = NULL;
    *port = 0;
    *user = NULL;
    *pass = NULL;

    if (!uri) return;

    const char *scheme_end = strstr(uri, "://");
    if (!scheme_end) return;

    const char *rest = scheme_end + 3;

    const char *at = strchr(rest, '@');
    if (at) {
        const char *user_pass = rest;
        const char *host_start = at + 1;

        int up_len = at - user_pass;
        char *user_pass_str = g_strndup(user_pass, up_len);
        char *colon = strchr(user_pass_str, ':');
        if (colon) {
            *user = g_strndup(user_pass_str, colon - user_pass_str);
            *pass = g_strdup(colon + 1);
        }
        g_free(user_pass_str);
        rest = host_start;
    }

    const char *port_sep = strchr(rest, ':');
    if (port_sep) {
        *host = g_strndup(rest, port_sep - rest);
        *port = atoi(port_sep + 1);
    } else {
        *host = g_strdup(rest);
        *port = 80;
    }
}

// Authentication callback for proxy
static void on_authenticate(WebKitWebContext *context,
                             WebKitAuthenticationRequest *request,
                             gpointer user_data) {
    (void)context;
    (void)user_data;

    // Auto-accept with stored proxy credentials
    if (g_proxy_user && g_proxy_pass) {
        WebKitCredential *cred = webkit_credential_new(
            g_proxy_user,
            g_proxy_pass,
            WEBKIT_CREDENTIAL_PERSISTENCE_NONE);

        webkit_authentication_request_authenticate(request, cred);
        webkit_credential_free(cred);
        fprintf(stderr, "AxonSurf: Proxy authenticated automatically\n");
    } else {
        webkit_authentication_request_cancel(request);
        fprintf(stderr, "AxonSurf: Proxy auth required but no credentials set\n");
    }
}

void profile_set_proxy(WebKitWebContext *context, const char *proxy_uri) {
    if (!context || !proxy_uri) return;

    // Parse the URI to extract credentials
    char *host = NULL, *user = NULL, *pass = NULL;
    int port = 0;
    parse_proxy_uri(proxy_uri, &host, &port, &user, &pass);

    // Build clean proxy URI (host:port only, no credentials)
    char clean_proxy[1024];
    snprintf(clean_proxy, sizeof(clean_proxy), "http://%s:%d",
             host ? host : "127.0.0.1", port);

    // Store credentials for auth callback
    g_free(g_proxy_user);
    g_free(g_proxy_pass);
    g_proxy_user = user;
    g_proxy_pass = pass;

    // Set up proxy settings
    WebKitNetworkProxySettings *proxy_settings =
        webkit_network_proxy_settings_new(clean_proxy, NULL);

    webkit_web_context_set_network_proxy_settings(
        context,
        WEBKIT_NETWORK_PROXY_MODE_CUSTOM,
        proxy_settings);

    webkit_network_proxy_settings_free(proxy_settings);


    fprintf(stderr, "AxonSurf: Proxy set to %s (user: %s)\n",
            clean_proxy, user ? user : "(none)");
    g_free(host);
}

// Connect auth signal to web view (call after web view is created)
void profile_connect_auth_handler(WebKitWebView *web_view) {
    if (!web_view) return;
    g_signal_connect(web_view, "authenticate",
                     G_CALLBACK(on_authenticate), NULL);
}
