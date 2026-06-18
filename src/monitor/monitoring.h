#ifndef MONITORING_H
#define MONITORING_H

#include <webkit2/webkit2.h>

// Performance monitoring
char *page_performance_timing(WebKitWebView *web_view);
char *page_performance_memory(WebKitWebView *web_view);

// Accessibility audit
char *page_accessibility_audit(WebKitWebView *web_view);

// Network logging
void page_start_network_log(WebKitWebView *web_view);
char *page_get_network_log(WebKitWebView *web_view);
void page_stop_network_log(WebKitWebView *web_view);

// Download handler
char *page_get_downloads(WebKitWebView *web_view);

// SSL certificate info
char *page_ssl_info(WebKitWebView *web_view);

#endif // MONITORING_H
