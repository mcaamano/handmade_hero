#ifndef __HANDMADE_H__
#define __HANDMADE_H__

#define MAX_CONTROLLERS     4

#define ARRAY_COUNT(array)  (sizeof(array)/sizeof((array)[0]))

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
    struct game_controller_input controllers[MAX_CONTROLLERS];
};

void game_update_and_render(struct game_input *input,
                            struct game_offscreen_buffer *buffer,
                            struct game_sound_output_buffer *sound_buffer);


#endif // __HANDMADE_H__

