const c = @cImport({
    @cInclude("raylib.h");
});

const std = @import("std");

const logical_width = 640;
const logical_height = 360;

pub fn main() void {
    const screen_width = logical_width * 4;
    const screen_height = logical_height * 4;

    c.InitWindow(screen_width, screen_height, "nbody2");
    defer c.CloseWindow();

    c.SetTargetFPS(60);

    while (!c.WindowShouldClose()) {
        c.BeginDrawing();
        c.ClearBackground(c.RAYWHITE);
        c.DrawText("nbody2", screen_width / 2, screen_height / 2, 80, c.LIGHTGRAY);
        c.EndDrawing();
    }
}
