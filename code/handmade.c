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

void game_update_and_render(struct game_memory *memory,
                            struct game_input *input,
                            struct game_offscreen_buffer *buffer,
                            struct game_sound_output_buffer *sound_buffer) {

    ASSERT(sizeof(struct game_state) <= memory->permanent_storage_size);

    struct game_controller_input *input0 = &input->controllers[0];
    struct game_state *state = (struct game_state *)memory->permanent_storage;

    if (!memory->is_initialized) {
        char *filename = __FILE__;

        struct debug_read_file_results file = debug_platform_read_entire_file(filename);
        if (!file.data) {
            // handle error
        }
        debug_platform_write_entire_file("test.out", file.data_size, file.data);
        debug_platform_free_file_memory(file.data);


        // no need to set this since VirtualAlloc will set memory to zero before giving it to us
        // state->blue_offset = 0;
        // state->green_offset = 0;
        state->tone_hz = BASE_TONE;

        //TODO this may be more appropriate to do in platform layer
        memory->is_initialized = true;
    }

    if (input0->is_analog) {
        // use analog movement tuning
        state->tone_hz = BASE_TONE + (int)(256.0f*input0->end_y);
        state->blue_offset += (int)(4.0f*input0->end_x);
    } else {
        // use digial movement tuning
    }
    // input.a_btn_ended_down
    // input.a_btn_half_transition_count
    if (input0->down.ended_down) {
        state->green_offset += 1;
    }
    // input.start_x
    // input.min_x
    // input.max_x
    // input.end_x


    // wave_period = sound_output.samples_per_second/sound_output.tone_hz;

    // TODO allow sample offset here for more robust platform options
    output_game_sound(sound_buffer, state->tone_hz);
    render_weird_gradient(buffer, state->blue_offset, state->green_offset);
}
