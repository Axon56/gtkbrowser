#include "extensions.h"
#include "cmd_utils.h"
#include "../extensions/extensions.h"
#include "../page.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

char *cmd_extensions(BrowserState *state, const char *cmd, int argc, char **parts) {
    if (strcmp(cmd, "extension-load") == 0 && argc >= 2) {
        int count = extensions_load_dir(parts[1]);
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", count);
        return cmd_json_result("loaded", count_str);
    }
    if (strcmp(cmd, "extension-file") == 0 && argc >= 2) {
        int count = extensions_load_file(parts[1]);
        return count > 0 ? cmd_json_ok() : cmd_json_error("failed_to_load");
    }
    if (strcmp(cmd, "extension-list") == 0) {
        return extensions_list_json();
    }
    if (strcmp(cmd, "extension-unload") == 0) {
        extensions_unload_all();
        return cmd_json_ok();
    }
    if (strcmp(cmd, "extension-inject") == 0) {
        const char *uri = browser_get_url(state);
        int injected = 0;
        DIR *dir = opendir("/home/gtkbrowser/extensions");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                char *dot = strrchr(entry->d_name, '.');
                if (!dot || (strcmp(dot, ".js") != 0 && strcmp(dot + 1, ".user.js") != 0))
                    continue;
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "/home/gtkbrowser/extensions/%s", entry->d_name);
                FILE *f = fopen(fullpath, "r");
                if (!f) continue;
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                char *code = g_malloc(fsize + 1);
                fread(code, 1, fsize, f);
                code[fsize] = '\0';
                fclose(f);
                char *code_start = strstr(code, "==/GES==");
                if (!code_start) code_start = strstr(code, "==/UserScript==");
                if (code_start) {
                    code_start += 9;
                    while (*code_start == '\n' || *code_start == '\r') code_start++;
                }
                char *res = page_eval_js(state->web_view, code_start ? code_start : code);
                if (res) g_free(res);
                g_free(code);
                injected++;
            }
            closedir(dir);
        }
        if (uri) g_free((char*)uri);
        return cmd_json_result("injected", injected > 0 ? "true" : "false");
    }
    if (strcmp(cmd, "extension-count") == 0) {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", extensions_count());
        return cmd_json_result("count", count_str);
    }
    return NULL;
}
