#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <webkit2/webkit2.h>
#include <stdbool.h>

// Schedule a full-page screenshot (deferred, non-blocking)
bool screenshot_schedule(WebKitWebView *web_view, const char *filepath);

// Take a synchronous screenshot (blocks until done)
bool screenshot_sync(WebKitWebView *web_view, const char *filepath);

// Schedule a screenshot with explicit region
bool screenshot_schedule_region(WebKitWebView *web_view,
                                 const char *filepath,
                                 WebKitSnapshotRegion region);

// Schedule a screenshot of a specific element (scrolls into view, captures viewport)
bool screenshot_schedule_element(WebKitWebView *web_view,
                                  const char *selector,
                                  const char *filepath);

// Check if a screenshot is complete and get the result
// Returns: 0 = still pending, 1 = success, -1 = failed
int screenshot_check_result(void);

// Get the result path (caller must free)
char *screenshot_get_result_path(void);

#endif // SCREENSHOT_H
