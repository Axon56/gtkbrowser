#include "monitoring.h"
#include "page.h"
#include <string.h>
#include <stdlib.h>
#include <json-glib/json-glib.h>

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
        "    var hasLabel = id && document.querySelector('label[for=\\\"' + id + '\\\"]');"
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
