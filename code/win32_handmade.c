/*
 * Entry point for Handmade Hero
 *
 * Will try to implement as C only and not pull in any cpp stuff
 * ...we'll see how that works out ;)
 */

#include <windows.h>
#include <Xinput.h>
#include <stdbool.h>
#include <stdint.h>
#include <dsound.h>

// TODO implement sine ourselves
#include <math.h>
#include <stdio.h>

#include "handmade.h"
#include "handmade.c"

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
    int base_tone;
    int tone_hz;
    int16_t tone_volume;
    uint32_t running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int secondary_buffer_size;
    float tSine;
    int latency_sample_count;
};

static bool is_running;

static bool tone_up_event = false;
static bool tone_down_event = false;

static struct win32_offscreen_buffer global_backbuffer;
static LPDIRECTSOUND direct_sound;
static LPDIRECTSOUNDBUFFER secondary_buffer;

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static x_input_get_state *XInputGetState_;
static x_input_set_state *XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

static void win32_load_xinput(void) {
    HMODULE x_input_library = LoadLibraryA("xinput1_4.dll");
    if(!x_input_library) {
        // Try 9.1
        x_input_library = LoadLibraryA("xinput9_1_0.dll");
    }
    if (!x_input_library) {
        // try 1.3
        x_input_library = LoadLibraryA("xinput1_3.dll");
    }
    if (x_input_library) {
        XInputGetState_ = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");
        XInputSetState_ = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
    } else {
        // TODO diagnostic
    }
}

// typedef HRESULT WINAPI x_direct_sound_create(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
// static x_direct_sound_create *DirectSoundCreate_;
// #define DirectSoundCreate DirectSoundCreate_

static void win32_init_dsound(HWND window, struct win32_sound_output *sound_output) {
    HRESULT result = DirectSoundCreate(0, &direct_sound, 0);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("DirectSoundCreate failed\n");
        exit(1);
    }
    OutputDebugStringA("DirectSoundCreate OK\n");

    // Set CoOp mode
    result = direct_sound->lpVtbl->SetCooperativeLevel(direct_sound, window, DSSCL_PRIORITY);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("SetCooperativeLevel failed\n");
        exit(1);
    }
    OutputDebugStringA("SetCooperativeLevel OK\n");

    // Create a primary buffer
    DSBUFFERDESC primary_buffer_desc = {0};
    primary_buffer_desc.dwSize = sizeof(primary_buffer_desc);
    primary_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    LPDIRECTSOUNDBUFFER primary_buffer;
    result = direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &primary_buffer_desc, &primary_buffer, 0);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("CreateSoundBuffer primary failed\n");
        exit(1);
    }
    OutputDebugStringA("CreateSoundBuffer primary_buffer OK\n");

    WAVEFORMATEX wave_format = {0};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = sound_output->samples_per_second;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;
    wave_format.cbSize = 0;
    result = primary_buffer->lpVtbl->SetFormat(primary_buffer, &wave_format);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("SetFormat failed\n");
        exit(1);
    }
    OutputDebugStringA("SetFormat primary_buffer OK\n");

    // Create a secondary buffer
    DSBUFFERDESC secondary_buffer_desc = {0};
    secondary_buffer_desc.dwSize = sizeof(primary_buffer_desc);
    secondary_buffer_desc.dwFlags = 0;
    secondary_buffer_desc.dwBufferBytes = sound_output->secondary_buffer_size;
    secondary_buffer_desc.lpwfxFormat = &wave_format;
    result = direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &secondary_buffer_desc, &secondary_buffer, 0);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("CreateSoundBuffer secondary failed\n");
        exit(1);
    }
    OutputDebugStringA("CreateSoundBuffer secondary_buffer OK\n");
}


static void win32_clear_soundbuffer(struct win32_sound_output *sound_output) {
    HRESULT result;
    void *region1;
    DWORD region1_size;
    void *region2;
    DWORD region2_size;
    result = secondary_buffer->lpVtbl->Lock(
        secondary_buffer,
        0,
        sound_output->secondary_buffer_size,
        &region1, &region1_size,
        &region2, &region2_size, 0);
    if (FAILED(result)) {
        // TODO diagnostic
        // OutputDebugStringA("secondary_buffer->lpVtbl->Lock failed\n");
        return;
    }
    int8_t *dest_sample = (int8_t *)region1;
    for(DWORD byte_index = 0; byte_index<region1_size; ++byte_index ) {
        *dest_sample++ = 0;
    }
    dest_sample = (int8_t *)region2;
    for(DWORD byte_index = 0; byte_index<region2_size; ++byte_index ) {
        *dest_sample++ = 0;
    }
    result = secondary_buffer->lpVtbl->Unlock(
        secondary_buffer,
        region1, region1_size,
        region2, region2_size);
    if (FAILED(result)) {
        // TODO diagnostic
        // OutputDebugStringA("secondary_buffer->lpVtbl->Unlock failed\n");
        return;
    }
}

static void win32_fill_soundbuffer(struct win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write, struct game_sound_output_buffer *source_buffer) {
    HRESULT result;
    // DirectSound output test
    // int16 int16 int16 int16 
    // [LEFT  RIGHT] [LEFT  RIGHT]
    //
    void *region1;
    DWORD region1_size;
    void *region2;
    DWORD region2_size;
    result = secondary_buffer->lpVtbl->Lock(
        secondary_buffer,
        byte_to_lock,
        bytes_to_write,
        &region1, &region1_size,
        &region2, &region2_size, 0);
    if (FAILED(result)) {
        // TODO diagnostic
        // OutputDebugStringA("secondary_buffer->lpVtbl->Lock failed\n");
        return;
    }
    // OutputDebugStringA("secondary_buffer Lock OK\n");
    // TODO assert region1_size and region2_size are valid
    DWORD region1_sample_count = region1_size/sound_output->bytes_per_sample;
    int16_t *source_sample = source_buffer->samples;
    int16_t *dest_sample = (int16_t *)region1;
    for(DWORD sample_index = 0; sample_index<region1_sample_count; ++sample_index ) {
        *dest_sample++ = *source_sample++;
        *dest_sample++ = *source_sample++;

        ++sound_output->running_sample_index;
    }
    DWORD region2_sample_count = region2_size/sound_output->bytes_per_sample;
    dest_sample = (int16_t *)region2;
    for(DWORD sample_index = 0; sample_index<region2_sample_count; ++sample_index ) {
        *dest_sample++ = *source_sample++;
        *dest_sample++ = *source_sample++;

        ++sound_output->running_sample_index;
    }
    result = secondary_buffer->lpVtbl->Unlock(
        secondary_buffer,
        region1, region1_size,
        region2, region2_size);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("secondary_buffer->lpVtbl->Unlock failed\n");
        return;
    }
    // OutputDebugStringA("secondary_buffer Unlock OK\n");
}

static struct win32_window_dimension win32_get_window_dimensions(HWND window) {
    struct win32_window_dimension result;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.height = client_rect.bottom - client_rect.top;
    result.width = client_rect.right - client_rect.left;
    return result;
}


static void win32_resize_dib_section(struct win32_offscreen_buffer *buffer, int width, int height) {
    // TODO Bulletproof this later, maybe don't free first

    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    // treat as TopDown not BottomUp - First 4bytes of the image 
    // are the top left pixel of the image (not the bottom left)
    buffer->info.bmiHeader.biHeight = -buffer->height; 
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;  // align on 4 byte boundary
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    // allocate memory ourselves
    int bitmap_size = (buffer->width * buffer->height) * BYTES_PER_PIXEL;
    // int bitmap_size = 0;
    buffer->memory = VirtualAlloc(NULL, bitmap_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    // probably clear this to black

    buffer->pitch = buffer->width * BYTES_PER_PIXEL;
}


static void win32_display_buffer_in_window(HDC device_context, 
                                           int window_width,
                                           int window_height,
                                           struct win32_offscreen_buffer *buffer) {
    // TODO Aspect Ratio correction
    // TODO Play with stretch modes
    StretchDIBits(device_context, 
                //   x, y, width, height, // destination
                //   x, y, width, height, // source
                  0, 0, window_width, window_height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}


LRESULT win32_main_window_callback(HWND window,
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param) {
    LRESULT result = 0;
    switch (message) {
        case WM_SIZE:
            break;

        case WM_DESTROY:
            // TODO: handle this as an error - recreate window?
            is_running = false;
            break;

        case WM_CLOSE:
            // TODO: handle this with a message to the user?
            is_running = false;
            break;

        case WM_ACTIVATEAPP:
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
            uint32_t vkey_code = w_param;
            bool was_down = ((l_param & (1<<30)) != 0);
            bool is_down = ((l_param & (1<<31)) == 0);
            if (was_down != is_down) {
                switch(vkey_code) {
                    case 'W':
                        OutputDebugStringA("W KEY");
                        break;
                    case 'A':
                        OutputDebugStringA("A KEY");
                        break;
                    case 'S':
                        OutputDebugStringA("S KEY");
                        break;
                    case 'D':
                        OutputDebugStringA("D KEY");
                        break;
                    case 'Q':
                        OutputDebugStringA("Q KEY");
                        break;
                    case 'E':
                        OutputDebugStringA("E KEY");
                        break;
                    case VK_SPACE:
                        OutputDebugStringA("SPACEBAR");
                        break;
                    case VK_LEFT:
                        OutputDebugStringA("LEFT");
                        break;
                    case VK_RIGHT:
                        OutputDebugStringA("RIGHT");
                        break;
                    case VK_UP:
                        if (is_down && !was_down) {
                            OutputDebugStringA("TONE_UP EVENT\n");
                            tone_up_event = true;
                        }
                        OutputDebugStringA("UP");
                        break;
                    case VK_DOWN:
                        if (is_down && !was_down) {
                            OutputDebugStringA("TONE_DOWN EVENT\n");
                            tone_down_event = true;
                        }
                        OutputDebugStringA("DOWN");
                        break;
                }
                if (is_down) {
                    OutputDebugStringA(" is_down ");
                } else {
                    OutputDebugStringA(" is_up ");
                }
                if (was_down) {
                    OutputDebugStringA(" was_down");
                } else {
                    OutputDebugStringA(" was_up");
                }
                OutputDebugStringA("\n");
            }
            bool alt_key_was_down = ((l_param & (1<<29)) != 0);
            if ((vkey_code == VK_F4) && alt_key_was_down) {
                is_running = false;
            }
            break;

        case WM_PAINT: {
            // Whenever Windoes wants to (re)draw our window this gets called
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            struct win32_window_dimension window_dim = win32_get_window_dimensions(window);
            win32_display_buffer_in_window(device_context, window_dim.width, window_dim.height, &global_backbuffer);
            EndPaint(window, &paint);
            break;
        }

        default:
            // OutputDebugStringA("Unhandled Message\n");
            result = DefWindowProcA(window, message, w_param, l_param);
            break;
        }
    return result;
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, int show_code) {
    HRESULT result;
    win32_load_xinput();

    WNDCLASSA window_class = {0};

    win32_resize_dib_section(&global_backbuffer, 1280, 720);

    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Handmade_Hero_Window_Class";

    LARGE_INTEGER perfcounter_frequency_result;
    int64_t perfcounter_frequency;
    QueryPerformanceFrequency(&perfcounter_frequency_result);
    perfcounter_frequency = perfcounter_frequency_result.QuadPart;

    if (!RegisterClassA(&window_class)) {
        OutputDebugStringA("RegisterClassA failed\n");
        exit(1);
    }
    // ask to create window
    HWND window = CreateWindowExA(
        0,
        window_class.lpszClassName,
        "Handmade Hero",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        instance,
        0);
    if (!window) {
        OutputDebugStringA("CreateWindowExA failed\n");
        exit(1);
    }
    /* 
     * NOTE: Since we specified CS_OWNDC, we can just get one
     * device context and use it forever because we are not
     * sharing it with anyone.
     */
    HDC device_context = GetDC(window);

    // graphics test
    int x_offset = 0;
    int y_offset = 0;

    struct win32_sound_output sound_output = {0};
    sound_output.samples_per_second = 48000;
    sound_output.base_tone = 256; // 256 cycles per second near middle C tone (261Hz)
    sound_output.tone_hz = 256; 
    sound_output.tone_volume = 3000;
    sound_output.running_sample_index = 0;
    sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
    sound_output.bytes_per_sample = sizeof(int16_t)*2;
    sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
    sound_output.latency_sample_count = sound_output.samples_per_second / 10;

    // sound test
    win32_init_dsound(window, &sound_output);
    win32_clear_soundbuffer(&sound_output);
    result = secondary_buffer->lpVtbl->Play(secondary_buffer, 0, 0, DSBPLAY_LOOPING);
    if (FAILED(result)) {
        // TODO diagnostic
        OutputDebugStringA("secondary_buffer->lpVtbl->Play failed\n");
        exit(1);
    }

    is_running = true;

    // TODO pool this into bitmap virtualalloc
    int16_t *samples = (int16_t *) VirtualAlloc(NULL, sound_output.secondary_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);
    uint64_t last_cycle_count = __rdtsc();
    while (is_running) {
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                is_running = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        // Should we poll this more frequently
        for (int controller_index=0; controller_index<XUSER_MAX_COUNT; ++controller_index) {
            XINPUT_STATE controller_state;
            if (XInputGetState_ && XInputGetState_(controller_index, &controller_state) == ERROR_SUCCESS) {
                // This controller is plugged in
                // TODO see if controller_state.dwPacketNumber increments too fast
                XINPUT_GAMEPAD *pad = &controller_state.Gamepad;
                if (pad->wButtons != 0) {
                    OutputDebugStringA("Got GamePad Activity\n");
                }
                bool pad_up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                bool pad_down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                bool pad_left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                bool pad_right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                bool pad_start = pad->wButtons & XINPUT_GAMEPAD_START;
                bool pad_back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                bool pad_left_s = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                bool pad_right_s = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                bool pad_a = pad->wButtons & XINPUT_GAMEPAD_A;
                bool pad_b = pad->wButtons & XINPUT_GAMEPAD_B;
                bool pad_x = pad->wButtons & XINPUT_GAMEPAD_X;
                bool pad_y = pad->wButtons & XINPUT_GAMEPAD_Y;
                int16_t stick_x = pad->sThumbLX;
                int16_t stick_y = pad->sThumbLY;

                x_offset += stick_x / 4096;
                y_offset += stick_y / 4096;

                if (tone_up_event) {
                    sound_output.base_tone = sound_output.tone_hz+50;
                    // sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
                    tone_up_event = false;
                } else if (tone_down_event) {
                    sound_output.base_tone = sound_output.tone_hz-50;
                    // sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
                    tone_down_event = false;
                }

                sound_output.tone_hz = sound_output.base_tone + (int)(256.0f*((float)stick_y / 30000.0f));
                sound_output.wave_period = sound_output.samples_per_second/sound_output.tone_hz;

                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 0;
                // vibration.wRightMotorSpeed = 0;
                // if (pad_x && pad_b) {
                //     vibration.wLeftMotorSpeed = 60000;
                //     vibration.wRightMotorSpeed = 60000;
                // } else if (pad_x) {
                //     vibration.wLeftMotorSpeed = 60000;
                // } else if (pad_b) {
                //     vibration.wRightMotorSpeed = 60000;
                // }
                // XInputSetState_(controller_index, &vibration);
            } else {
                // The controller is not available
            }
        }


        DWORD play_cursor;
        DWORD write_cursor;
        DWORD byte_to_lock;
        DWORD bytes_to_write;
        DWORD target_cursor;

        bool sound_is_valid = false;

        // After init check current position and fill in the rest
        result = secondary_buffer->lpVtbl->GetCurrentPosition(secondary_buffer, &play_cursor, &write_cursor);
        if (SUCCEEDED(result)) {
            byte_to_lock = (sound_output.running_sample_index*sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
            target_cursor = ((play_cursor + (sound_output.latency_sample_count*sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size);
            bytes_to_write = 0;
            // TODO change this to using lower latency offset from the playcursor
            // when we actually start having sound effects
            if (byte_to_lock > target_cursor) {
                // case where we have 2 regions to write (=)
                // |           |play_cursor                 |byte_to_write
                // |===========v____________v_______________v===============|
                // |                        |write_cursor
                //
                bytes_to_write = sound_output.secondary_buffer_size - byte_to_lock;
                bytes_to_write += target_cursor;
            } else {
                // case where we only have 1 region to write (=)
                // |           |play_cursor
                // |___v=======v___________________v______________________|
                // |   |byte_to_write               |write_cursor
                //
                bytes_to_write = target_cursor - byte_to_lock;
            }
            sound_is_valid = true;
        }

        struct game_sound_output_buffer sound_buffer = {0};
        sound_buffer.samples_per_second = sound_output.samples_per_second;
        sound_buffer.sample_count = bytes_to_write/sound_output.bytes_per_sample;
        sound_buffer.samples = samples;

        struct game_offscreen_buffer buffer = {0};
        buffer.memory = global_backbuffer.memory;
        buffer.width = global_backbuffer.width;
        buffer.height = global_backbuffer.height;
        buffer.pitch = global_backbuffer.pitch;
        game_update_and_render(&buffer, x_offset, y_offset, &sound_buffer, sound_output.tone_hz);

        if (sound_is_valid) {
            win32_fill_soundbuffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
        }

        struct win32_window_dimension window_dim = win32_get_window_dimensions(window);

        win32_display_buffer_in_window(device_context, window_dim.width, window_dim.height, &global_backbuffer);

        uint64_t end_cycle_count = __rdtsc();

        LARGE_INTEGER end_counter;
        QueryPerformanceCounter(&end_counter);

        uint64_t cycles_elapsed = end_cycle_count - last_cycle_count;
        float mega_cycles_per_frame = (float)cycles_elapsed/(1000.0f*1000.0f);
        int64_t counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
        float milliseconds_per_frame = ((float)counter_elapsed*1000.0f) / (float)perfcounter_frequency;
        float frames_per_second = (float)((float)perfcounter_frequency/(float)counter_elapsed);

        char stats_buffer[256];
        sprintf(stats_buffer, "ms/frame: %.02f | FPS: %.02f | MegaCycles/Frame: %.02f\n", milliseconds_per_frame, frames_per_second, mega_cycles_per_frame);
        OutputDebugStringA(stats_buffer);

        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
    }

    return 0;
}
