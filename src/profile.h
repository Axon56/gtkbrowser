#ifndef PROFILE_H
#define PROFILE_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Initialize cookie persistence for a web context
// data_dir: where to store cookies/profile (e.g. ~/.local/share/gtkbrowser/default)
void profile_init(WebKitWebContext *context, const char *data_dir);

// Get the default profile path
char *profile_get_default_path(void);

// Get a named profile path
char *profile_get_path(const char *name);

// List all saved profiles
char **profile_list(int *count);

// Delete a profile
bool profile_delete(const char *name);

// Set custom user agent
void profile_set_user_agent(WebKitSettings *settings, const char *ua);

// Set proxy
void profile_set_proxy(WebKitWebContext *context,
                        const char *proxy_uri);

#endif // PROFILE_H
