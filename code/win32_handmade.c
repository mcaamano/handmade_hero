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

#define BYTES_PER_PIXEL     4


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

static struct win32_offscreen_buffer global_backbuffer;
static bool is_running;

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static x_input_get_state *XInputGetState_;
static x_input_set_state *XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

static void win32_load_xinput(void) {
    HMODULE x_input_library = LoadLibraryA(XINPUT_DLL_A);
    if (x_input_library) {
        XInputGetState_ = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");
        XInputSetState_ = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
    }
}

static struct win32_window_dimension win32_get_window_dimensions(HWND window) {
    struct win32_window_dimension result;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.height = client_rect.bottom - client_rect.top;
    result.width = client_rect.right - client_rect.left;
    return result;
}

static void render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset) {
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
            int blue = (x + x_offset);
            int green = (y + y_offset);
            int red = 0;
           
            *pixel++ = (red<<16 | green<<8 | blue);
        }
        row += buffer->pitch;
    }
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
    buffer->memory = VirtualAlloc(NULL, bitmap_size, MEM_COMMIT, PAGE_READWRITE);

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
                    OutputDebugStringA("UP");
                    break;
                case VK_DOWN:
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
    win32_load_xinput();

    WNDCLASSA window_class = {0};

    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Handmade_Hero_Window_Class";

    win32_resize_dib_section(&global_backbuffer, 1280, 720);

    if (RegisterClass(&window_class)) {
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
        if (window) {
            /* 
             * NOTE: Since we specified CS_OWNDC, we can just get one
             * device context and use it forever because we are not
             * sharing it with anyone.
             */
            HDC device_context = GetDC(window);

            int x_offset = 0;
            int y_offset = 0;
            is_running = true;
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

                        // Add some vertical movement
                        y_offset++;

                        XINPUT_VIBRATION vibration;
                        vibration.wLeftMotorSpeed = 60000;
                        vibration.wRightMotorSpeed = 60000;
                        XInputSetState_(controller_index, &vibration);
                    } else {
                        // The controller is not available
                    }
                }

                render_weird_gradient(&global_backbuffer, x_offset, y_offset);
                
                struct win32_window_dimension window_dim = win32_get_window_dimensions(window);

                win32_display_buffer_in_window(device_context, window_dim.width, window_dim.height, &global_backbuffer);

                x_offset++;
                // y_offset++;
            }
        } else {
            // TODO log error
        }

    } else {
        // TODO log error
    }



    return 0;
}
