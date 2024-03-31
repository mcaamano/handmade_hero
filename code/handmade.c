#include <stdint.h>
#include <math.h>

#include "handmade.h"

#define PI                          3.14159265359f

static void output_game_sound(struct game_sound_output_buffer *sound_buffer, int tone_hz) {
    static float tsine;
    int16_t tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second/tone_hz;

    int16_t *sample_out = sound_buffer->samples;
    for(int sample_index = 0; sample_index<sound_buffer->sample_count; ++sample_index ) {
        float sine_value = sinf(tsine);
        int16_t sample_value = (int16_t)(sine_value*(float)tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        tsine += 2.0f*PI*1.0f/(float)wave_period;
    }
}

static void render_weird_gradient(struct game_offscreen_buffer *buffer, int blue_offset, int green_offset) {
    // Draw some pixels
    uint8_t *row = (uint8_t *)buffer->memory;
    for (int y=0; y<buffer->height; ++y) {
        uint32_t *pixel = (uint32_t *)row;
        for (int x=0; x<buffer->width; ++x) {
            /*
                                  0  1  2  3
                Pixel in Memory: 00 00 00 00
                                 BB GG RR XX
                Little Endian Architecture ends up with 0xXXBBGGRR
                MS swapped so that on memory it looks like 0xXXRRGGBB
            */
            uint8_t blue = x + blue_offset;
            uint8_t green = y + green_offset;
            uint8_t red = 0;
           
            *pixel++ = ((red<<16) | (green<<8) | blue);
        }
        row += buffer->pitch;
    }
}

void game_update_and_render(struct game_offscreen_buffer *buffer,
                            int blue_offset,
                            int green_offset,
                            struct game_sound_output_buffer *sound_buffer,
                            int tone_hz) {
    // TODO allow sample offset here for more robust platform options
    output_game_sound(sound_buffer, tone_hz);
    render_weird_gradient(buffer, blue_offset, green_offset);
}
