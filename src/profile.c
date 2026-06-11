#include "profile.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

static char *g_data_dir = NULL;

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
    char *path = g_strdup_printf("%s/.local/share/gtkbrowser/default", home);
    g_free(home);
    return path;
}

char *profile_get_path(const char *name) {
    if (!name) return profile_get_default_path();
    char *home = get_home_dir();
    char *path = g_strdup_printf("%s/.local/share/gtkbrowser/%s", home, name);
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
    char *base = g_strdup_printf("%s/.local/share/gtkbrowser", home);
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
        settings, "GTKBrowser", "1.0");
}

void profile_set_proxy(WebKitWebContext *context, const char *proxy_uri) {
    if (!context || !proxy_uri) return;

    WebKitNetworkProxySettings *proxy_settings =
        webkit_network_proxy_settings_new(proxy_uri, NULL);

    webkit_web_context_set_network_proxy_settings(
        context,
        WEBKIT_NETWORK_PROXY_MODE_CUSTOM,
        proxy_settings);

    webkit_network_proxy_settings_free(proxy_settings);
}
