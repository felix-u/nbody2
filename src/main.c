#include "base.c"

#include "raylib.h"
#include "math.h"

#include "game.c"

int main(void) {
    Context ctx = {
        .arena = arena_init(16 * 1024 * 1024),
        .name = "nbody2",
        .width = 2560,
        .height = 1440,
        .fps = 60,
    };
    game_init(&ctx);

    while (!WindowShouldClose()) {
        game_frame_begin(&ctx);
        game_update_and_render(&ctx);
        game_frame_end(&ctx);
    }

    game_deinit(&ctx);
}
