#ifndef VIDEO_H
#define VIDEO_H

#include <stdbool.h>

// Start screen recording to file (MP4)
// fps: frames per second (default 15)
bool video_start_record(const char *filepath, int fps);

// Stop recording and finalize MP4
bool video_stop_record(void);

// Check if recording is active
bool video_is_recording(void);

// Get the output file path
const char *video_get_output(void);

#endif // VIDEO_H
