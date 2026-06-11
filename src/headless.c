#include "headless.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glib.h>

static pid_t g_xvfb_pid = 0;
static char g_display_str[32] = {0};
static int g_display_num = -1;

static bool display_exists(int num) {
    char path[64];
    snprintf(path, sizeof(path), "/tmp/.X11-unix/X%d", num);
    struct stat st;
    return (stat(path, &st) == 0);
}

static int find_free_display(void) {
    for (int i = 99; i >= 0; i--) {
        if (!display_exists(i)) {
            return i;
        }
    }
    return -1;
}

static void cleanup_handler(int sig) {
    (void)sig;
    headless_stop();
}

// Try to install Xvfb if not found
static bool try_install_xvfb(void) {
    // Check common package managers
    if (system("which apt-get >/dev/null 2>&1") == 0) {
        fprintf(stderr, "GTKBrowser: Installing Xvfb via apt...\n");
        return (system("apt-get install -y xvfb >/dev/null 2>&1") == 0);
    }
    else if (system("which dnf >/dev/null 2>&1") == 0) {
        fprintf(stderr, "GTKBrowser: Installing Xvfb via dnf...\n");
        return (system("dnf install -y xorg-x11-server-Xvfb >/dev/null 2>&1") == 0);
    }
    else if (system("which pacman >/dev/null 2>&1") == 0) {
        fprintf(stderr, "GTKBrowser: Installing Xvfb via pacman...\n");
        return (system("pacman -S --noconfirm xorg-server-xvfb >/dev/null 2>&1") == 0);
    }
    return false;
}

bool headless_auto_start(void) {
    // Check if DISPLAY is already set
    const char *current_display = getenv("DISPLAY");
    if (current_display && current_display[0]) {
        return false;
    }

    // Check if Xvfb is available
    if (system("which Xvfb >/dev/null 2>&1") != 0) {
        fprintf(stderr, "GTKBrowser: Xvfb not found. Attempting auto-install...\n");
        if (!try_install_xvfb()) {
            fprintf(stderr, "GTKBrowser: Failed to install Xvfb.\n");
            fprintf(stderr, "GTKBrowser: Please install manually: apt install xvfb\n");
            return false;
        }
        fprintf(stderr, "GTKBrowser: Xvfb installed successfully.\n");
    }

    // Find free display
    g_display_num = find_free_display();
    if (g_display_num < 0) {
        fprintf(stderr, "GTKBrowser: No free display available.\n");
        return false;
    }

    snprintf(g_display_str, sizeof(g_display_str), ":%d", g_display_num);

    // Start Xvfb
    char resolution[32];
    snprintf(resolution, sizeof(resolution), "%dx%dx24", 1280, 800);

    g_xvfb_pid = fork();
    if (g_xvfb_pid == 0) {
        // Child: run Xvfb
        char display_arg[32];
        snprintf(display_arg, sizeof(display_arg), ":%d", g_display_num);

        execl("/usr/bin/Xvfb", "Xvfb",
              display_arg,
              "-screen", "0", resolution,
              "-ac",
              "+extension", "GLX",
              "-nolisten", "tcp",
              NULL);

        _exit(1);
    } else if (g_xvfb_pid > 0) {
        // Parent: wait for Xvfb to be ready
        setenv("DISPLAY", g_display_str, 1);

        g_usleep(500000); // 500ms

        signal(SIGTERM, cleanup_handler);
        signal(SIGINT, cleanup_handler);

        fprintf(stderr, "GTKBrowser: Auto-started Xvfb on display %s (pid %d)\n",
                g_display_str, g_xvfb_pid);
        return true;
    }

    return false;
}

void headless_stop(void) {
    if (g_xvfb_pid > 0) {
        kill(g_xvfb_pid, SIGTERM);
        waitpid(g_xvfb_pid, NULL, 0);
        fprintf(stderr, "GTKBrowser: Stopped Xvfb (pid %d)\n", g_xvfb_pid);
        g_xvfb_pid = 0;
    }
}

const char *headless_get_display(void) {
    return g_display_str;
}
