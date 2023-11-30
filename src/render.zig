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

pub const Camera = struct {
    active: bool = false,
    x: f32 = 0,
    y: f32 = 0,
    zoom: f32 = 1,
};

pub fn screenFromNormal(state: State, n: f32) f32 {
    return n * State.screen_scale * state.camera.zoom;
}

pub fn normalFromScreen(state: State, n: f32) f32 {
    return n / State.screen_scale / state.camera.zoom;
}

pub fn init(state: *State) void {
    c.SetConfigFlags(c.FLAG_MSAA_4X_HINT);
    c.SetTargetFPS(State.target_fps);
    c.InitWindow(State.screen_width, State.screen_height, "nbody2");

    state.camera.zoom = 1;
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

pub fn drawAll(state: *State) void {
    state.delta = c.GetFrameTime();
    drawBodies(state.*);
    drawCreator(state.*);
    drawFpsText(state.*);
}

pub fn drawBodies(state: State) void {
    for (0..state.bodies.len) |body_i| {
        const body = state.bodies.bodies[body_i];
        const x_coord: c_int = @intFromFloat(screenFromNormal(state, body.x));
        const y_coord: c_int = @intFromFloat(screenFromNormal(state, body.y));
        c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
    }
}

pub fn drawCreator(state: State) void {
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

    const start_x = screenFromNormal(state, state.creator.body.x);
    const start_y = screenFromNormal(state, state.creator.body.y);
    const end_x = screenFromNormal(state, state.creator.body.x +
        state.creator.x_displacement);
    const end_y = screenFromNormal(state, state.creator.body.y +
        state.creator.y_displacement);
    c.DrawLineEx(
        .{ .x = start_x, .y = start_y },
        .{ .x = end_x, .y = end_y },
        4,
        c_grey,
    );

    c.DrawRing(
        .{
            .x = screenFromNormal(state, state.creator.body.x),
            .y = screenFromNormal(state, state.creator.body.y),
        },
        state.creator.body.radius,
        state.creator.body.radius + 5,
        0,
        360,
        0,
        c_grey,
    );
}

pub fn drawFpsText(state: State) void {
    var fps_text_buf: [64]u8 = undefined;
    const fps_text = std.fmt.bufPrint(
        &fps_text_buf,
        "{d}/{d}fps x:{} y:{}{c}",
        .{ c.GetFPS(), State.target_fps, state.camera.x, state.camera.y, 0 },
    ) catch "invalid";
    c.DrawText(
        @ptrCast(fps_text),
        State.screen_width / 40,
        State.screen_height / 40,
        State.screen_width / 40,
        c_grey,
    );
}
