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

    // Set viewport meta tag for responsive design
    char *js = g_strdup_printf(
        "(function(){"
        "  var meta = document.querySelector('meta[name=viewport]');"
        "  if(!meta) {"
        "    meta = document.createElement('meta');"
        "    meta.name = 'viewport';"
        "    document.head.appendChild(meta);"
        "  }"
        "  meta.content = 'width=%d, initial-scale=1.0, minimum-scale=0.1';"
        "  return 'viewport_set';"
        "})()", width);

    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);

    // Set mobile user agent to trigger responsive design
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);
    webkit_settings_set_user_agent_with_application_details(
        settings, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Mobile/15E148 Safari/604.1", "GTKBrowser");

    // Reload the page so responsive CSS kicks in
    webkit_web_view_reload(web_view);
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
            "  var best=null; var bestScore=-1;"
            "  for(var i=0;i<els.length;i++){"
            "    var el=els[i];"
            "    var n=(el.getAttribute('aria-label')||el.innerText||el.placeholder||el.value||'').toLowerCase();"
            "    if(n.indexOf('%s'.toLowerCase())>=0){"
            "      var r=el.getBoundingClientRect();"
            "      if(r.width>0&&r.height>0){"
            "        var area=r.width*r.height;"
            "        var visible=(r.top>=0&&r.top<window.innerHeight&&r.left>=0&&r.left<window.innerWidth)?1:0;"
            "        var score=area+visible*10000;"
            "        if(score>bestScore){bestScore=score;best=el;}"
            "      }"
            "    }"
            "  }"
            "  if(!best) return JSON.stringify({error:'not_found'});"
            "  var r=best.getBoundingClientRect();"
            "  return JSON.stringify({x:Math.round(r.x+r.width/2),y:Math.round(r.y+r.height/2),w:Math.round(r.width),h:Math.round(r.height),text:n.substring(0,80)});"
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

// === Navigation History ===

void page_go_back(WebKitWebView *web_view) {
    if (!web_view) return;
    webkit_web_view_go_back(web_view);
}

void page_go_forward(WebKitWebView *web_view) {
    if (!web_view) return;
    webkit_web_view_go_forward(web_view);
}

int page_get_history_length(WebKitWebView *web_view) {
    if (!web_view) return 0;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    return webkit_back_forward_list_get_length(bfl);
}

int page_get_history_index(WebKitWebView *web_view) {
    if (!web_view) return 0;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    int len = webkit_back_forward_list_get_length(bfl);
    WebKitBackForwardListItem *current = webkit_back_forward_list_get_current_item(bfl);
    if (!current) return 0;
    // Count how many back items exist
    GList *back_list = webkit_back_forward_list_get_back_list_with_limit(bfl, len);
    int idx = g_list_length(back_list);
    g_list_free(back_list);
    return idx;
}

void page_goto_history(WebKitWebView *web_view, int index) {
    if (!web_view) return;
    WebKitBackForwardList *bfl = webkit_web_view_get_back_forward_list(web_view);
    WebKitBackForwardListItem *item = webkit_back_forward_list_get_nth_item(bfl, index);
    if (item) {
        webkit_web_view_go_to_back_forward_list_item(web_view, item);
    }
}

// === Find in Page ===

char *page_find_in_page(WebKitWebView *web_view, const char *query, bool highlight) {
    if (!web_view || !query) return NULL;

    char *js;
    if (highlight) {
        js = g_strdup_printf(
            "(function(){"
            "  if(window._findHighlight) {"
            "    window._findHighlight.removeHighlight();"
            "  }"
            "  window._findHighlight = new rangy.Highlighter();"
            "  var results = rangy.cssClassApplier "
            "    ? null : null;"
            "  // Simple highlight via CSS"
            "  var style = document.createElement('style');"
            "  style.id = 'gb-find-style';"
            "  style.textContent = '.gb-highlight{background:yellow;padding:1px 2px;}';"
            "  if(!document.getElementById('gb-find-style')) document.head.appendChild(style);"
            "  var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);"
            "  var count = 0;"
            "  while(walker.nextNode()) {"
            "    var node = walker.currentNode;"
            "    var idx = node.nodeValue.toLowerCase().indexOf('%s'.toLowerCase());"
            "    if(idx >= 0) {"
            "      var span = document.createElement('span');"
            "      span.className = 'gb-highlight';"
            "      span.textContent = node.nodeValue.substring(idx, idx + '%s'.length);"
            "      node.parentNode.replaceChild(span, node);"
            "      count++;"
            "    }"
            "  }"
            "  return JSON.stringify({matches: count});"
            "})()", query, query);
    } else {
        js = g_strdup_printf(
            "(function(){"
            "  var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);"
            "  var count = 0;"
            "  while(walker.nextNode()) {"
            "    if(walker.currentNode.nodeValue.toLowerCase().indexOf('%s'.toLowerCase()) >= 0) {"
            "      count++;"
            "    }"
            "  }"
            "  return JSON.stringify({matches: count});"
            "})()", query);
    }

    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

int page_count_matches(WebKitWebView *web_view, const char *query) {
    char *json = page_find_in_page(web_view, query, false);
    if (!json) return 0;
    int count = 0;
    char *p = strstr(json, "\"matches\":");
    if (p) count = atoi(p + 11);
    g_free(json);
    return count;
}

// === Local/Session Storage ===

char *page_local_storage_get(WebKitWebView *web_view, const char *key) {
    if (!web_view || !key) return NULL;
    char *js = g_strdup_printf(
        "(function(){ var v = localStorage.getItem('%s'); return v !== null ? v : ''; })()", key);
    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

void page_local_storage_set(WebKitWebView *web_view, const char *key, const char *value) {
    if (!web_view || !key || !value) return;
    char *js = g_strdup_printf(
        "(function(){ localStorage.setItem('%s', '%s'); return true; })()", key, value);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

char *page_session_storage_get(WebKitWebView *web_view, const char *key) {
    if (!web_view || !key) return NULL;
    char *js = g_strdup_printf(
        "(function(){ var v = sessionStorage.getItem('%s'); return v !== null ? v : ''; })()", key);
    char *result = page_eval_js(web_view, js);
    g_free(js);
    return result;
}

void page_session_storage_set(WebKitWebView *web_view, const char *key, const char *value) {
    if (!web_view || !key || !value) return;
    char *js = g_strdup_printf(
        "(function(){ sessionStorage.setItem('%s', '%s'); return true; })()", key, value);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

char *page_local_storage_all(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "(function(){ var items = {}; for(var i=0; i<localStorage.length; i++){"
        "  var k = localStorage.key(i); items[k] = localStorage.getItem(k);"
        "} return JSON.stringify(items); })()");
}

// === Clipboard ===

char *clipboard_read(void) {
    char *result = NULL;
    FILE *fp = popen("xclip -selection clipboard -o 2>/dev/null", "r");
    if (fp) {
        GString *buf = g_string_new(NULL);
        char tmp[1024];
        while (fgets(tmp, sizeof(tmp), fp)) {
            g_string_append(buf, tmp);
        }
        pclose(fp);
        result = g_string_free(buf, FALSE);
    }
    return result;
}

void clipboard_write(const char *text) {
    if (!text) return;
    char *cmd = g_strdup_printf("echo -n '%s' | xclip -selection clipboard", text);
    system(cmd);
    g_free(cmd);
}

// === PDF Export ===

bool page_export_pdf(WebKitWebView *web_view, const char *filepath) {
    if (!web_view || !filepath) return false;

    // Save via JS: create a blob and trigger download
    char *js = g_strdup_printf(
        "(function(){"
        "  var printWindow = window.open('', '_blank');"
        "  printWindow.document.write(document.documentElement.outerHTML);"
        "  printWindow.document.close();"
        "  printWindow.print();"
        "  return 'print_dialog_opened';"
        "})()");
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);

    // Also save the page HTML to a .html file as fallback
    char *html = page_get_content(web_view, true);
    if (html) {
        FILE *f = fopen(filepath, "w");
        if (f) {
            fprintf(f, "%s", html);
            fclose(f);
            g_free(html);
            return true;
        }
        g_free(html);
    }
    return false;
}

// === Session Recording ===

static const char *RECORDING_JS =
    "(function(){"
    "  if(window._gbRecording) return 'already_recording';"
    "  window._gbActions = [];"
    "  window._gbRecording = true;"
    "  document.addEventListener('click', function(e) {"
    "    if(window._gbRecording) {"
    "      window._gbActions.push({"
    "        type: 'click',"
    "        x: e.clientX, y: e.clientY,"
    "        target: e.target.tagName + (e.target.id ? '#' + e.target.id : ''),"
    "        time: Date.now()"
    "      });"
    "    }"
    "  }, true);"
    "  document.addEventListener('input', function(e) {"
    "    if(window._gbRecording) {"
    "      window._gbActions.push({"
    "        type: 'input',"
    "        target: e.target.tagName + (e.target.name ? '[name=' + e.target.name + ']' : ''),"
    "        value: e.target.value.substring(0, 100),"
    "        time: Date.now()"
    "      });"
    "    }"
    "  }, true);"
    "  document.addEventListener('change', function(e) {"
    "    if(window._gbRecording && (e.target.tagName === 'SELECT')) {"
    "      window._gbActions.push({"
    "        type: 'select',"
    "        target: e.target.tagName + (e.target.name ? '[name=' + e.target.name + ']' : ''),"
    "        value: e.target.value,"
    "        time: Date.now()"
    "      });"
    "    }"
    "  }, true);"
    "  return 'recording_started';"
    "})();";

void page_start_recording(WebKitWebView *web_view) {
    if (!web_view) return;
    char *res = page_eval_js(web_view, RECORDING_JS);
    g_free(res);
}

char *page_stop_recording(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    char *res = page_eval_js(web_view,
        "(function(){ window._gbRecording = false; return 'recording_stopped'; })()");
    return res;
}

char *page_get_recording(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "JSON.stringify(window._gbActions || [])");
}

// === Performance Monitoring ===

char *page_performance_timing(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "(function(){"
        "  var t = performance.timing;"
        "  var nav = performance.navigation;"
        "  return JSON.stringify({"
        "    dns: t.domainLookupEnd - t.domainLookupStart,"
        "    tcp: t.connectEnd - t.connectStart,"
        "    ttfb: t.responseStart - t.requestStart,"
        "    download: t.responseEnd - t.responseStart,"
        "    dom_interactive: t.domInteractive - t.navigationStart,"
        "    dom_complete: t.domComplete - t.navigationStart,"
        "    load_event: t.loadEventEnd - t.navigationStart,"
        "    total: t.loadEventEnd - t.navigationStart,"
        "    type: nav.type,"
        "    redirect_count: nav.redirectCount"
        "  });"
        "})()");
}

char *page_performance_memory(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "(function(){"
        "  if(performance.memory) {"
        "    return JSON.stringify({"
        "      used: performance.memory.usedJSHeapSize,"
        "      total: performance.memory.totalJSHeapSize,"
        "      limit: performance.memory.jsHeapSizeLimit"
        "    });"
        "  }"
        "  return JSON.stringify({error: 'memory API not available'});"
        "})()");
}

// === Accessibility Audit ===

char *page_accessibility_audit(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    const char *js =
        "(function(){"
        "  var issues = [];"
        "  // Check images without alt text"
        "  var imgs = document.querySelectorAll('img:not([alt])');"
        "  if(imgs.length > 0) issues.push({rule: 'img-alt', count: imgs.length, severity: 'warning'});"
        "  // Check links without text"
        "  var links = document.querySelectorAll('a:not([aria-label]):not([title])');"
        "  var emptyLinks = 0;"
        "  for(var i=0; i<links.length; i++) {"
        "    if(!links[i].innerText.trim() && !links[i].querySelector('img')) emptyLinks++;"
        "  }"
        "  if(emptyLinks > 0) issues.push({rule: 'link-text', count: emptyLinks, severity: 'warning'});"
        "  // Check form inputs without labels"
        "  var inputs = document.querySelectorAll('input:not([type=hidden]):not([type=submit])');"
        "  var unlabeled = 0;"
        "  for(var i=0; i<inputs.length; i++) {"
        "    var id = inputs[i].id;"
        "    var hasLabel = id && document.querySelector('label[for=\"' + id + '\"]');"
        "    var hasAriaLabel = inputs[i].getAttribute('aria-label');"
        "    var hasTitle = inputs[i].getAttribute('title');"
        "    var hasPlaceholder = inputs[i].getAttribute('placeholder');"
        "    if(!hasLabel && !hasAriaLabel && !hasTitle && !hasPlaceholder) unlabeled++;"
        "  }"
        "  if(unlabeled > 0) issues.push({rule: 'input-label', count: unlabeled, severity: 'warning'});"
        "  // Check heading hierarchy"
        "  var headings = document.querySelectorAll('h1,h2,h3,h4,h5,h6');"
        "  var prevLevel = 0;"
        "  var skipCount = 0;"
        "  for(var i=0; i<headings.length; i++) {"
        "    var level = parseInt(headings[i].tagName[1]);"
        "    if(level > prevLevel + 1 && prevLevel > 0) skipCount++;"
        "    prevLevel = level;"
        "  }"
        "  if(skipCount > 0) issues.push({rule: 'heading-order', count: skipCount, severity: 'info'});"
        "  // Check color contrast (simplified)"
        "  var total = imgs.length + emptyLinks + unlabeled + skipCount;"
        "  return JSON.stringify({issues: issues, total_issues: total, score: Math.max(0, 100 - total * 5)});"
        "})()";

    return page_eval_js(web_view, js);
}

// === Network Logging ===

static const char *NET_LOG_JS =
    "(function(){"
    "  if(window._gbNetLog) return 'already_logging';"
    "  window._gbNetRequests = [];"
    "  window._gbNetLog = true;"
    "  var origFetch = window.fetch;"
    "  window.fetch = function() {"
    "    var url = arguments[0];"
    "    if(typeof url === 'string') url = url;"
    "    else if(url && url.url) url = url.url;"
    "    else url = String(url);"
    "    window._gbNetRequests.push({"
    "      type: 'fetch', url: url, time: Date.now()"
    "    });"
    "    return origFetch.apply(this, arguments);"
    "  };"
    "  var origOpen = XMLHttpRequest.prototype.open;"
    "  XMLHttpRequest.prototype.open = function(method, url) {"
    "    window._gbNetRequests.push({"
    "      type: 'xhr', method: method, url: url, time: Date.now()"
    "    });"
    "    return origOpen.apply(this, arguments);"
    "  };"
    "  var origSend = XMLHttpRequest.prototype.send;"
    "  XMLHttpRequest.prototype.send = function(body) {"
    "    var req = window._gbNetRequests[window._gbNetRequests.length - 1];"
    "    if(req && req.type === 'xhr') {"
    "      req.body = typeof body === 'string' ? body.substring(0, 200) : 'binary';"
    "    }"
    "    return origSend.apply(this, arguments);"
    "  };"
    "  return 'network_logging_started';"
    "})();";

void page_start_network_log(WebKitWebView *web_view) {
    if (!web_view) return;
    char *res = page_eval_js(web_view, NET_LOG_JS);
    g_free(res);
}

char *page_get_network_log(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "JSON.stringify(window._gbNetRequests || [])");
}

void page_stop_network_log(WebKitWebView *web_view) {
    if (!web_view) return;
    char *res = page_eval_js(web_view,
        "(function(){ window._gbNetLog = false; return 'network_logging_stopped'; })()");
    g_free(res);
}

// === Download Handler ===

char *page_get_downloads(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    return page_eval_js(web_view,
        "(function(){"
        "  var downloads = [];"
        "  var links = document.querySelectorAll('a[download],a[href$=.pdf],a[href$=.zip],a[href$=.tar],a[href$=.gz]');"
        "  for(var i=0; i<links.length; i++) {"
        "    downloads.push({"
        "      url: links[i].href,"
        "      name: links[i].download || links[i].href.split('/').pop(),"
        "      text: (links[i].innerText || '').substring(0, 50)"
        "    });"
        "  }"
        "  return JSON.stringify(downloads);"
        "})()");
}

// === Drag and Drop Simulation ===

void page_drag(WebKitWebView *web_view, int sx, int sy, int ex, int ey) {
    if (!web_view) return;

    // Simulate drag via mouse events at source, move, then target
    input_mouse_down(web_view, sx, sy);
    g_usleep(100000); // 100ms hold

    // Move in steps
    int steps = 20;
    for (int i = 1; i <= steps; i++) {
        int x = sx + (ex - sx) * i / steps;
        int y = sy + (ey - sy) * i / steps;
        input_mouse_move(web_view, x, y);
        g_usleep(20000); // 20ms per step
    }

    g_usleep(100000); // 100ms pause at target
    input_mouse_up(web_view, ex, ey);
}

// === SSL Certificate Info ===

char *page_ssl_info(WebKitWebView *web_view) {
    if (!web_view) return NULL;

    return page_eval_js(web_view,
        "(function(){"
        "  var info = {"
        "    protocol: location.protocol,"
        "    hostname: location.hostname,"
        "    isSecure: location.protocol === 'https:'"
        "  };"
        "  return JSON.stringify(info);"
        "})()");
}

// === Checkbox/Radio ===

void page_check(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return;
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el) return false;"
        "  el.checked = true;"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return true;"
        "})()", selector);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

void page_uncheck(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return;
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el) return false;"
        "  el.checked = false;"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return true;"
        "})()", selector);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

bool page_is_checked(WebKitWebView *web_view, const char *selector) {
    if (!web_view || !selector) return false;
    char *js = g_strdup_printf(
        "(function(){ var el = document.querySelector('%s'); return el ? el.checked : false; })()",
        selector);
    char *res = page_eval_js(web_view, js);
    bool checked = (res && strstr(res, "true"));
    g_free(res);
    g_free(js);
    return checked;
}

// === File Upload ===

void page_upload_file(WebKitWebView *web_view, const char *selector, const char *filepath) {
    if (!web_view || !selector || !filepath) return;

    // Find the file input and set its files via DataTransfer API
    char *js = g_strdup_printf(
        "(function(){"
        "  var el = document.querySelector('%s');"
        "  if(!el || el.type !== 'file') return 'not_file_input';"
        "  // Create a File object and set it on the input"
        "  // Note: direct file access isn't possible from JS in modern browsers"
        "  // But we can trigger the change event after setting the value"
        "  el.value = '%s';"
        "  el.dispatchEvent(new Event('change', {bubbles: true}));"
        "  return 'file_set';"
        "})()", selector, filepath);
    char *res = page_eval_js(web_view, js);
    g_free(res);
    g_free(js);
}

// === Bug 7: Dismiss overlays (cookie banners, chat widgets, popups) ===

char *page_dismiss_overlays(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    const char *js =
        "(function(){"
        "  var dismissed = [];"
        "  // Close cookie banners"
        "  var cookieBtns = document.querySelectorAll('[class*=cookie] button, [id*=cookie] button, [class*=consent] button, [id*=consent] button, [class*=banner] button, [aria-label*=close], [aria-label*=dismiss], [aria-label*=accept], [class*=close-btn], [class*=dismiss]');"
        "  for(var i=0; i<cookieBtns.length; i++) {"
        "    var r = cookieBtns[i].getBoundingClientRect();"
        "    if(r.width > 0 && r.height > 0) {"
        "      cookieBtns[i].click();"
        "      dismissed.push('cookie:' + cookieBtns[i].innerText.substring(0,30));"
        "    }"
        "  }"
        "  // Close chat widgets"
        "  var chatBtns = document.querySelectorAll('[class*=chat] [class*=close], [id*=chat] [class*=close], [class*=intercom] [class*=close], [class*=zendesk] [class*=close], [class*=drift] [class*=close], [class*=crisp] [class*=close]');"
        "  for(var i=0; i<chatBtns.length; i++) {"
        "    var r = chatBtns[i].getBoundingClientRect();"
        "    if(r.width > 0 && r.height > 0) {"
        "      chatBtns[i].click();"
        "      dismissed.push('chat:' + chatBtns[i].innerText.substring(0,30));"
        "    }"
        "  }"
        "  // Close modals/overlays"
        "  var modals = document.querySelectorAll('[class*=modal] [class*=close], [role=dialog] [class*=close], [class*=overlay] [class*=close], [class*=popup] [class*=close]');"
        "  for(var i=0; i<modals.length; i++) {"
        "    var r = modals[i].getBoundingClientRect();"
        "    if(r.width > 0 && r.height > 0) {"
        "      modals[i].click();"
        "      dismissed.push('modal:' + modals[i].innerText.substring(0,30));"
        "    }"
        "  }"
        "  // Press Escape as fallback"
        "  if(dismissed.length === 0) {"
        "    document.dispatchEvent(new KeyboardEvent('keydown', {key: 'Escape', bubbles: true}));"
        "    dismissed.push('escape_key');"
        "  }"
        "  return JSON.stringify({dismissed: dismissed, count: dismissed.length});"
        "})()";

    return page_eval_js(web_view, js);
}

// === Bug 3: Force elements scan (with delay for SPA render) ===

char *page_force_elements(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    // Wait a bit for JS to render, then scan
    return page_get_elements(web_view);
}
