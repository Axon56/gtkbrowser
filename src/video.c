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

    strncpy(g_output_path, filepath, sizeof(g_output_path) - 1);

    // Get display size
    char size_buf[32] = "1280x800";
    FILE *fp = popen("xdpyinfo 2>/dev/null | grep dimensions | awk '{print $2}'", "r");
    if (fp) {
        char tmp[32] = {0};
        if (fgets(tmp, sizeof(tmp), fp)) {
            tmp[strcspn(tmp, "\n")] = 0;
            if (strlen(tmp) > 2) strcpy(size_buf, tmp);
        }
        pclose(fp);
    }

    // Pad to even dimensions for H.264
    int w = 0, h = 0;
    sscanf(size_buf, "%dx%d", &w, &h);
    if (w % 2 != 0) w++;
    if (h % 2 != 0) h++;
    char padded_size[32];
    snprintf(padded_size, sizeof(padded_size), "%dx%d", w, h);

    char fps_str[16];
    snprintf(fps_str, sizeof(fps_str), "%d", fps);

    // Start ffmpeg x11grab capture in background
    g_ffmpeg_pid = fork();
    if (g_ffmpeg_pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        char *argv[] = {
            "ffmpeg", "-y",
            "-f", "x11grab",
            "-framerate", fps_str,
            "-video_size", padded_size,
            "-i", (char *)display,
            "-pix_fmt", "yuv420p",
            "-c:v", "libx264",
            "-preset", "ultrafast",
            "-crf", "23",
            (char *)filepath,
            NULL
        };
        execvp("ffmpeg", argv);
        _exit(1);
    } else if (g_ffmpeg_pid > 0) {
        g_recording = true;
        fprintf(stderr, "GTKBrowser: Recording started → %s (pid %d, %s, %dfps)\n",
                filepath, g_ffmpeg_pid, padded_size, fps);
        return true;
    }

    return false;
}

bool video_stop_record(void) {
    if (!g_recording || g_ffmpeg_pid <= 0) return false;

    // Send SIGINT to ffmpeg (triggers clean MP4 finalization)
    kill(g_ffmpeg_pid, SIGINT);

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
