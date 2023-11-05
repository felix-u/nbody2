#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-shell-protocol.c"

#define BASE_IMPLEMENTATION
#include "base.h"

void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

int create_shm_file(void) {
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

int allocate_shm_file(size_t size) {
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

typedef struct wl_buffer Wl_Buffer;
typedef struct wl_buffer_listener Wl_Buffer_Listener;
typedef struct wl_compositor Wl_Compositor;
typedef struct wl_display Wl_Display;
typedef struct wl_registry Wl_Registry;
typedef struct wl_registry_listener Wl_Registry_Listener;
typedef struct wl_shm Wl_Shared_Mem;
typedef struct wl_shm_pool Wl_Shared_Mem_Pool;
typedef struct wl_surface Wl_Surface;
typedef struct xdg_surface XDG_Surface;
typedef struct xdg_surface_listener XDG_Surface_Listener;
typedef struct xdg_toplevel XDG_Toplevel;
typedef struct xdg_wm_base XDG_WM_Base;
typedef struct xdg_wm_base_listener XDG_WM_Base_Listener;

typedef struct Client_State {
    Wl_Display *wl_display;
    Wl_Registry *wl_registry;
    Wl_Shared_Mem *wl_shm;
    Wl_Compositor *wl_compositor;
    XDG_WM_Base *xdg_wm_base;
    Wl_Surface *wl_surface;
    XDG_Surface *xdg_surface;
    XDG_Toplevel *xdg_toplevel;
} Client_State;

void wl_buffer_release(void *data, Wl_Buffer *wl_buffer) {
    (void)data;
    wl_buffer_destroy(wl_buffer);
}

const Wl_Buffer_Listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

Wl_Buffer *draw_frame(Client_State *state) {
    const usize width = 640, height = 480;
    usize stride = width * 4;
    usize size = stride * height;

    int fd = allocate_shm_file(size);
    if (fd == -1) return NULL;

    u32 *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    Wl_Shared_Mem_Pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
    Wl_Buffer *buffer = wl_shm_pool_create_buffer(
        pool, 
        0, 
        width, 
        height, 
        stride, 
        WL_SHM_FORMAT_XRGB8888
    );
    wl_shm_pool_destroy(pool);
    close(fd);

    for (usize y = 0; y < height; ++y) {
        for (usize x = 0; x < width; ++x) {
            if ((x + y / 8 * 8) % 16 < 8) data[y * width + x] = 0xFF666666;
            else data[y * width + x] = 0xFFEEEEEE;
        }
    }

    munmap(data, size);
    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}

void xdg_surface_configure(void *data, XDG_Surface *xdg_surface, u32 serial) {
    Client_State *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    Wl_Buffer *buffer = draw_frame(state);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

const XDG_Surface_Listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

void xdg_wm_base_ping(void *data, XDG_WM_Base *xdg_wm_base, u32 serial) {
    (void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

const XDG_WM_Base_Listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

void registry_global(
    void *data, 
    Wl_Registry *wl_registry,
    u32 name, 
    const char *interface, 
    u32 version
) {
    (void)version;
    Client_State *state = data;
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->wl_shm = 
            wl_registry_bind( wl_registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor = 
            wl_registry_bind( wl_registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = 
            wl_registry_bind( wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(
            state->xdg_wm_base, 
            &xdg_wm_base_listener, 
            state
        );
    }
}

void registry_global_remove(void *data, Wl_Registry *wl_registry, u32 name) {
    (void)data;
    (void)wl_registry;
    (void)name;
}

const Wl_Registry_Listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int main(void) {
    Client_State state = { 0 };
    state.wl_display = wl_display_connect(NULL);
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = 
        xdg_wm_base_get_xdg_surface( state.xdg_wm_base, state.wl_surface);
    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Example client");
    wl_surface_commit(state.wl_surface);

    while (wl_display_dispatch(state.wl_display)) {
        // TODO
    }

    return 0;
}
