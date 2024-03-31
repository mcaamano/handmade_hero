#ifndef __HANDMADE_H__
#define __HANDMADE_H__


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

void game_update_and_render(struct game_offscreen_buffer *buffer,
                            int blue_offset,
                            int green_offset,
                            struct game_sound_output_buffer *sound_buffer,
                            int tone_hz);


#endif // __HANDMADE_H__

