#ifndef __HANDMADE_H__
#define __HANDMADE_H__

#include <stdbool.h>

#define KILO_BYTES(x)       (x*1024ULL)
#define MEGA_BYTES(x)       (KILO_BYTES(x)*1024ULL)
#define GIGA_BYTES(x)       (MEGA_BYTES(x)*1024ULL)
#define TERA_BYTES(x)       (GIGA_BYTES(x)*1024ULL)

/*
 * Global Defines
 * 
 * HANDMADE_INTERNAL:
 * 0 - Build for public release
 * 1 - Build for developer only
 * 
 * HANDMADE_SLOW:
 * 0 - No slow code allowed!
 * 1 - Slow code welcome
 */

#ifdef HANDMADE_SLOW
#include <assert.h>
#define ASSERT(Expression)  assert(Expression)
#else
#define ASSERT(Expression)
#endif

#ifdef HANDMADE_INTERNAL
#define STORAGE_BASE_ADDR TERA_BYTES(2)
#else
#define STORAGE_BASE_ADDR 0
#endif

#define MAX_CONTROLLERS     4
#define BASE_TONE           512

#define ARRAY_COUNT(array)  (sizeof(array)/sizeof((array)[0]))

inline uint32_t safe_truncate_uint64(uint64_t value) {
    uint32_t result;
    assert(value <= UINT32_MAX);
    result = (uint32_t)value;
    return result;
}

#ifdef HANDMADE_INTERNAL
// These are NOT for doing anything in the shipping game, they are blocking
// and the write doesn't protect against lost data!
struct debug_read_file_results {
    uint32_t data_size;
    void *data;
};
struct debug_read_file_results debug_platform_read_entire_file(char *filename);
void debug_platform_free_file_memory(void *data);
bool debug_platform_write_entire_file(char *filename, uint32_t data_size, void *data);
#endif

struct game_memory {
    bool is_initialized;
    uint64_t permanent_storage_size;
    void *permanent_storage;    // REQUIERED to be cleared to zero at startup
    uint64_t transient_storage_size;
    void *transient_storage;    // REQUIERED to be cleared to zero at startup
};

struct game_state {
    int blue_offset;
    int green_offset;
    int tone_hz;
};

struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_sound_output_buffer {
    int samples_per_second;
    int sample_count;
    int16_t *samples;

};

struct game_button_state {
    int half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_analog;

    float start_x;
    float min_x;
    float max_x;
    float end_x;

    float start_y;
    float min_y;
    float max_y;
    float end_y;

    union {
        struct game_button_state buttons[0];
        struct {
            struct game_button_state up;
            struct game_button_state down;
            struct game_button_state left;
            struct game_button_state right;
            struct game_button_state left_shoulder;
            struct game_button_state right_shoulder;
        };
    };
};

struct game_input {
    // TODO inser clock value here
    struct game_controller_input controllers[MAX_CONTROLLERS];
};

void game_update_and_render(struct game_memory *memory,
                            struct game_input *input,
                            struct game_offscreen_buffer *buffer,
                            struct game_sound_output_buffer *sound_buffer);


#endif // __HANDMADE_H__

