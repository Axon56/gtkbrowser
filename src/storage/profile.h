#ifndef PROFILE_H
#define PROFILE_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Cookie management
void profile_init(WebKitWebContext *context, const char *data_dir);
char *profile_get_default_path(void);
char *profile_get_path(const char *name);
char **profile_list(int *count);
bool profile_delete(const char *name);

// User agent
void profile_set_user_agent(WebKitSettings *settings, const char *ua);

// Proxy
void profile_set_proxy(WebKitWebContext *context, const char *proxy_uri);

// Connect auth handler to web view (call after web view is created)
void profile_connect_auth_handler(WebKitWebView *web_view);

#endif // PROFILE_H
