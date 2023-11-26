#define BASE_IMPLEMENTATION
#include "base.h"

#include <windows.h>

static bool running;

typedef struct Screen_Buffer {
    BITMAPINFO info;
    void *mem;
    int width;
    int height;
    int stride;
    int bytes_per_pixel;
} Screen_Buffer;

static Screen_Buffer screen_buffer = { .bytes_per_pixel = 4 };

typedef struct Window_Dimension {
    int width;
    int height;
} Window_Dimension;

static Window_Dimension get_window_dimension(const HWND window_handle) {
    RECT client_rect;
    GetClientRect(window_handle, &client_rect);
    return (Window_Dimension){
        .width = client_rect.right - client_rect.left,
        .height = client_rect.bottom - client_rect.top,
    };
}

static void render_gradient(
    const Screen_Buffer buffer, 
    const int x_offset, 
    const int y_offset
) {
    u8 *row = buffer.mem;
    for (int y = 0; y < buffer.height; y += 1, row += buffer.stride) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer.width; x += 1) {
            const u32 r = (u8)(x + x_offset);
            const u32 g = (u8)(y + y_offset);
            const u32 b = (u8)(x_offset);
            const u32 pad = 0x00;

            *(pixel++) = (pad << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

static void resize_DIB_section(
    Screen_Buffer *const buffer, 
    const int width, 
    const int height
) {
    if (buffer->mem) VirtualFree(buffer->mem, 0, MEM_RELEASE);

    buffer->width = width;
    buffer->height = height;

    buffer->info = (BITMAPINFO){
        .bmiHeader = { 
            .biSize = sizeof(buffer->info.bmiHeader),
            .biWidth = buffer->width,
            .biHeight = -buffer->height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        },
    };

    const int bitmap_mem_size = 
        buffer->bytes_per_pixel * buffer->width * buffer->height;
    buffer->mem = VirtualAlloc(0, bitmap_mem_size, MEM_COMMIT, PAGE_READWRITE);

    buffer->stride = buffer->width * buffer->bytes_per_pixel;
}

static void blit_buffer(
    const HDC device_ctx, 
    const int window_width,
    const int window_height,
    const Screen_Buffer buffer,
    const int x, 
    const int y, 
    const int width, 
    const int height
) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    StretchDIBits(
        device_ctx,
        // x, y, width, height,
        // x, y, width, height,
        0, 0, window_width, window_height,
        0, 0, buffer.width, buffer.height,
        buffer.mem,
        &buffer.info,
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

            const Window_Dimension dim = get_window_dimension(window_handle);

            blit_buffer(
                device_ctx, 
                dim.width,
                dim.height,
                screen_buffer, 
                x, y, width, height
            );

            EndPaint(window_handle, &paint);
        } break;
        case WM_SIZE: {
            // const Window_Dimension dim = get_window_dimension(window_handle);
            // resize_DIB_section(&screen_buffer, dim.width, dim.height);
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

    resize_DIB_section(&screen_buffer, 640, 360);

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

        render_gradient(screen_buffer, x_offset, y_offset);

        HDC device_ctx = GetDC(window_handle);
        const Window_Dimension dim = get_window_dimension(window_handle);
        blit_buffer(
            device_ctx, 
            dim.width, 
            dim.height,
            screen_buffer, 
            0, 0, dim.width, dim.height
        );
        ReleaseDC(window_handle, device_ctx);

        x_offset += 1;
        y_offset += 1;
    }

    return 0;
}
