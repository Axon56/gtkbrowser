#include "video.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <glib.h>

static pid_t g_ffmpeg_pid = 0;
static char g_output_path[1024] = {0};
static bool g_recording = false;

bool video_start_record(const char *filepath, int fps) {
    if (g_recording) return false;
    if (!filepath || fps < 1 || fps > 60) return false;

    const char *display = getenv("DISPLAY");
    if (!display) display = ":0";

    // Copy output path
    strncpy(g_output_path, filepath, sizeof(g_output_path) - 1);

    // Build ffmpeg command for x11grab capture
    char fps_str[16];
    snprintf(fps_str, sizeof(fps_str), "%d", fps);

    g_ffmpeg_pid = fork();
    if (g_ffmpeg_pid == 0) {
        // Child: run ffmpeg
        char *argv[] = {
            "ffmpeg",
            "-y",                          // overwrite output
            "-f", "x11grab",              // X11 screen capture
            "-framerate", fps_str,        // frame rate
            "-video_size", "1280x800",    // capture size
            "-i", (char *)display,        // display
            "-pix_fmt", "yuv420p",        // pixel format
            "-c:v", "libx264",            // H.264 codec
            "-preset", "ultrafast",       // fast encoding
            "-crf", "23",                 // quality
            (char *)filepath,             // output file
            NULL
        };

        // Redirect stdout/stderr to /dev/null
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        execvp("ffmpeg", argv);
        _exit(1);
    } else if (g_ffmpeg_pid > 0) {
        g_recording = true;
        fprintf(stderr, "GTKBrowser: Recording started → %s (pid %d)\n",
                filepath, g_ffmpeg_pid);
        return true;
    }

    return false;
}

bool video_stop_record(void) {
    if (!g_recording || g_ffmpeg_pid <= 0) return false;

    // Send SIGINT to ffmpeg (triggers clean MP4 finalization)
    kill(g_ffmpeg_pid, SIGINT);

    // Wait for ffmpeg to finish
    int status;
    waitpid(g_ffmpeg_pid, &status, 0);

    fprintf(stderr, "GTKBrowser: Recording stopped → %s\n", g_output_path);

    g_recording = false;
    g_ffmpeg_pid = 0;
    return true;
}

bool video_is_recording(void) {
    return g_recording;
}

const char *video_get_output(void) {
    return g_recording ? g_output_path : NULL;
}
