/*
 * Entry point for Handmade Hero
 *
 * Will try to implement as C only and not pull in any cpp stuff
 * ...we'll see how that works out ;)
 */

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BYTES_PER_PIXEL     4

// TODO move this elsewhere later
static bool is_running;
static BITMAPINFO bitmap_info;
static void *bitmap_memory;
static int bitmap_width;
static int bitmap_height;


static void render_weird_gradient(int x_offset, int y_offset) {
    // Draw some pixels
    int pitch = bitmap_width * BYTES_PER_PIXEL;
    u8 *row = (u8 *)bitmap_memory;
    for (int y=0; y<bitmap_height; ++y) {
        u32 *pixel = (u32 *)row;
        for (int x=0; x<bitmap_width; ++x) {
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
        row += pitch;
    }
}

static void win32_resize_dib_section(int width, int height) {
    // TODO Bulletproof this later, maybe don't free first

    if (bitmap_memory) {
        VirtualFree(bitmap_memory, 0, MEM_RELEASE);
    }

    bitmap_width = width;
    bitmap_height = height;

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;  // align on 4 byte boundary
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    // allocate memory ourselves
    int bitmap_size = (width * height) * BYTES_PER_PIXEL;
    bitmap_memory = VirtualAlloc(NULL, bitmap_size, MEM_COMMIT, PAGE_READWRITE);

    // probably clear this to black
}


static void win32_update_window(HDC device_context, RECT *client_rect, int x, int y, int width, int height) {
    int window_width = client_rect->right - client_rect->left;
    int window_height = client_rect->bottom - client_rect->top;
    StretchDIBits(device_context, 
                //   x, y, width, height, // destination
                //   x, y, width, height, // source
                  0, 0, bitmap_width, bitmap_height,
                  0, 0, window_width, window_height,
                  bitmap_memory,
                  &bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);

}


LRESULT win32_main_window_callback(HWND window,
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param) {
    LRESULT result = 0;
    switch (message) {
        case WM_SIZE:
            RECT client_rect;
            GetClientRect(window, &client_rect);
            int height = client_rect.bottom - client_rect.top;
            int width = client_rect.right - client_rect.left;
            win32_resize_dib_section(width, height);
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

        case WM_PAINT: {
            // Whenever Windoes wants to (re)draw our window this gets called
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            RECT client_rect;
            GetClientRect(window, &client_rect);
            win32_update_window(device_context, &client_rect, x, y, width, height);
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
    WNDCLASS window_class = {0};

    // TODO Check if CS_HREDRAW and CS_VREDRAW still matter
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

    window_class.lpfnWndProc = win32_main_window_callback;

    // getModuleHandle can also gives us the current instance
    window_class.hInstance = instance;

    window_class.lpszClassName = "Handmade_Hero_Window_Class";


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

                render_weird_gradient(x_offset, y_offset);

                RECT client_rect;
                HDC device_context = GetDC(window);
                GetClientRect(window, &client_rect);
                int window_width = client_rect.right - client_rect.left;
                int window_height = client_rect.bottom - client_rect.top;

                win32_update_window(device_context, &client_rect, 0, 0, window_width, window_height);
                ReleaseDC(window, device_context);
                x_offset++;
            }
        } else {
            // TODO log error
        }

    } else {
        // TODO log error
    }



    return 0;
}
