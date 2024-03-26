#ifndef __HANDMADE_H__
#define __HANDMADE_H__


struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

void game_update_and_render(struct game_offscreen_buffer *buffer, int blue_offset, int green_offset);


#endif // __HANDMADE_H__

