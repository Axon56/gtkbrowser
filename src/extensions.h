#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Initialize extensions system
void extensions_init(WebKitWebContext *context);

// Load all userscripts from a directory
int extensions_load_dir(const char *dirpath);

// Load a single userscript file
int extensions_load_file(const char *filepath);

// Unload all extensions
void extensions_unload_all(void);

// Get list of loaded extensions
char *extensions_list_json(void);

// Get count of loaded extensions
int extensions_count(void);

#endif // EXTENSIONS_H

// Synchronous injection using page_eval_js
void extensions_inject_sync(WebKitWebView *web_view, const char *url);
