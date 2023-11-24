#define BASE_IMPLEMENTATION
#include "base.h"

#include <windows.h>

static bool running;

LRESULT main_window_callback(
    HWND window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    (void)window_handle;
    (void)w_param;
    (void)l_param;

    LRESULT result = 0;

    switch (message) {
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_CLOSE: {
            // TODO: message to user
            running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        case WM_DESTROY: {
            // TODO: error
            running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_ctx = BeginPaint(window_handle, &paint);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            PatBlt(device_ctx, x, y, width, height, BLACKNESS);

            EndPaint(window_handle, &paint);
        } break;
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        default: {
            result = DefWindowProc(window_handle, message, w_param, l_param);
            // OutputDebugStringA("default\n");
        } break;
    }

    return result;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR command_line,
    int show_code
) {
    (void)prev_instance;
    (void)command_line;
    (void)show_code;

    WNDCLASSA window_class = {
        .style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = main_window_callback,
        .hInstance = instance,
        .lpszClassName = "nbody2_window_class",
    };

    if (!RegisterClass(&window_class)) {
        // TODO: handle error
    }

    HWND window_handle = CreateWindowEx(
        0,
        window_class.lpszClassName,
        "nbody2",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        instance,
        0
    );

    if (!window_handle) {
        // TODO: handle error
    }

    MSG message;
    while (GetMessage(&message, 0, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
