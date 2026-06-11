#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <glib.h>

static pid_t g_ffmpeg_pid = 0;
static char g_output_path[1024] = {0};
static bool g_recording = false;
static char g_frames_dir[256] = {0};
static int g_frame_count = 0;

bool video_start_record(const char *filepath, int fps) {
    if (g_recording) return false;
    if (!filepath || fps < 1 || fps > 60) return false;

    const char *display = getenv("DISPLAY");
    if (!display) display = ":0";

    strncpy(g_output_path, filepath, sizeof(g_output_path) - 1);

    // Create temp directory for frames
    snprintf(g_frames_dir, sizeof(g_frames_dir), "/tmp/gtkbrowser_frames_%d", getpid());
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "rm -rf %s && mkdir -p %s", g_frames_dir, g_frames_dir);
    system(mkdir_cmd);

    g_frame_count = 0;
    g_recording = true;

    fprintf(stderr, "GTKBrowser: Recording started → %s\n", filepath);
    return true;
}

void video_capture_frame(void) {
    if (!g_recording) return;

    const char *display = getenv("DISPLAY");
    if (!display) display = ":0";

    char frame_path[512];
    snprintf(frame_path, sizeof(frame_path), "%s/frame_%05d.png", g_frames_dir, g_frame_count);

    // Capture frame using import
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "DISPLAY=%s import -window root '%s'", display, frame_path);
    int ret = system(cmd);
    if (ret != 0) {
        // Fallback: try xwd
        char xwd_cmd[1024];
        snprintf(xwd_cmd, sizeof(xwd_cmd), "DISPLAY=%s xwd -root -out '%s.xwd' 2>/dev/null && convert '%s.xwd' '%s' 2>/dev/null && rm -f '%s.xwd'", display, frame_path, frame_path, frame_path, frame_path);
        system(xwd_cmd);
    }

    g_frame_count++;
}

bool video_stop_record(void) {
    if (!g_recording) return false;

    g_recording = false;

    if (g_frame_count == 0) {
        fprintf(stderr, "GTKBrowser: No frames captured\n");
        return false;
    }

    fprintf(stderr, "GTKBrowser: Encoding %d frames...\n", g_frame_count);

    // Build ffmpeg command to encode frames to MP4
    // H.264 requires even dimensions — use pad filter
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -framerate 10 -i '%s/frame_%%05d.png' "
        "-vf 'pad=ceil(iw/2)*2:ceil(ih/2)*2' "
        "-c:v libx264 -pix_fmt yuv420p -preset fast -crf 23 '%s' 2>/dev/null",
        g_frames_dir, g_output_path);

    int ret = system(cmd);

    // Clean up frames
    char rm_cmd[512];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf %s", g_frames_dir);
    system(rm_cmd);

    if (ret == 0) {
        fprintf(stderr, "GTKBrowser: Recording saved → %s\n", g_output_path);
        g_frame_count = 0;
        return true;
    } else {
        fprintf(stderr, "GTKBrowser: Encoding failed\n");
        g_frame_count = 0;
        return false;
    }
}

bool video_is_recording(void) {
    return g_recording;
}

const char *video_get_output(void) {
    return g_recording ? g_output_path : NULL;
}
