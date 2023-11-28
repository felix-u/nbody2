const c = @cImport({
    @cInclude("raylib.h");
});

const std = @import("std");

inline fn colour(comptime rgb: u24) c.Color {
    return .{
        .r = (rgb & 0xff0000) >> 16,
        .g = (rgb & 0x00ff00) >> 8,
        .b = (rgb & 0x0000ff),
        .a = 0xff,
    };
}
const colour_white = colour(0xffffff);
const colour_black = colour(0x000000);
const colour_grey = colour(0x707070);
const colour_red = colour(0xff0000);
const colour_blue = colour(0x5599ff);

const screen_scale = 1440;
const screen_height = screen_scale;
const screen_width = 2560;

const Body = struct {
    mass: f32 = undefined,
    radius: f32 = undefined,
    x: f32 = undefined,
    y: f32 = undefined,
    colour: c.Color = colour_white,
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

pub fn main() void {
    c.InitWindow(screen_width, screen_height, "nbody2");
    defer c.CloseWindow();

    const target_fps: comptime_float = 360;
    c.SetTargetFPS(target_fps);

    const cursor_radius_min = 10;
    const cursor_radius_max = 100;
    var cursor_radius: f32 = cursor_radius_min * 4;

    var bodies = Bodies{};

    while (!c.WindowShouldClose()) {
        c.BeginDrawing();
        defer c.EndDrawing();
        c.ClearBackground(colour_black);

        // Update ---
        const delta = c.GetFrameTime();

        const mouse_pos = c.GetMousePosition();
        const mouse_wheel = c.GetMouseWheelMove();
        cursor_radius = cursor_radius + mouse_wheel * 10;
        if (cursor_radius > cursor_radius_max) {
            cursor_radius = cursor_radius_max;
        } else if (cursor_radius < cursor_radius_min) {
            cursor_radius = cursor_radius_min;
        }

        const G_constant = 3 * 10e-9 / target_fps;
        if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT) and
            bodies.len < bodies_cap)
        {
            bodies.add(.{
                .mass = std.math.pow(f32, cursor_radius, 3) * G_constant,
                .radius = cursor_radius,
                .x = mouse_pos.x / screen_scale,
                .y = mouse_pos.y / screen_scale,
                .vel_x = if (cursor_radius == cursor_radius_min) -0.3 else 0,
            });
        }

        for (0..bodies.len) |body_i| {
            const body = &bodies.bodies[body_i];
            for (0..bodies.len) |body_cmp_i| {
                const body_cmp = &bodies.bodies[body_cmp_i];
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
                    .width = screen_scale,
                    .height = screen_scale,
                },
            )) {
                const x = body.x * screen_scale;
                const y = body.y * screen_scale;

                if (x - body.radius < 0) {
                    body.x = body.radius / screen_scale;
                    body.vel_x = -body.vel_x * dampen;
                } else if (x + body.radius > screen_width) {
                    body.x = 1 - body.radius / screen_scale;
                    body.vel_x = -body.vel_x * dampen;
                }

                if (y - body.radius < 0) {
                    body.y = body.radius / screen_scale;
                    body.vel_y = -body.vel_y * dampen;
                } else if (y + body.radius > screen_height) {
                    body.y = 1 - body.radius / screen_scale;
                    body.vel_y = -body.vel_y * dampen;
                }
            }

            body.x += body.vel_x * delta;
            body.y += body.vel_y * delta;
        }

        // Render ---

        var fps_text_buf: [64]u8 = undefined;
        const fps_text = std.fmt.bufPrint(
            &fps_text_buf,
            "{d}/{d}fps{c}",
            .{ c.GetFPS(), target_fps, 0 },
        ) catch "invalid";
        c.DrawText(
            @ptrCast(fps_text),
            screen_width / 40,
            screen_height / 40,
            screen_width / 40,
            colour_grey,
        );

        c.DrawRing(
            mouse_pos,
            cursor_radius,
            cursor_radius + 5,
            0,
            360,
            0,
            colour_blue,
        );

        for (0..bodies.len) |body_i| {
            const body = bodies.bodies[body_i];
            const x_coord: c_int = @intFromFloat(body.x * screen_scale);
            const y_coord: c_int = @intFromFloat(body.y * screen_scale);
            c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
        }
    }
}
