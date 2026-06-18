#ifndef CMD_UTILS_H
#define CMD_UTILS_H

#include <json-glib/json-glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

static inline char *cmd_json_result(const char *key, const char *value) {
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, key);
    json_builder_add_string_value(builder, value ? value : "");
    json_builder_end_object(builder);

    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, json_builder_get_root(builder));
    char *json = json_generator_to_data(gen, NULL);

    g_object_unref(gen);
    g_object_unref(builder);
    return json;
}

static inline char *cmd_json_ok(void) {
    return cmd_json_result("ok", "true");
}

static inline char *cmd_json_error(const char *msg) {
    return cmd_json_result("error", msg);
}

static inline char *cmd_clean_eval_result(char *raw) {
    if (!raw) return NULL;
    int len = strlen(raw);
    if (len >= 2 && raw[0] == '"' && raw[len-1] == '"') {
        char *inner = g_strndup(raw + 1, len - 2);
        g_strstrip(inner);
        g_free(raw);
        return inner;
    }
    return raw;
}

/* Check if a string looks like a numeric coordinate */
static inline int cmd_is_coord(const char *s) {
    if (!s || !*s) return 0;
    if (s[0] >= '0' && s[0] <= '9') return 1;
    if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') return 1;
    return 0;
}

/* Parse element center from page_find_element JSON result.
 * Returns 1 on success (sets cx, cy), 0 on failure.
 * Frees rect_json. */
static inline int cmd_parse_element_center(char *rect_json, int *cx, int *cy) {
    if (!rect_json || strstr(rect_json, "\"error\"")) {
        g_free(rect_json);
        return 0;
    }
    JsonParser *parser = json_parser_new();
    if (json_parser_load_from_data(parser, rect_json, -1, NULL)) {
        JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
        *cx = (int)json_object_get_int_member(obj, "x");
        *cy = (int)json_object_get_int_member(obj, "y");
        g_object_unref(parser);
        g_free(rect_json);
        return 1;
    }
    g_object_unref(parser);
    g_free(rect_json);
    return 0;
}

#endif /* CMD_UTILS_H */
