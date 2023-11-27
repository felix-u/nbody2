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

const logical_width = 640;
const logical_height = 360;

const Body = struct {
    mass: f32 = undefined,
    radius: f32 = undefined,
    x: f32 = undefined,
    y: f32 = undefined,
    colour: c.Color = colour_black,
    vel_x: f32 = 0,
    vel_y: f32 = 0,
};

pub fn main() void {
    const screen_width = logical_width * 4;
    const screen_height = logical_height * 4;

    c.InitWindow(screen_width, screen_height, "nbody2");
    defer c.CloseWindow();

    c.SetTargetFPS(60);

    const cursor_radius_min = 10;
    const cursor_radius_max = 100;
    var cursor_radius: f32 = cursor_radius_min * 4;

    const bodies_cap = 100;
    var bodies = [_]Body{.{}} ** bodies_cap;
    var bodies_len: usize = 0;

    while (!c.WindowShouldClose()) {
        c.BeginDrawing();
        defer c.EndDrawing();
        c.ClearBackground(colour_white);

        // Update ---
        const mouse_pos = c.GetMousePosition();
        const mouse_wheel = c.GetMouseWheelMove();
        cursor_radius = cursor_radius + mouse_wheel * 10;
        if (cursor_radius > cursor_radius_max) {
            cursor_radius = cursor_radius_max;
        } else if (cursor_radius < cursor_radius_min) {
            cursor_radius = cursor_radius_min;
        }

        const G_constant = 3 * 10e-12;
        if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT)) {
            bodies[bodies_len] = .{
                .mass = std.math.pow(f32, cursor_radius, 3) * G_constant,
                .radius = cursor_radius,
                .x = mouse_pos.x / screen_width,
                .y = mouse_pos.y / screen_height,
            };
            bodies_len += 1;
        }

        for (0..bodies_len) |body_i| {
            const body = &bodies[body_i];
            for (0..bodies_len) |body_cmp_i| {
                const body_cmp = &bodies[body_cmp_i];
                if (body_cmp_i == body_i) continue;

                const x_dist = body.x - body_cmp.x;
                const y_dist = body.y - body_cmp.y;
                const dist = std.math.sqrt(
                    std.math.pow(f32, x_dist, 2) +
                        std.math.pow(f32, y_dist, 2),
                );

                const colliding = c.CheckCollisionCircles(
                    .{
                        .x = body.x * screen_width,
                        .y = body.y * screen_height,
                    },
                    body.radius,
                    .{
                        .x = body_cmp.x * screen_width,
                        .y = body_cmp.y * screen_height,
                    },
                    body_cmp.radius,
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

            if (body.x < 0) {
                body.x = 0;
                body.vel_x = -body.vel_x * dampen;
            } else if (body.x > 1) {
                body.x = 1;
                body.vel_x = -body.vel_x * dampen;
            }

            if (body.y < 0) {
                body.y = 0;
                body.vel_y = -body.vel_y * dampen;
            } else if (body.y > 1) {
                body.y = 1;
                body.vel_y = -body.vel_y * dampen;
            }

            body.x += body.vel_x;
            body.y += body.vel_y;
        }

        // Render ---

        c.DrawRing(
            mouse_pos,
            cursor_radius,
            cursor_radius + 3,
            0,
            360,
            0,
            colour_black,
        );

        for (bodies) |body| {
            const x_coord: c_int = @intFromFloat(body.x * screen_width);
            const y_coord: c_int = @intFromFloat(body.y * screen_height);
            c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
        }
    }
}
