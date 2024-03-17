/*
 * Entry point for Handmade Hero
 *
 * Will try to implement as C only and not pull in any cpp stuff
 * ...we'll see how that works out ;)
 */

#include <windows.h>
#include <stdbool.h>

// TODO move this elsewhere later
static bool is_running;
static BITMAPINFO bitmap_info;
static void *bitmap_memory;
static HBITMAP bitmap_handle;
static HDC device_context;

static void win32_resize_dib_section(int width, int height) {
    // TODO Bulletproof this later, maybe don't free first

    // Free our DIBSection
    if (bitmap_handle) {
        // pre-existing
        DeleteObject(bitmap_handle);
    }

    if (!device_context) {
        // TODO should we recreate this under certain circumstances
        device_context = CreateCompatibleDC(0);
    }

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    bitmap_handle = CreateDIBSection(device_context, 
                                     &bitmap_info,
                                     DIB_RGB_COLORS,
                                     &bitmap_memory,
                                     0, 0);

}


static void win32_update_window(HDC device_context, int x, int y, int width, int height) {

    StretchDIBits(device_context, 
                  x, y, width, height, // destination
                  x, y, width, height, // source
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
            win32_update_window(device_context, x, y, width, height);
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
        HWND WindowHandle = CreateWindowExA(
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
        if (WindowHandle) {
            is_running = true;
            while (is_running) {
                MSG message;
                BOOL msg_result = GetMessageA(&message, 0, 0, 0);
                if (msg_result > 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                } else {
                    // TODO Log error
                    // break out of message loop
                    break;
                }
            }
        } else {
            // TODO log error
        }

    } else {
        // TODO log error
    }



    return 0;
}
