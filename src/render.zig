const c = @cImport({
    @cInclude("raylib.h");
});

const std = @import("std");
const main = @import("main.zig");
const State = main.State;
const Creator = main.Creator;

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

pub fn init() void {
    c.SetConfigFlags(c.FLAG_MSAA_4X_HINT);
    c.SetTargetFPS(State.target_fps);
    c.InitWindow(State.screen_width, State.screen_height, "nbody2");
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

pub fn drawAll(state: *main.State) void {
    state.delta = c.GetFrameTime();
    drawBodies(state.*);
    drawCreator(state.*);
    drawFpsText();
}

pub fn drawBodies(state: main.State) void {
    for (0..state.bodies.len) |body_i| {
        const body = state.bodies.bodies[body_i];
        const x_coord: c_int = @intFromFloat(body.x * State.screen_scale);
        const y_coord: c_int = @intFromFloat(body.y * State.screen_scale);
        c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
    }
}

pub fn drawCreator(state: main.State) void {
    if (!state.creator.active) {
        c.DrawRing(
            state.mouse_pos,
            state.cursor_radius,
            state.cursor_radius + 5,
            0,
            360,
            0,
            c_blue,
        );
        return;
    }

    const start_x = state.creator.body.x * State.screen_scale;
    const start_y = state.creator.body.y * State.screen_scale;
    const end_x = State.screen_scale *
        (state.creator.body.x + state.creator.x_displacement);
    const end_y = State.screen_scale *
        (state.creator.body.y + state.creator.y_displacement);
    c.DrawLineEx(
        .{ .x = start_x, .y = start_y },
        .{ .x = end_x, .y = end_y },
        4,
        c_grey,
    );

    const x_coord = state.creator.body.x * State.screen_scale;
    const y_coord = state.creator.body.y * State.screen_scale;
    c.DrawRing(
        .{ .x = x_coord, .y = y_coord },
        state.creator.body.radius,
        state.creator.body.radius + 5,
        0,
        360,
        0,
        c_grey,
    );
}

pub fn drawFpsText() void {
    var fps_text_buf: [64]u8 = undefined;
    const fps_text = std.fmt.bufPrint(
        &fps_text_buf,
        "{d}/{d}fps{c}",
        .{ c.GetFPS(), State.target_fps, 0 },
    ) catch "invalid";
    c.DrawText(
        @ptrCast(fps_text),
        State.screen_width / 40,
        State.screen_height / 40,
        State.screen_width / 40,
        c_grey,
    );
}
