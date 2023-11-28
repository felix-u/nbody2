const c = @cImport({
    @cInclude("raylib.h");
});

const std = @import("std");
const render = @import("render.zig");

const screen_scale = 1440;
const screen_height = screen_scale;
const screen_width = 2560;

const Body = struct {
    mass: f32 = undefined,
    radius: f32 = undefined,
    x: f32 = undefined,
    y: f32 = undefined,
    colour: c.Color = render.c_white,
    vel_x: f32 = 0,
    vel_y: f32 = 0,
};

const bodies_cap = 100;
const Bodies = struct {
    bodies: [bodies_cap]Body = [_]Body{.{}} ** bodies_cap,
    len: usize = 0,

    fn add(self: *Bodies, body: Body) void {
        self.bodies[self.len] = body;
        self.len += 1;
    }
};

const Creator = struct {
    active: bool = false,
    body: Body = .{},
    x_displacement: f32 = 0,
    y_displacement: f32 = 0,
};

fn drawCreator(creator: Creator) void {
    if (!creator.active) return;
    const start_x = creator.body.x * screen_scale;
    const start_y = creator.body.y * screen_scale;
    const end_x = (creator.body.x + creator.x_displacement) * screen_scale;
    const end_y = (creator.body.y + creator.y_displacement) * screen_scale;
    c.DrawLineEx(
        .{ .x = start_x, .y = start_y },
        .{ .x = end_x, .y = end_y },
        4,
        render.c_grey,
    );

    const x_coord = creator.body.x * screen_scale;
    const y_coord = creator.body.y * screen_scale;
    c.DrawRing(
        .{ .x = x_coord, .y = y_coord },
        creator.body.radius,
        creator.body.radius + 5,
        0,
        360,
        0,
        render.c_grey,
    );
}

pub fn main() void {
    var state = State{};

    render.init(screen_width, screen_height);
    defer render.deinit();

    while (!render.shouldQuit()) {
        render.beginFrame();
        defer render.endFrame();

        // Update ---
        state.step();

        // Render ---

        var fps_text_buf: [64]u8 = undefined;
        const fps_text = std.fmt.bufPrint(
            &fps_text_buf,
            "{d}/{d}fps{c}",
            .{ c.GetFPS(), State.target_fps, 0 },
        ) catch "invalid";
        c.DrawText(
            @ptrCast(fps_text),
            screen_width / 40,
            screen_height / 40,
            screen_width / 40,
            render.c_grey,
        );

        if (!state.creator.active) c.DrawRing(
            state.mouse_pos,
            state.cursor_radius,
            state.cursor_radius + 5,
            0,
            360,
            0,
            render.c_blue,
        );

        for (0..state.bodies.len) |body_i| {
            const body = state.bodies.bodies[body_i];
            const x_coord: c_int = @intFromFloat(body.x * screen_scale);
            const y_coord: c_int = @intFromFloat(body.y * screen_scale);
            c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
        }

        drawCreator(state.creator);
    }
}

const cursor_radius_min = 10;
const cursor_radius_max = 100;

pub const State = struct {
    cursor_radius: f32 = cursor_radius_min * 3,
    mouse_pos: c.Vector2 = undefined,
    creator: Creator = .{},
    bodies: Bodies = .{},

    pub const target_fps: comptime_float = 360;
    pub const G_constant = 3 * 10e-9 / target_fps;

    pub fn step(self: *State) void {
        const delta = c.GetFrameTime();

        self.mouse_pos = c.GetMousePosition();
        const mouse_wheel = c.GetMouseWheelMove();
        self.cursor_radius += mouse_wheel * 10;
        if (self.cursor_radius > cursor_radius_max) {
            self.cursor_radius = cursor_radius_max;
        } else if (self.cursor_radius < cursor_radius_min) {
            self.cursor_radius = cursor_radius_min;
        }

        if (self.creator.active) {
            if (c.IsMouseButtonDown(c.MOUSE_BUTTON_RIGHT)) {
                self.creator.x_displacement = (self.mouse_pos.x / screen_scale) - self.creator.body.x;
                self.creator.y_displacement = (self.mouse_pos.y / screen_scale) - self.creator.body.y;
            } else if (c.IsMouseButtonReleased(c.MOUSE_BUTTON_RIGHT)) {
                self.creator.active = false;
                self.creator.body.vel_x = self.creator.x_displacement;
                self.creator.body.vel_y = self.creator.y_displacement;
                self.bodies.add(self.creator.body);
            }

            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT)) {
                self.creator.body.vel_x = self.creator.x_displacement;
                self.creator.body.vel_y = self.creator.y_displacement;
                self.bodies.add(self.creator.body);
            }
        } else {
            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_RIGHT)) {
                self.creator.active = true;
                self.creator.body.x = self.mouse_pos.x / screen_scale;
                self.creator.body.y = self.mouse_pos.y / screen_scale;
                self.creator.body.mass = std.math.pow(f32, self.cursor_radius, 3) * G_constant;
                self.creator.body.radius = self.cursor_radius;
            }

            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT) and
                self.bodies.len < bodies_cap)
            {
                self.bodies.add(.{
                    .mass = std.math.pow(f32, self.cursor_radius, 3) * G_constant,
                    .radius = self.cursor_radius,
                    .x = self.mouse_pos.x / screen_scale,
                    .y = self.mouse_pos.y / screen_scale,
                });
            }
        }

        for (0..self.bodies.len) |body_i| {
            const body = &self.bodies.bodies[body_i];
            for (0..self.bodies.len) |body_cmp_i| {
                const body_cmp = &self.bodies.bodies[body_cmp_i];
                if (body_cmp_i == body_i) continue;

                const x_dist = body.x - body_cmp.x;
                const y_dist = body.y - body_cmp.y;
                const dist = std.math.sqrt(
                    std.math.pow(f32, x_dist, 2) +
                        std.math.pow(f32, y_dist, 2),
                );

                const colliding = c.CheckCollisionCircles(
                    .{
                        .x = body.x * screen_scale,
                        .y = body.y * screen_scale,
                    },
                    body.radius / 2,
                    .{
                        .x = body_cmp.x * screen_scale,
                        .y = body_cmp.y * screen_scale,
                    },
                    body_cmp.radius / 2,
                );
                if (colliding) continue;

                const force = -1 *
                    body.mass * body_cmp.mass /
                    std.math.pow(f32, dist, 2);
                const force_x = force * (x_dist / dist);
                const force_y = force * (y_dist / dist);

                const accel_x = force_x / body.mass;
                const accel_y = force_y / body.mass;

                body.vel_x += accel_x;
                body.vel_y += accel_y;
            }

            const dampen = 0.3;

            if (c.CheckCollisionCircleRec(
                .{ .x = body.x * screen_scale, .y = body.y * screen_scale },
                body.radius,
                .{
                    .x = 0,
                    .y = 0,
                    .width = screen_width,
                    .height = screen_height,
                },
            )) {
                const x = body.x * screen_scale;
                const y = body.y * screen_scale;

                if (x - body.radius < 0) {
                    body.x = body.radius / screen_scale;
                    body.vel_x = -body.vel_x * dampen;
                } else if (x + body.radius > screen_width) {
                    body.x = (screen_width - body.radius) / screen_scale;
                    body.vel_x = -body.vel_x * dampen;
                }

                if (y - body.radius < 0) {
                    body.y = body.radius / screen_scale;
                    body.vel_y = -body.vel_y * dampen;
                } else if (y + body.radius > screen_height) {
                    body.y = (screen_height - body.radius) / screen_scale;
                    body.vel_y = -body.vel_y * dampen;
                }
            }

            body.x += body.vel_x * delta;
            body.y += body.vel_y * delta;
        }
    }
};
