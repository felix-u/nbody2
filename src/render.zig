const c = @cImport({
    @cInclude("raylib.h");
});

const State = @import("main.zig").State;

inline fn colour(comptime rgb: u24) c.Color {
    return .{
        .r = (rgb & 0xff0000) >> 16,
        .g = (rgb & 0x00ff00) >> 8,
        .b = (rgb & 0x0000ff),
        .a = 0xff,
    };
}

pub const c_white = colour(0xffffff);
pub const c_black = colour(0x000000);
pub const c_grey = colour(0x707070);
pub const c_red = colour(0xff0000);
pub const c_blue = colour(0x5599ff);

pub fn init(width: c_int, height: c_int) void {
    c.SetConfigFlags(c.FLAG_MSAA_4X_HINT);
    c.SetTargetFPS(State.target_fps);
    c.InitWindow(width, height, "nbody2");
}

pub fn deinit() void {
    c.CloseWindow();
}

pub fn shouldQuit() bool {
    return c.WindowShouldClose();
}

pub fn beginFrame() void {
    c.BeginDrawing();
    c.ClearBackground(c_black);
}

pub fn endFrame() void {
    c.EndDrawing();
}
