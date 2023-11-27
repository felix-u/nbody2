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
    mass: f64,
    radius: f32,
    x: f64,
    y: f64,
    colour: c.Color,
    vel_x: f64 = 0,
    vel_y: f64 = 0,
};

pub fn main() void {
    const screen_width = logical_width * 4;
    const screen_height = logical_height * 4;

    c.InitWindow(screen_width, screen_height, "nbody2");
    defer c.CloseWindow();

    c.SetTargetFPS(60);

    var bodies = [_]Body{
        .{
            .mass = 50,
            .radius = 20,
            .x = 0.1,
            .y = 0.1,
            .colour = colour_black,
        },
        .{
            .mass = 100,
            .radius = 40,
            .x = 0.7,
            .y = 0.6,
            .colour = colour_black,
        },
    };

    while (!c.WindowShouldClose()) {
        c.BeginDrawing();
        defer c.EndDrawing();
        c.ClearBackground(colour_white);

        // Update ---

        const G_constant = 0.000001;
        const proximity_threshold = 0.1;

        for (&bodies, 0..bodies.len) |*body, body_i| {
            for (bodies, 0..bodies.len) |body_cmp, body_cmp_i| {
                if (body_cmp_i == body_i) continue;

                const x_dist = body.x - body_cmp.x;
                const y_dist = body.y - body_cmp.y;
                var dist = std.math.sqrt(
                    std.math.pow(f64, x_dist, 2) +
                        std.math.pow(f64, y_dist, 2),
                );

                if (dist < proximity_threshold) continue;

                if (x_dist > 0 or y_dist > 0) dist = -dist;

                const force = G_constant * body.mass * body_cmp.mass / dist;
                const force_x = force * (x_dist / dist);
                const force_y = force * (y_dist / dist);

                const accel_x = force_x / body.mass;
                const accel_y = force_y / body.mass;

                body.vel_x += accel_x;
                body.vel_y += accel_y;
            }
            body.x += body.vel_x;
            body.y += body.vel_y;
        }

        // Render ---

        for (bodies) |body| {
            const x_coord: c_int = @intFromFloat(body.x * screen_width);
            const y_coord: c_int = @intFromFloat(body.y * screen_height);
            c.DrawCircle(x_coord, y_coord, body.radius, body.colour);
        }
    }
}
