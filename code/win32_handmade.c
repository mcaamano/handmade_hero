/*
 * Entry point for Handmade here
 *
 * Will try to implement as c only and not pull in any cpp stuff
 * ...we'll see how that works out ;)
 */

#include <windows.h>

LRESULT main_window_callback(HWND window,
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param) {
    LRESULT result = 0;
    switch (message) {
        case WM_SIZE:
            OutputDebugStringA("WM_SIZE\n");
            break;

        case WM_DESTROY:
            OutputDebugStringA("WM_DESTROY\n");
            break;

        case WM_CLOSE:
            OutputDebugStringA("WM_CLOSE\n");
            break;

        case WM_ACTIVATEAPP:
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            PatBlt(device_context, x, y, width, height, BLACKNESS);
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

    window_class.lpfnWndProc = main_window_callback;

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
            MSG message;
            for (;;) {
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
