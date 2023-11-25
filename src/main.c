#define BASE_IMPLEMENTATION
#include "base.h"

#include <windows.h>

static bool running;

static BITMAPINFO bitmap_info;
static void *bitmap_mem;
static HBITMAP bitmap_handle;
static HDC bitmap_device_ctx;

static void resize_DIB_section(int width, int height) {
    if (bitmap_handle) DeleteObject(bitmap_handle);
    if (!bitmap_device_ctx) bitmap_device_ctx = CreateCompatibleDC(0);

    bitmap_info = (BITMAPINFO){
        .bmiHeader = { 
            .biSize = sizeof(bitmap_info.bmiHeader),
            .biWidth = width,
            .biHeight = height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        },
    };

    bitmap_handle = CreateDIBSection(
        bitmap_device_ctx,
        &bitmap_info,
        DIB_RGB_COLORS,
        &bitmap_mem,
        0, 0 
    );
}

static void update_window(
    HDC device_ctx, 
    int x, 
    int y, 
    int width, 
    int height
) {
    StretchDIBits(
        device_ctx,
        x, y, width, height,
        x, y, width, height,
        bitmap_mem,
        &bitmap_info,
        DIB_RGB_COLORS, 
        SRCCOPY
    );
}

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
            update_window(device_ctx, x, y, width, height);

            EndPaint(window_handle, &paint);
        } break;
        case WM_SIZE: {
            RECT client_rect;
            GetClientRect(window_handle, &client_rect);
            int width = client_rect.right - client_rect.left;
            int height = client_rect.bottom - client_rect.top;
            resize_DIB_section(width, height);
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

    running = true;
    while (running) {
        MSG message;
        if (GetMessage(&message, 0, 0, 0) <= 0) break;
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
