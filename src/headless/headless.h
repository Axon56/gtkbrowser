#ifndef HEADLESS_H
#define HEADLESS_H

#include <stdbool.h>

// Auto-start Xvfb if no display is available
// Returns true if we started Xvfb (caller should clean up), false if display already exists
bool headless_auto_start(void);

// Stop the auto-started Xvfb
void headless_stop(void);

// Get the display string that was set
const char *headless_get_display(void);

#endif // HEADLESS_H
