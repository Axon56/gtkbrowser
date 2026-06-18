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
        "    var name = getName(el).substring(0,80).replace(/\\\\n/g,' ');"
        "    items.push(role + ': ' + (name || '(unnamed)'));"
        "  }"
        "  return items.join('\\\\n');"
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
