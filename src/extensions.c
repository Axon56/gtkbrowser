#include "extensions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <glib.h>

// Internal extension data structure
typedef struct {
    char *name;
    char *filepath;
    char *match_pattern;   // URL pattern to match
    char *include_pattern; // URL pattern to include
    char *run_at;          // document-start, document-end, document-idle
    char *code;            // JavaScript code to inject
    char *css;             // CSS to inject
    bool enabled;
} Extension;

// Global extension state
static GPtrArray *g_extensions = NULL;
static WebKitWebContext *g_web_context = NULL;

// Parse a userscript file
static Extension *parse_userscript(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return NULL;

    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = g_malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    // Check for UserScript header
    if (!strstr(content, "==UserScript==")) {
        g_free(content);
        return NULL;
    }

    Extension *ext = g_new0(Extension, 1);
    ext->filepath = g_strdup(filepath);
    ext->enabled = true;
    ext->run_at = g_strdup("document-end"); // default

    // Parse metadata
    char *line = strtok(content, "\n");
    while (line) {
        if (strstr(line, "@name")) {
            char *key = strstr(line, "@name");
            char *val = key + 5;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->name = g_strdup(val);
        }
        else if (strstr(line, "@match")) {
            char *key = strstr(line, "@match");
            char *val = key + 6;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->match_pattern = g_strdup(val);
        }
        else if (strstr(line, "@include")) {
            char *key = strstr(line, "@include");
            char *val = key + 8;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->include_pattern = g_strdup(val);
        }
        else if (strstr(line, "@run-at")) {
            char *key = strstr(line, "@run-at");
            char *val = key + 7;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->run_at = g_strdup(val);
        }
        else if (strstr(line, "@inject")) {
            char *key = strstr(line, "@inject");
            char *val = key + 7;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->code = g_strdup(val);
        }
        else if (strstr(line, "@css")) {
            char *key = strstr(line, "@css");
            char *val = key + 4;
            while (*val && *val == ' ') val++;
            if (*val == ':') val++;
            while (*val && *val == ' ') val++;
            ext->css = g_strdup(val);
        }
        line = strtok(NULL, "\n");
    }

    // If no name, use filename
    if (!ext->name) {
        char *base = g_path_get_basename(filepath);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        ext->name = base;
    }

    // If no code but file has non-metadata content, use the whole file as code
    if (!ext->code) {
        // Find end of metadata block
        char *end_header = strstr(content, "==/UserScript==");
        if (end_header) {
            end_header += strlen("==/UserScript==");
            while (*end_header == '\n' || *end_header == '\r') end_header++;
            if (strlen(end_header) > 0) {
                ext->code = g_strdup(end_header);
            }
        }
    }

    g_free(content);
    return ext;
}

// Check if a URL matches a pattern
static bool url_matches_pattern(const char *url, const char *pattern) {
    if (!url || !pattern) return false;

    // Simple wildcard matching: https://*/*
    if (strstr(pattern, "*")) {
        // Convert pattern to regex-like match
        GRegex *regex = g_regex_new(pattern, 0, 0, NULL);

        if (regex) {
            gboolean match = g_regex_match(regex, url, 0, NULL);
            g_regex_unref(regex);
            return match;
        }
    }

    // Exact match
    return (strcmp(url, pattern) == 0);
}

// Check if extension should run on this URL
static bool should_run(Extension *ext, const char *url) {
    if (!ext->enabled || !url) return false;

    // Check @match
    if (ext->match_pattern && url_matches_pattern(url, ext->match_pattern))
        return true;

    // Check @include
    if (ext->include_pattern && url_matches_pattern(url, ext->include_pattern))
        return true;

    // If no pattern specified, run on all pages
    if (!ext->match_pattern && !ext->include_pattern)
        return true;

    return false;
}

// Initialize extensions system
void extensions_init(WebKitWebContext *context) {
    if (!g_extensions) {
        g_extensions = g_ptr_array_new_with_free_func((GDestroyNotify)g_free);
    }
    g_web_context = context;
}

// Load all userscripts from a directory
int extensions_load_dir(const char *dirpath) {
    if (!dirpath) return 0;

    DIR *dir = opendir(dirpath);
    if (!dir) return 0;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char *full = g_strdup_printf("%s/%s", dirpath, entry->d_name);

        // Check file extension
        char *dot = strrchr(entry->d_name, '.');
        if (dot && (strcmp(dot, ".js") == 0 || strcmp(dot, ".user.js") == 0)) {
            if (extensions_load_file(full)) count++;
        }
        g_free(full);
    }

    closedir(dir);
    fprintf(stderr, "AxonBrowser: Loaded %d extensions from %s\n", count, dirpath);
    return count;
}

// Load a single userscript file
int extensions_load_file(const char *filepath) {
    Extension *ext = parse_userscript(filepath);
    if (!ext) return 0;

    g_ptr_array_add(g_extensions, ext);
    fprintf(stderr, "AxonBrowser: Loaded extension '%s' (%s)\n",
            ext->name, ext->filepath);
    return 1;
}

// Unload all extensions
void extensions_unload_all(void) {
    if (g_extensions) {
        g_ptr_array_set_size(g_extensions, 0);
    }
    fprintf(stderr, "AxonBrowser: All extensions unloaded\n");
}

// Get list of loaded extensions as JSON
char *extensions_list_json(void) {
    if (!g_extensions || g_extensions->len == 0) {
        return strdup("[]");
    }

    GString *json = g_string_new("[");
    for (unsigned int i = 0; i < g_extensions->len; i++) {
        Extension *ext = g_ptr_array_index(g_extensions, i);
        if (i > 0) g_string_append_c(json, ',');
        g_string_append_printf(json,
            "{\"name\":\"%s\",\"file\":\"%s\",\"match\":\"%s\",\"run_at\":\"%s\",\"enabled\":%s}",
            ext->name ? ext->name : "unnamed",
            ext->filepath ? ext->filepath : "",
            ext->match_pattern ? ext->match_pattern : "*",
            ext->run_at ? ext->run_at : "document-end",
            ext->enabled ? "true" : "false");
    }
    g_string_append_c(json, ']');

    char *result = json->str;
    g_string_free(json, FALSE);
    return result;
}

// Get count of loaded extensions
int extensions_count(void) {
    return g_extensions ? g_extensions->len : 0;
}

// This function is called from the load-changed signal handler
// It injects extension code into pages when they finish loading
void extensions_inject_for_url(WebKitWebView *web_view, const char *url) {
    if (!g_extensions || g_extensions->len == 0 || !web_view || !url) return;

    for (unsigned int i = 0; i < g_extensions->len; i++) {
        Extension *ext = g_ptr_array_index(g_extensions, i);
        if (!should_run(ext, url)) continue;

        // Inject CSS if present
        if (ext->css) {
            char *css_js = g_strdup_printf(
                "(function(){"
                "  var s = document.createElement('style');"
                "  s.textContent = '%s';"
                "  document.head.appendChild(s);"
                "})()", ext->css);
            webkit_web_view_run_javascript(web_view, css_js, NULL, NULL, NULL);
            g_free(css_js);
        }

        // Inject JS if present
        if (ext->code) {
            webkit_web_view_run_javascript(web_view, ext->code, NULL, NULL, NULL);
        }
    }
}
