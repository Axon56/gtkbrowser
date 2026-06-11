#ifndef COMMAND_H
#define COMMAND_H

#include "browser.h"

// Initialize the command listener (Unix socket or stdin)
int command_init(BrowserState *state, const char *socket_path);

// Process a single command line
char *command_process(BrowserState *state, const char *line);

// Cleanup
void command_cleanup(BrowserState *state);

#endif // COMMAND_H
