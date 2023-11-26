#define BASE_IMPLEMENTATION
#include "base.h"

#include <dsound.h>
#include <windows.h>
#include <xinput.h>

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

static LPDIRECTSOUNDBUFFER secondary_buffer;

static void init_dsound(
    const HWND window_handle, 
    const usize samples_per_second,
    const usize buffer_size
) {
    LPDIRECTSOUND dsound;
    if (!SUCCEEDED(DirectSoundCreate(0, &dsound, 0))) {
        // TODO: handle error
        return;
    }

    const IDirectSoundVtbl *dsound_vt = dsound->lpVtbl;

    if (!SUCCEEDED(
        dsound_vt->SetCooperativeLevel(dsound, window_handle, DSSCL_PRIORITY))
    ) {
        // TODO: handle error
        return;
    }

    const DSBUFFERDESC buffer_desc = {
        .dwSize = sizeof(buffer_desc),
        .dwFlags = DSBCAPS_PRIMARYBUFFER,
        .dwBufferBytes = buffer_size,
    };

    LPDIRECTSOUNDBUFFER primary_buffer;
    if (!SUCCEEDED(
        dsound_vt->CreateSoundBuffer(dsound, &buffer_desc, &primary_buffer, 0))
    ) {
        // TODO: handle error
        return;
    }

    const IDirectSoundBufferVtbl *pbuffer_vt = primary_buffer->lpVtbl;

    WAVEFORMATEX wave_format = {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = samples_per_second,
        .nAvgBytesPerSec = 
            wave_format.nSamplesPerSec * wave_format.nBlockAlign,
        .nBlockAlign = wave_format.nChannels * wave_format.wBitsPerSample / 8,
        .wBitsPerSample = 16,
    };

    if (!SUCCEEDED(pbuffer_vt->SetFormat(primary_buffer, &wave_format))) {
        // TODO: handle error
        return;
    }

    const DSBUFFERDESC buffer_desc2 = {
        .dwSize = sizeof(buffer_desc2),
        .dwBufferBytes = buffer_size,
        .lpwfxFormat = &wave_format,
    };

    if (!SUCCEEDED(dsound_vt->CreateSoundBuffer(
        dsound, &buffer_desc2, &secondary_buffer, 0))
    ) {
        // TODO: handle error
        return;
    }
}

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
    Screen_Buffer *const buffer, 
    const int x_offset, 
    const int y_offset
) {
    u8 *row = buffer->mem;
    for (int y = 0; y < buffer->height; y += 1, row += buffer->stride) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1) {
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
    Screen_Buffer *const buffer,
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
        0, 0, buffer->width, buffer->height,
        buffer->mem,
        &buffer->info,
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
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            const u32  vkcode   = w_param;
            const bool was_down = ((l_param & ((u32)1 << 30)) != 0);
            const bool is_down  = ((l_param & ((u32)1 << 31)) == 0);

            if (is_down && was_down) break;

            const bool alt_key_down = (l_param & ((u32)1 << 29)) != 0;
            if (alt_key_down && vkcode == VK_F4) {
                running = false;
                break;
            }

            if (vkcode == 'W') {
                OutputDebugStringA("W ");
                if (is_down) OutputDebugStringA("is_down ");
                if (was_down) OutputDebugStringA("was_down ");
                OutputDebugStringA("\n");
            } else if (vkcode == 'A') {
            } else if (vkcode == 'S') {
            } else if (vkcode == 'D') {
            } else if (vkcode == 'Q') {
            } else if (vkcode == 'E') {
            } else if (vkcode == VK_ESCAPE) {
                running = false;
                break;
            } else if (vkcode == VK_SPACE) {
            }
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
                &screen_buffer, 
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

    const WNDCLASSA window_class = {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = main_window_callback,
        .hInstance = instance,
        .lpszClassName = "nbody2_window_class",
    };

    if (!RegisterClass(&window_class)) {
        // TODO: handle error
    }

    const HWND window_handle = CreateWindowEx(
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

    const usize hz = 256;

    const usize bytes_per_sample = sizeof(i16) * 2;
    const usize samples_per_second = 48000;
    const usize secondary_buffer_size = samples_per_second * bytes_per_sample;

    const usize square_wave_period = samples_per_second / hz;
    const usize half_square_wave_period = square_wave_period / 2;

    usize running_sample_i = 0;

    init_dsound(window_handle, samples_per_second, secondary_buffer_size);
    const IDirectSoundBufferVtbl *sbuffer_vt = secondary_buffer->lpVtbl;
    sbuffer_vt->Play(secondary_buffer, 0, 0, DSBPLAY_LOOPING);

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

        for (
            DWORD controller_i = 0; 
            controller_i < XUSER_MAX_COUNT; 
            controller_i += 1
        ) {
            XINPUT_STATE controller_state;
            if (
                XInputGetState(controller_i, &controller_state) 
                    == ERROR_SUCCESS
            ) {
                const XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

                // const bool d_left  = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                // const bool d_down  = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                // const bool d_up    = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                // const bool d_right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                // const bool start = pad->wButtons & XINPUT_GAMEPAD_START;
                // const bool back  = pad->wButtons & XINPUT_GAMEPAD_BACK;

                // const bool left_shoulder = 
                //     pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                // const bool right_shoulder 
                //     = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

                const bool a = pad->wButtons & XINPUT_GAMEPAD_A;
                const bool b = pad->wButtons & XINPUT_GAMEPAD_B;
                const bool x = pad->wButtons & XINPUT_GAMEPAD_X;
                const bool y = pad->wButtons & XINPUT_GAMEPAD_Y;

                // const i16 lstick_x = pad->sThumbLX;
                // const i16 lstick_y = pad->sThumbLY;
                
                if (a) y_offset += 2;
                if (b) x_offset += 2;
                if (x) x_offset -= 2;
                if (y) y_offset -= 2;

                if (x_offset || y_offset) {
                    XINPUT_VIBRATION vibration = {
                        .wLeftMotorSpeed = 20000,
                        .wRightMotorSpeed = 20000,
                    };
                    XInputSetState(0, &vibration);
                }
            } else {
                // Controller unavailable
            }
        }

        DWORD play_cursor;
        DWORD write_cursor;

        if (!SUCCEEDED(sbuffer_vt->GetCurrentPosition(
            secondary_buffer, 
            &play_cursor, 
            &write_cursor))
        ) {
            // TODO: handle error
            return 1;
        }

        void *region1 = NULL;
        DWORD region1_size;
        void *region2 = NULL;
        DWORD region2_size;

        const DWORD byte_to_lock = 
            (running_sample_i * bytes_per_sample) % secondary_buffer_size;
        const DWORD bytes_to_write = byte_to_lock > play_cursor
            ? secondary_buffer_size - byte_to_lock + play_cursor
            : play_cursor - byte_to_lock;

        if (!SUCCEEDED(sbuffer_vt->Lock(
            secondary_buffer,
            byte_to_lock,
            bytes_to_write,
            region1,
            &region1_size,
            region2,
            &region2_size,
            0))
        ) {
            // TODO: handle error
            return 1;
        }

        // const DWORD region1_sample_count = region1_size / bytes_per_sample;
        // const DWORD region2_sample_count = region2_size / bytes_per_sample;
        
        const i16 tone_volume = 5000;

        i16 *sample_out = region1;
        for (DWORD sample_i = 0; sample_i < region1_size; sample_i += 1) {
            const i16 sample_value = 
                (running_sample_i / half_square_wave_period) % 2
                    ? tone_volume
                    : -tone_volume;
            *(sample_out++) = sample_value;
            *(sample_out++) = sample_value;
            running_sample_i += 1;
        }

        sample_out = region2;
        for (DWORD sample_i = 0; sample_i < region2_size; sample_i += 1) {
            const i16 sample_value = 
                (running_sample_i / half_square_wave_period) % 2
                    ? tone_volume
                    : -tone_volume;
            *(sample_out++) = sample_value;
            *(sample_out++) = sample_value;
            running_sample_i += 1;
        }

        render_gradient(&screen_buffer, x_offset, y_offset);

        HDC device_ctx = GetDC(window_handle);
        const Window_Dimension dim = get_window_dimension(window_handle);
        blit_buffer(
            device_ctx, 
            dim.width, 
            dim.height,
            &screen_buffer, 
            0, 0, dim.width, dim.height
        );
        ReleaseDC(window_handle, device_ctx);
    }

    return 0;
}
