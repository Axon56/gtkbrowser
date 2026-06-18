#include "elements.h"
#include "page.h"
#include "input/input.h"
#include <string.h>
#include <stdlib.h>
#include <json-glib/json-glib.h>

// === Find element by selector ===

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

// === Get all interactive elements ===

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
        "txt = txt.substring(0,80).replace(/\\\\n/g,' ');";
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

// === Accessibility Tree ===

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
        "      name = name.substring(0,100).replace(/\\\\n/g,' '); "
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

// === Find by accessibility role ===

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

// === Dismiss overlays (cookie banners, chat widgets, popups) ===

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

// === Force elements scan (with delay for SPA render) ===

char *page_force_elements(WebKitWebView *web_view) {
    if (!web_view) return NULL;
    // Wait a bit for JS to render, then scan
    return page_get_elements(web_view);
}
