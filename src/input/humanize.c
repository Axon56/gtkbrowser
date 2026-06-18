#include "humanize.h"
#include "input.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

static HumanizeConfig g_config = {0};
static bool g_initialized = false;

// Seeded random for this session
static void ensure_init(void) {
    if (!g_initialized) {
        srand((unsigned int)time(NULL));
        g_initialized = true;
    }
}

// Random int in [min, max]
static int rand_range(int min, int max) {
    if (min >= max) return min;
    return min + (rand() % (max - min + 1));
}

// Random double in [0.0, 1.0)
static double rand_double(void) {
    return (double)rand() / (double)RAND_MAX;
}

void humanize_init(void) {
    ensure_init();
    humanize_set_level(50); // default mid-level
}

void humanize_set_level(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    g_config.level = level;

    // Scale delays by level: 0 = instant, 100 = slow human
    g_config.min_delay_ms = level * 2;      // 0-200ms
    g_config.max_delay_ms = level * 5;      // 0-500ms
    g_config.mouse_movement = (level > 0);
    g_config.typing_jitter = (level > 10);
    g_config.typing_min_ms = 20 + (level * 1);   // 20-120ms
    g_config.typing_max_ms = 50 + (level * 3);   // 50-350ms
}

HumanizeConfig *humanize_get_config(void) {
    return &g_config;
}

// --- Bezier curve mouse movement ---

typedef struct {
    int start_x, start_y;
    int end_x, end_y;
    int ctrl1_x, ctrl1_y;
    int ctrl2_x, ctrl2_y;
    int steps;
} BezierPath;

static BezierPath make_bezier_path(int sx, int sy, int tx, int ty) {
    BezierPath p;
    p.start_x = sx;
    p.start_y = sy;
    p.end_x = tx;
    p.end_y = ty;

    // Random control points for natural curve
    int dx = tx - sx;
    int dy = ty - sy;
    double dist = sqrt(dx * dx + dy * dy);

    int jitter = (int)(dist * 0.3);
    if (jitter < 20) jitter = 20;

    p.ctrl1_x = sx + dx / 3 + rand_range(-jitter, jitter);
    p.ctrl1_y = sy + dy / 3 + rand_range(-jitter, jitter);
    p.ctrl2_x = sx + 2 * dx / 3 + rand_range(-jitter, jitter);
    p.ctrl2_y = sy + 2 * dy / 3 + rand_range(-jitter, jitter);

    // More steps for longer distances, fewer for short
    p.steps = (int)(dist / 8);
    if (p.steps < 5) p.steps = 5;
    if (p.steps > 80) p.steps = 80;

    return p;
}

static void bezier_point(BezierPath *p, double t, int *x, int *y) {
    double u = 1.0 - t;
    *x = (int)(u*u*u * p->start_x +
               3*u*u*t * p->ctrl1_x +
               3*u*t*t * p->ctrl2_x +
               t*t*t * p->end_x);
    *y = (int)(u*u*u * p->start_y +
               3*u*u*t * p->ctrl1_y +
               3*u*t*t * p->ctrl2_y +
               t*t*t * p->end_y);
}

static void simulate_mouse_path(WebKitWebView *web_view, int tx, int ty) {
    if (!g_config.mouse_movement) return;

    // Get current "virtual" position (use center of window as start)
    int sx = 640, sy = 400;

    BezierPath path = make_bezier_path(sx, sy, tx, ty);

    for (int i = 0; i <= path.steps; i++) {
        double t = (double)i / (double)path.steps;

        // Add slight easing (slow start, fast middle, slow end)
        // Cubic ease-in-out
        double eased_t;
        if (t < 0.5) {
            eased_t = 4.0 * t * t * t;
        } else {
            double f = 2.0 * t - 2.0;
            eased_t = 0.5 * f * f * f + 1.0;
        }

        int px, py;
        bezier_point(&path, eased_t, &px, &py);

        // Add micro-jitter for realism
        if (i > 0 && i < path.steps) {
            px += rand_range(-1, 1);
            py += rand_range(-1, 1);
        }

        input_mouse_move(web_view, px, py);

        // Variable delay between steps (slower at start/end)
        int step_delay;
        if (t < 0.2 || t > 0.8) {
            step_delay = rand_range(8, 25);
        } else {
            step_delay = rand_range(2, 12);
        }

        g_usleep(step_delay * 1000);
    }
}

void humanize_click(WebKitWebView *web_view, int x, int y) {
    ensure_init();

    if (g_config.level > 0) {
        // Pre-click hover (move to target first)
        simulate_mouse_path(web_view, x, y);

        // Small pause before clicking (thinking time)
        int think_ms = rand_range(50, 150 + g_config.level * 3);
        g_usleep(think_ms * 1000);
    }

    // Do the actual click
    input_click(web_view, x, y);

    // Post-click delay
    if (g_config.level > 0) {
        humanize_delay();
    }
}

void humanize_double_click(WebKitWebView *web_view, int x, int y) {
    ensure_init();

    if (g_config.level > 0) {
        simulate_mouse_path(web_view, x, y);
        int think_ms = rand_range(30, 100);
        g_usleep(think_ms * 1000);
    }

    input_double_click(web_view, x, y);

    if (g_config.level > 0) {
        humanize_delay();
    }
}

void humanize_right_click(WebKitWebView *web_view, int x, int y) {
    ensure_init();

    if (g_config.level > 0) {
        simulate_mouse_path(web_view, x, y);
        int think_ms = rand_range(80, 200);
        g_usleep(think_ms * 1000);
    }

    input_right_click(web_view, x, y);

    if (g_config.level > 0) {
        humanize_delay();
    }
}

void humanize_type_text(WebKitWebView *web_view, const char *text) {
    ensure_init();
    if (!text) return;

    while (*text) {
        guint32 ch = g_utf8_get_char(text);

        // Calculate delay for this character
        if (g_config.typing_jitter) {
            int delay_ms = rand_range(g_config.typing_min_ms,
                                       g_config.typing_max_ms);

            // Space/newline = slightly longer pause (thinking)
            if (ch == ' ' || ch == '\n') {
                delay_ms += rand_range(50, 150);
            }

            // Common letters typed faster
            if (ch >= 'a' && ch <= 'z') {
                delay_ms = (int)(delay_ms * 0.8);
            }

            // Rare/special characters typed slower
            if (ch > 127 || ch == '@' || ch == '#' || ch == '&') {
                delay_ms = (int)(delay_ms * 1.5);
            }

            g_usleep(delay_ms * 1000);
        }

        // Type the character
        char buf[8] = {0};
        g_unichar_to_utf8(ch, buf);
        input_type_text(web_view, buf);

        text = g_utf8_next_char(text);
    }
}

void humanize_delay(void) {
    ensure_init();
    if (g_config.level == 0) return;

    int delay = rand_range(g_config.min_delay_ms, g_config.max_delay_ms);
    g_usleep(delay * 1000);
}

void humanize_delay_range(int min_ms, int max_ms) {
    ensure_init();
    if (g_config.level == 0) return;

    // Scale by level
    int scaled_min = min_ms * g_config.level / 100;
    int scaled_max = max_ms * g_config.level / 100;
    int delay = rand_range(scaled_min, scaled_max);
    g_usleep(delay * 1000);
}
