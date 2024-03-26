#include <stdint.h>
#include "handmade.h"



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

void game_update_and_render(struct game_offscreen_buffer *buffer, int blue_offset, int green_offset) {
    render_weird_gradient(buffer, blue_offset, green_offset);
}
