#define BASE_IMPLEMENTATION
#include "base.h"

#include <windows.h>

static bool running;

static BITMAPINFO bitmap_info;
static void *bitmap_mem;
static int bitmap_width;
static int bitmap_height;
static const int bytes_per_pixel = 4;

static void render_gradient(const int x_offset, const int y_offset) {
    const int bitmap_mem_size = bytes_per_pixel * bitmap_width * bitmap_height;
    bitmap_mem = VirtualAlloc(0, bitmap_mem_size, MEM_COMMIT, PAGE_READWRITE);

    const int stride = bitmap_width * bytes_per_pixel;
    u8 *row = bitmap_mem;
    for (int y = 0; y < bitmap_height; y += 1, row += stride) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < bitmap_width; x += 1) {
            const u32 r = (u8)(x + x_offset);
            const u32 g = (u8)(y + y_offset);
            const u32 b = (u8)(x_offset);
            const u32 pad = 0x00;

            *(pixel++) = (pad << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

static void resize_DIB_section(const int width, const int height) {
    if (bitmap_mem) VirtualFree(bitmap_mem, 0, MEM_RELEASE);

    bitmap_width = width;
    bitmap_height = height;

    bitmap_info = (BITMAPINFO){
        .bmiHeader = { 
            .biSize = sizeof(bitmap_info.bmiHeader),
            .biWidth = bitmap_width,
            .biHeight = -bitmap_height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        },
    };
}

static void update_window(
    HDC device_ctx, 
    RECT *client_rect,
    const int x, 
    const int y, 
    const int width, 
    const int height
) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    const int window_width = client_rect->right - client_rect->left;
    const int window_height = client_rect->bottom - client_rect->top;
    StretchDIBits(
        device_ctx,
        // x, y, width, height,
        // x, y, width, height,
        0, 0, bitmap_width, bitmap_height,
        0, 0, window_width, window_height,
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

            const int x = paint.rcPaint.left;
            const int y = paint.rcPaint.top;
            const int width = paint.rcPaint.right - paint.rcPaint.left;
            const int height = paint.rcPaint.bottom - paint.rcPaint.top;

            RECT client_rect;
            GetClientRect(window_handle, &client_rect);

            update_window(device_ctx, &client_rect, x, y, width, height);

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
        .style = CS_HREDRAW | CS_VREDRAW,
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

    int x_offset = 0;
    int y_offset = 0;

    running = true;
    while (running) {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) running = false;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        render_gradient(x_offset, y_offset);

        HDC device_ctx = GetDC(window_handle);
        RECT client_rect;
        GetClientRect(window_handle, &client_rect);
        const int window_width = client_rect.right - client_rect.left;
        const int window_height = client_rect.bottom - client_rect.top;
        update_window(device_ctx, &client_rect, 0, 0, window_width, window_height);
        ReleaseDC(window_handle, device_ctx);

        x_offset += 1;
        y_offset += 1;
    }

    return 0;
}
