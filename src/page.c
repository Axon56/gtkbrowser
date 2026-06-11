#include "page.h"
#include "input.h"
#include <string.h>
#include <stdlib.h>
#include <json-glib/json-glib.h>

static char *page_json_result(const char *key, const char *value) {
    JsonBuilder *b = json_builder_new();
    json_builder_begin_object(b);
    json_builder_set_member_name(b, key);
    json_builder_add_string_value(b, value ? value : "");
    json_builder_end_object(b);
    JsonGenerator *g = json_generator_new();
    json_generator_set_root(g, json_builder_get_root(b));
    char *r = json_generator_to_data(g, NULL);
    g_object_unref(g);
    g_object_unref(b);
    return r;
}

// Helper struct for async JS evaluation
typedef struct {
    char *result;
    bool done;
} JsEvalData;

// Callback must match GAsyncReadyCallback: (GObject*, GAsyncResult*, gpointer)
static void js_eval_finished(GObject *source_object,
                              GAsyncResult *result,
                              gpointer user_data) {
    (void)source_object;
    JsEvalData *data = (JsEvalData *)user_data;

    GError *error = NULL;
    JSCValue *js_value = webkit_web_view_evaluate_javascript_finish(
        WEBKIT_WEB_VIEW(source_object), result, &error);

    if (js_value) {
        char *str_value = jsc_value_to_json(js_value, 0);
        data->result = str_value;
        g_object_unref(js_value);
    } else if (error) {
        data->result = g_strdup("");
        g_error_free(error);
    }

    data->done = true;
}

char *page_eval_js(WebKitWebView *web_view, const char *script) {
    if (!web_view || !script) return NULL;

    JsEvalData data = { .result = NULL, .done = false };

    webkit_web_view_evaluate_javascript(
        web_view,
        script,
        -1,              // length (-1 = null-terminated)
        NULL,            // world_name (NULL = default)
        NULL,            // source_uri
        NULL,            // cancellable
        js_eval_finished,
        &data);

    // Spin the GTK main loop until JS evaluation completes
    while (!data.done) {
        gtk_main_iteration_do(FALSE);
        g_usleep(1000); // 1ms
    }

    return data.result;
}

char *page_get_content(WebKitWebView *web_view, bool outer) {
    if (!web_view) return NULL;

    const char *js = outer
        ? "document.documentElement.outerHTML"
        : "document.documentElement.innerHTML";

    return page_eval_js(web_view, js);
}

char *page_get_text(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view, "document.body.innerText");
}

char *page_find_element(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return NULL;

    char *js = g_strdup_printf(
        "(function(){ "
        "  var el = document.querySelector('%s'); "
        "  if(!el) return JSON.stringify({error:'not_found'}); "
        "  var r = el.getBoundingClientRect(); "
        "  return JSON.stringify({"
        "    x: Math.round(r.x + r.width/2),"
        "    y: Math.round(r.y + r.height/2),"
        "    width: Math.round(r.width),"
        "    height: Math.round(r.height),"
        "    left: Math.round(r.x),"
        "    top: Math.round(r.y),"
        "    tag: el.tagName,"
        "    text: (el.innerText||'').substring(0,200),"
        "    visible: r.width > 0 && r.height > 0"
        "  }); "
        "})()", selector);

    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

char *page_get_accessibility_tree(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    const char *js =
        "(function(){ "
        "  function walk(el, depth) { "
        "    if(depth > 5) return []; "
        "    var items = []; "
        "    var children = el.children; "
        "    for(var i = 0; i < children.length && i < 50; i++) { "
        "      var c = children[i]; "
        "      var role = c.getAttribute('role') || c.tagName.toLowerCase(); "
        "      var name = c.getAttribute('aria-label') || c.getAttribute('title') || "
        "                 c.getAttribute('placeholder') || c.innerText || ''; "
        "      name = name.substring(0,100).replace(/\\n/g,' '); "
        "      var r = c.getBoundingClientRect(); "
        "      items.push({ "
        "        role: role, "
        "        name: name, "
        "        x: Math.round(r.x), y: Math.round(r.y), "
        "        w: Math.round(r.width), h: Math.round(r.height), "
        "        children: walk(c, depth+1) "
        "      }); "
        "    } "
        "    return items; "
        "  } "
        "  return JSON.stringify(walk(document.body, 0)); "
        "})()";

    return page_eval_js(web_view, js);
}

int page_wait_for(WebKitWebView *web_view, const char *selector,
                   int timeout_ms) {
    if (!web_view || !selector) return -1;

    int elapsed = 0;
    int interval = 100;

    while (elapsed < timeout_ms) {
        char *js = g_strdup_printf(
            "document.querySelector('%s') !== null", selector);

        char *result = page_eval_js(web_view, js);
        g_free(js);

        if (result && strstr(result, "true")) {
            g_free(result);
            return 0;
        }
        g_free(result);

        g_usleep(interval * 1000);
        elapsed += interval;
    }

    return -1;
}

int page_wait_for_load(WebKitWebView *web_view, int timeout_ms) {
    if (!web_view) return -1;

    int elapsed = 0;
    int interval = 200;

    while (elapsed < timeout_ms) {
        // Use is_loading — when it returns FALSE, page is done
        if (!webkit_web_view_is_loading(web_view)) {
            return 0;
        }

        g_usleep(interval * 1000);
        elapsed += interval;
    }

    return -1;
}

char *page_get_elements(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    const char *js =
        "(function(){";
        "var items = [];";
        "var seen = {};";
        "function scan(root, prefix) {";
        "var all = root.querySelectorAll('a,button,input,textarea,select,[role],[tabindex],[onclick],[href]');";
        "for(var i=0; i<all.length && i<300; i++){";
        "var el = all[i];";
        "var r = el.getBoundingClientRect();";
        "if(r.width===0 && r.height===0) continue;";
        "var sel = '';";
        "if(el.id) sel = '#' + el.id;";
        "else if(el.name) sel = el.tagName.toLowerCase() + '[name=' + el.name + ']';";
        "else {";
        "var idx = 0; var sib = el;";
        "while(sib.previousElementSibling){sib=sib.previousElementSibling;idx++;}";
        "sel = prefix + el.tagName.toLowerCase() + ':nth-of-type(' + (idx+1) + ')';";
        "}";
        "if(seen[sel]) continue; seen[sel] = true;";
        "var txt = (el.innerText || el.value || el.placeholder || el.getAttribute('aria-label') || '');";
        "txt = txt.substring(0,80).replace(/\\n/g,' ');";
        "items.push({s:sel, t:el.tagName.toLowerCase(),";
        "x:Math.round(r.x+r.width/2), y:Math.round(r.y+r.height/2),";
        "w:Math.round(r.width), h:Math.round(r.height), v:txt});";
        "}";
        "// Shadow DOM traversal";
        "var allEls = root.querySelectorAll('*');";
        "for(var j=0; j<allEls.length; j++) {";
        "  var sh = allEls[j].shadowRoot;";
        "  if(sh) scan(sh, prefix + '>> ');";
        "}";
        "}";
        "scan(document, '');";
        "return JSON.stringify(items);";
        "})();";

    return page_eval_js(web_view, js);
}

void page_set_viewport(WebKitWebView *web_view, int width, int height) {
    if (!web_view) return;

    // Set the viewport meta tag via JavaScript
    char *js = g_strdup_printf(
        "(function(){"
        "  var meta = document.querySelector('meta[name=viewport]');"
        "  if(!meta) {"
        "    meta = document.createElement('meta');"
        "    meta.name = 'viewport';"
        "    document.head.appendChild(meta);"
        "  }"
        "  meta.content = 'width=%d, initial-scale=0.5';"
        "  return true;"
        "})()", width);

    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

char *page_read_element(WebKitWebView *web_view, const char *selector, bool read_value) {
    if (!web_view || !selector) return NULL;

    const char *prop = read_value ? "value" : "innerText";
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el) return JSON.stringify({error:'not_found'});"
        "  var val = el.%s || el.value || el.placeholder || '';"
        "  return JSON.stringify({text: val, tag: el.tagName});"
        "})()", selector, prop);

    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

char *page_count_elements(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return NULL;

    char *js = g_strdup_printf(
        "(function(){"
        "  var els = document.querySelectorAll('%s');"
        "  return JSON.stringify({count: els.length, selector: '%s'});"
        "})()", selector, selector);

    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

char *page_inspect(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    const char *js =
        "(function(){"
        "  function getRole(el) {"
        "    var tag = el.tagName.toLowerCase();"
        "    var role = el.getAttribute('role') || '';"
        "    if(role) return role;"
        "    if(tag==='button') return 'Push Button';"
        "    if(tag==='a') return 'Link';"
        "    if(tag==='input') return el.type==='submit'?'Push Button':el.type==='checkbox'?'Check Box':el.type==='radio'?'Radio Button':'Text Box';"
        "    if(tag==='textarea') return 'Text Box';"
        "    if(tag==='select') return 'Combo Box';"
        "    if(tag==='h1') return 'Heading';"
        "    if(tag==='h2') return 'Heading';"
        "    if(tag==='h3') return 'Heading';"
        "    if(tag==='img') return 'Image';"
        "    if(tag==='label') return 'Label';"
        "    return tag;"
        "  }"
        "  function getName(el) {"
        "    return el.getAttribute('aria-label') || el.getAttribute('title') || el.innerText || el.placeholder || el.value || '';"
        "  }"
        "  var items = [];"
        "  var all = document.querySelectorAll('a,button,input,textarea,select,h1,h2,h3,img,label,[role],[tabindex]');"
        "  for(var i=0; i<all.length && i<300; i++){"
        "    var el = all[i];"
        "    var r = el.getBoundingClientRect();"
        "    if(r.width===0 && r.height===0) continue;"
        "    var role = getRole(el);"
        "    var name = getName(el).substring(0,80).replace(/\\n/g,' ');"
        "    items.push(role + ': ' + (name || '(unnamed)'));"
        "  }"
        "  return items.join('\\n');"
        "})();";

    return page_eval_js(web_view, js);
}

int page_wait_for_text(WebKitWebView *web_view, const char *text, bool disappear, int timeout_ms) {
    if (!web_view || !text) return -1;

    int elapsed = 0;
    int interval = 200;

    while (elapsed < timeout_ms) {
        char *js = g_strdup_printf(
            "document.body.innerText.indexOf('%s') >= 0", text);

        char *result = page_eval_js(web_view, js);
        g_free(js);

        bool found = (result && strstr(result, "true"));
        g_free(result);

        if (disappear && !found) return 0;
        if (!disappear && found) return 0;

        g_usleep(interval * 1000);
        elapsed += interval;
    }

    return -1;
}

int page_wait_for_url(WebKitWebView *web_view, const char *url_part, int timeout_ms) {
    if (!web_view || !url_part) return -1;

    int elapsed = 0;
    int interval = 200;

    while (elapsed < timeout_ms) {
        char *js = g_strdup_printf(
            "window.location.href.indexOf('%s') >= 0", url_part);

        char *result = page_eval_js(web_view, js);
        g_free(js);

        bool found = (result && strstr(result, "true"));
        g_free(result);

        if (found) return 0;

        g_usleep(interval * 1000);
        elapsed += interval;
    }

    return -1;
}

int page_wait_for_state(WebKitWebView *web_view, const char *selector, const char *state, int timeout_ms) {
    if (!web_view || !selector || !state) return -1;

    int elapsed = 0;
    int interval = 200;

    while (elapsed < timeout_ms) {
        char *js = g_strdup_printf(
            "(function(){"
            "  var el = document.querySelector('%s');"
            "  if(!el) return false;"
            "  if('%s'==='focused') return document.activeElement===el;"
            "  if('%s'==='visible') { var r=el.getBoundingClientRect(); return r.width>0 && r.height>0; }"
            "  if('%s'==='hidden') { var r=el.getBoundingClientRect(); return r.width===0 || r.height===0; }"
            "  return false;"
            "})()", selector, state, state, state);

        char *result = page_eval_js(web_view, js);
        g_free(js);

        bool found = (result && strstr(result, "true"));
        g_free(result);

        if (found) return 0;

        g_usleep(interval * 1000);
        elapsed += interval;
    }

    return -1;
}

char *page_find_role_element(WebKitWebView *web_view, const char *role_selector) {
    if (!web_view || !role_selector) return NULL;

    // Parse "Role:Name" or just "Role"
    char *role = NULL;
    char *name = NULL;
    char *colon = strchr(role_selector, ':');
    if (colon) {
        role = g_strndup(role_selector, colon - role_selector);
        name = g_strdup(colon + 1);
    } else {
        role = g_strdup(role_selector);
    }

    // Map common role aliases
    char *mapped_role = NULL;
    if (g_ascii_strcasecmp(role, "Button") == 0 || g_ascii_strcasecmp(role, "Push Button") == 0) {
        mapped_role = g_strdup("button");
    } else if (g_ascii_strcasecmp(role, "Text Box") == 0 || g_ascii_strcasecmp(role, "Input") == 0 || g_ascii_strcasecmp(role, "Entry") == 0) {
        mapped_role = g_strdup("input,textarea");
    } else if (g_ascii_strcasecmp(role, "Link") == 0) {
        mapped_role = g_strdup("a");
    } else if (g_ascii_strcasecmp(role, "Check Box") == 0) {
        mapped_role = g_strdup("input[type=checkbox]");
    } else if (g_ascii_strcasecmp(role, "Radio Button") == 0) {
        mapped_role = g_strdup("input[type=radio]");
    } else if (g_ascii_strcasecmp(role, "Combo Box") == 0 || g_ascii_strcasecmp(role, "Select") == 0) {
        mapped_role = g_strdup("select");
    } else if (g_ascii_strcasecmp(role, "Heading") == 0) {
        mapped_role = g_strdup("h1,h2,h3,h4,h5,h6");
    } else if (g_ascii_strcasecmp(role, "Image") == 0) {
        mapped_role = g_strdup("img");
    } else if (g_ascii_strcasecmp(role, "Label") == 0) {
        mapped_role = g_strdup("label");
    } else {
        mapped_role = g_strdup_printf("[role='%s']", role);
    }

    // Build JS to find element
    char *js;
    if (name) {
        js = g_strdup_printf(
            "(function(){"
            "  var els = document.querySelectorAll('%s');"
            "  for(var i=0;i<els.length;i++){"
            "    var el=els[i];"
            "    var n=(el.getAttribute('aria-label')||el.innerText||el.placeholder||el.value||'').toLowerCase();"
            "    if(n.indexOf('%s'.toLowerCase())>=0){"
            "      var r=el.getBoundingClientRect();"
            "      if(r.width>0&&r.height>0) return JSON.stringify({x:Math.round(r.x+r.width/2),y:Math.round(r.y+r.height/2),w:Math.round(r.width),h:Math.round(r.height),text:n.substring(0,80)});"
            "    }"
            "  }"
            "  return JSON.stringify({error:'not_found'});"
            "})()", mapped_role, name);
    } else {
        js = g_strdup_printf(
            "(function(){"
            "  var el = document.querySelector('%s');"
            "  if(!el) return JSON.stringify({error:'not_found'});"
            "  var r=el.getBoundingClientRect();"
            "  return JSON.stringify({x:Math.round(r.x+r.width/2),y:Math.round(r.y+r.height/2),w:Math.round(r.width),h:Math.round(r.height)});"
            "})()", mapped_role);
    }

    char *result = page_eval_js(web_view, js);
    g_free(js);
    g_free(role);
    g_free(name);
    g_free(mapped_role);
    return result;
}

int page_click_role(WebKitWebView *web_view, const char *role_selector) {
    char *json = page_find_role_element(web_view, role_selector);
    if (!json || strstr(json, "\"error\"")) {
        g_free(json);
        return -1;
    }

    JsonParser *parser = json_parser_new();
    if (json_parser_load_from_data(parser, json, -1, NULL)) {
        JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
        int x = (int)json_object_get_int_member(obj, "x");
        int y = (int)json_object_get_int_member(obj, "y");
        input_click(web_view, x, y);
        g_object_unref(parser);
        g_free(json);
        return 0;
    }
    g_object_unref(parser);
    g_free(json);
    return -1;
}

int page_type_role(WebKitWebView *web_view, const char *role_selector, const char *text) {
    char *json = page_find_role_element(web_view, role_selector);
    if (!json || strstr(json, "\"error\"")) {
        g_free(json);
        return -1;
    }

    JsonParser *parser = json_parser_new();
    if (json_parser_load_from_data(parser, json, -1, NULL)) {
        JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
        int x = (int)json_object_get_int_member(obj, "x");
        int y = (int)json_object_get_int_member(obj, "y");
        input_click(web_view, x, y);
        g_usleep(100000); // 100ms after focus
        input_type_text(web_view, text);
        g_object_unref(parser);
        g_free(json);
        return 0;
    }
    g_object_unref(parser);
    g_free(json);
    return -1;
}

char *page_handle_dialog(WebKitWebView *web_view, const char *action, const char *value) {
    if (!web_view) return NULL;
    // WebKit handles dialogs via JS dialogs API
    // For now, we set up a handler that auto-accepts/dismisses
    // This requires WebKitSettings configuration
    (void)action;
    (void)value;
    return page_json_result("status", "dialog_handler_set");
}

char *page_get_title(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    const gchar *title = webkit_web_view_get_title(web_view);
    return page_json_result("title", title ? title : "");
}

char *page_get_frames(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    const char *js =
        "(function(){"
        "  var frames = document.querySelectorAll('iframe,frame');"
        "  var items = [];"
        "  for(var i=0;i<frames.length;i++){"
        "    var f=frames[i];"
        "    var r=f.getBoundingClientRect();"
        "    items.push({"
        "      index: i,"
        "      src: f.src||f.getAttribute('src')||'',"
        "      name: f.name||'',"
        "      id: f.id||'',"
        "      x: Math.round(r.x), y: Math.round(r.y),"
        "      w: Math.round(r.width), h: Math.round(r.height)"
        "    });"
        "  }"
        "  return JSON.stringify(items);"
        "})();";

    return page_eval_js(web_view, js);
}

char *page_find_nth(WebKitWebView *web_view, const char *selector, int nth) {
    if (!web_view || !selector) return NULL;

    char *js = g_strdup_printf(
        "(function(){"
        "  var els = document.querySelectorAll('%s');"
        "  if(els.length <= %d) return JSON.stringify({error:'not_found',count:els.length});"
        "  var el = els[%d];"
        "  var r = el.getBoundingClientRect();"
        "  return JSON.stringify({"
        "    x: Math.round(r.x + r.width/2),"
        "    y: Math.round(r.y + r.height/2),"
        "    width: Math.round(r.width),"
        "    height: Math.round(r.height),"
        "    left: Math.round(r.x),"
        "    top: Math.round(r.y),"
        "    tag: el.tagName,"
        "    text: (el.innerText||'').substring(0,200),"
        "    visible: r.width > 0 && r.height > 0,"
        "    index: %d,"
        "    total: els.length"
        "  });"
        "})()", selector, nth, nth, nth);

    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

int page_click_nth(WebKitWebView *web_view, const char *selector, int nth) {
    char *json = page_find_nth(web_view, selector, nth);
    if (!json || strstr(json, "\"error\"")) {
        g_free(json);
        return -1;
    }

    JsonParser *parser = json_parser_new();
    if (json_parser_load_from_data(parser, json, -1, NULL)) {
        JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
        int x = (int)json_object_get_int_member(obj, "x");
        int y = (int)json_object_get_int_member(obj, "y");
        input_click(web_view, x, y);
        g_object_unref(parser);
        g_free(json);
        return 0;
    }
    g_object_unref(parser);
    g_free(json);
    return -1;
}

// Dialog override - injects JS to capture and auto-handle alerts/confirms/prompts
static const char *DIALOG_OVERRIDE_JS =
    "(function(){"
    "  if(window.__gb_dialog_ready) return;"
    "  window.__gb_dialog_ready = true;"
    "  window.__gb_dialogs = [];"
    "  window.__gb_dialog_auto = true;"
    "  window.__gb_dialog_response = '';"
    "  var orig_alert = window.alert;"
    "  var orig_confirm = window.confirm;"
    "  var orig_prompt = window.prompt;"
    "  window.alert = function(msg) {"
    "    window.__gb_dialogs.push({type:'alert',message:String(msg),time:Date.now()});"
    "  };"
    "  window.confirm = function(msg) {"
    "    window.__gb_dialogs.push({type:'confirm',message:String(msg),time:Date.now()});"
    "    return window.__gb_dialog_auto;"
    "  };"
    "  window.prompt = function(msg, def) {"
    "    window.__gb_dialogs.push({type:'prompt',message:String(msg),default:def||'',time:Date.now()});"
    "    return window.__gb_dialog_response || def || '';"
    "  };"
    "  return true;"
    "})();";

void page_inject_dialog_handler(WebKitWebView *web_view) {
    if (!web_view) return;
    char *res = page_eval_js(web_view, DIALOG_OVERRIDE_JS);
    g_free(res);
}

char *page_get_dialogs(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view, "JSON.stringify(window.__gb_dialogs || [])");
}

char *page_clear_dialogs(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view, "(function(){ window.__gb_dialogs=[]; return 'cleared'; })()");
}

void page_set_dialog_auto(WebKitWebView *web_view, bool auto_accept, const char *prompt_value) {
    if (!web_view) return;
    char js[512];
    snprintf(js, sizeof(js),
        "window.__gb_dialog_auto = %s; window.__gb_dialog_response = '%s';",
        auto_accept ? "true" : "false",
        prompt_value ? prompt_value : "");
    char *res = page_eval_js(web_view, js);
    g_free(res);
}
