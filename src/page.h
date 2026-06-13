#ifndef PAGE_H
#define PAGE_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Get page content (innerHTML or outerHTML)
char *page_get_content(WebKitWebView *web_view, bool outer);

// Get page text content (innerText equivalent)
char *page_get_text(WebKitWebView *web_view);

// Execute JavaScript in the page context
char *page_eval_js(WebKitWebView *web_view, const char *script);

// Find element by selector and return its bounding rect as JSON
char *page_find_element(WebKitWebView *web_view, const char *selector);

// Get accessibility tree as JSON
char *page_get_accessibility_tree(WebKitWebView *web_view);

// Wait for element to appear
int page_wait_for(WebKitWebView *web_view, const char *selector, int timeout_ms);

// Wait for navigation to complete
int page_wait_for_load(WebKitWebView *web_view, int timeout_ms);

// Get all interactive elements as JSON array
char *page_get_elements(WebKitWebView *web_view);

// Set viewport size
void page_set_viewport(WebKitWebView *web_view, int width, int height);

// Read text/value of a specific element
char *page_read_element(WebKitWebView *web_view, const char *selector, bool read_value);

// Count elements matching selector
char *page_count_elements(WebKitWebView *web_view, const char *selector);

// Get formatted accessibility tree (human-readable)
char *page_inspect(WebKitWebView *web_view);

// Wait for text to appear/disappear on page
int page_wait_for_text(WebKitWebView *web_view, const char *text, bool disappear, int timeout_ms);

// Wait for URL to contain string
int page_wait_for_url(WebKitWebView *web_view, const char *url_part, int timeout_ms);

// Wait for element to have a specific state
int page_wait_for_state(WebKitWebView *web_view, const char *selector, const char *state, int timeout_ms);

// Find element by accessibility role selector (e.g. "Button:Submit")
char *page_find_role_element(WebKitWebView *web_view, const char *role_selector);

// Click element by accessibility role selector
int page_click_role(WebKitWebView *web_view, const char *role_selector);

// Type into element by accessibility role selector
int page_type_role(WebKitWebView *web_view, const char *role_selector, const char *text);

// Handle next dialog
char *page_handle_dialog(WebKitWebView *web_view, const char *action, const char *value);

// Get page title
char *page_get_title(WebKitWebView *web_view);

// Get iframes list
char *page_get_frames(WebKitWebView *web_view);

// Find element with nth match (0-based)
char *page_find_nth(WebKitWebView *web_view, const char *selector, int nth);

// Click nth element
int page_click_nth(WebKitWebView *web_view, const char *selector, int nth);

// Dialog management (JS-based)
void page_inject_dialog_handler(WebKitWebView *web_view);
char *page_get_dialogs(WebKitWebView *web_view);
char *page_clear_dialogs(WebKitWebView *web_view);
void page_set_dialog_auto(WebKitWebView *web_view, bool auto_accept, const char *prompt_value);

#endif // PAGE_H

// Navigation history
void page_go_back(WebKitWebView *web_view);
void page_go_forward(WebKitWebView *web_view);
int page_get_history_length(WebKitWebView *web_view);
int page_get_history_index(WebKitWebView *web_view);

// Find in page
char *page_find_in_page(WebKitWebView *web_view, const char *query, bool highlight);
int page_count_matches(WebKitWebView *web_view, const char *query);

// Local/Session storage
char *page_local_storage_get(WebKitWebView *web_view, const char *key);
void page_local_storage_set(WebKitWebView *web_view, const char *key, const char *value);
char *page_session_storage_get(WebKitWebView *web_view, const char *key);
void page_session_storage_set(WebKitWebView *web_view, const char *key, const char *value);
char *page_local_storage_all(WebKitWebView *web_view);

// Clipboard
char *clipboard_read(void);
void clipboard_write(const char *text);

// PDF export
bool page_export_pdf(WebKitWebView *web_view, const char *filepath);

// Navigation
void page_goto_history(WebKitWebView *web_view, int index);

// Session recording
void page_start_recording(WebKitWebView *web_view);
char *page_stop_recording(WebKitWebView *web_view);
char *page_get_recording(WebKitWebView *web_view);

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

// Drag and drop simulation
void page_drag(WebKitWebView *web_view, int sx, int sy, int ex, int ey);

// SSL certificate info
char *page_ssl_info(WebKitWebView *web_view);

// Checkbox/Radio
void page_check(WebKitWebView *web_view, const char *selector);
void page_uncheck(WebKitWebView *web_view, const char *selector);
bool page_is_checked(WebKitWebView *web_view, const char *selector);

// File upload
void page_upload_file(WebKitWebView *web_view, const char *selector, const char *filepath);
