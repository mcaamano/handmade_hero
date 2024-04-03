#ifndef __WIN32_HANDMADE_H_
#define __WIN32_HANDMADE_H_

#define BYTES_PER_PIXEL             4

struct win32_offscreen_buffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct win32_window_dimension {
    int width;
    int height;
};

struct win32_sound_output {
    int samples_per_second;
    uint32_t running_sample_index;
    
    int bytes_per_sample;
    int secondary_buffer_size;
    float tSine;
    int latency_sample_count;
};


#endif //__WIN32_HANDMADE_H_