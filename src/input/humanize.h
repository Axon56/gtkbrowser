#ifndef HUMANIZE_H
#define HUMANIZE_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

// Humanization level: 0 = robotic, 100 = full human mimicry
typedef struct {
    int level;           // 0-100
    int min_delay_ms;    // minimum delay between actions
    int max_delay_ms;    // maximum delay between actions
    bool mouse_movement; // whether to simulate mouse path to target
    bool typing_jitter;  // whether to vary typing speed
    int typing_min_ms;   // min ms per keystroke
    int typing_max_ms;   // max ms per keystroke
} HumanizeConfig;

// Initialize with default level
void humanize_init(void);

// Set level 0-100
void humanize_set_level(int level);

// Get current config
HumanizeConfig *humanize_get_config(void);

// --- Human-like mouse movement ---
// Move mouse from current position to (tx, ty) with realistic trajectory
void humanize_move_mouse(WebKitWebView *web_view, int tx, int ty);

// Click with optional pre-click hover and post-click delay
void humanize_click(WebKitWebView *web_view, int x, int y);

// Double click
void humanize_double_click(WebKitWebView *web_view, int x, int y);

// Right click
void humanize_right_click(WebKitWebView *web_view, int x, int y);

// --- Human-like typing ---
// Type text with realistic speed variation and occasional pauses
void humanize_type_text(WebKitWebView *web_view, const char *text);

// --- Timing helpers ---
// Sleep for a random duration within [min, max] based on level
void humanize_delay(void);

// Sleep for a specific range (scaled by humanization level)
void humanize_delay_range(int min_ms, int max_ms);

#endif // HUMANIZE_H
