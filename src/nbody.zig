const c = @cImport({
    @cInclude("raylib.h");
});

const main = @import("main.zig");
const render = @import("render.zig");
const std = @import("std");

const State = main.State;

pub const Body = struct {
    mass: f32 = undefined,
    radius: f32 = undefined,
    x: f32 = undefined,
    y: f32 = undefined,
    colour: c.Color = render.c_white,
    vel_x: f32 = 0,
    vel_y: f32 = 0,

    pub fn computeScreenCollision(self: *Body) void {
        if (!c.CheckCollisionCircleRec(
            .{
                .x = self.x * State.screen_scale,
                .y = self.y * State.screen_scale,
            },
            self.radius,
            .{
                .x = 0,
                .y = 0,
                .width = State.screen_width,
                .height = State.screen_height,
            },
        )) {
            return;
        }

        const x = self.x * State.screen_scale;
        const y = self.y * State.screen_scale;

        if (x - self.radius < 0) {
            self.x = self.radius / State.screen_scale;
            self.vel_x = -self.vel_x * State.screen_collision_dampen;
        } else if (x + self.radius > State.screen_width) {
            self.x = (State.screen_width - self.radius) / State.screen_scale;
            self.vel_x = -self.vel_x * State.screen_collision_dampen;
        }

        if (y - self.radius < 0) {
            self.y = self.radius / State.screen_scale;
            self.vel_y = -self.vel_y * State.screen_collision_dampen;
        } else if (y + self.radius > State.screen_height) {
            self.y = (State.screen_height - self.radius) / State.screen_scale;
            self.vel_y = -self.vel_y * State.screen_collision_dampen;
        }
    }
};

pub const Bodies = struct {
    bodies: [cap]Body = [_]Body{.{}} ** cap,
    len: usize = 0,

    pub const cap = 100;

    pub fn add(self: *Bodies, body: Body) void {
        if (self.len == Bodies.cap) return;
        self.bodies[self.len] = body;
        self.len += 1;
    }

    pub fn computeInteraction(
        self: *Bodies,
        delta: f32,
        i: usize,
        cmp_i: usize,
    ) void {
        const body = &self.bodies[i];
        const body_cmp = &self.bodies[cmp_i];
        const x_dist = body.x - body_cmp.x;
        const y_dist = body.y - body_cmp.y;
        const dist = std.math.sqrt(
            std.math.pow(f32, x_dist, 2) + std.math.pow(f32, y_dist, 2),
        );

        const colliding = c.CheckCollisionCircles(
            .{
                .x = body.x * State.screen_scale,
                .y = body.y * State.screen_scale,
            },
            body.radius / 2,
            .{
                .x = body_cmp.x * State.screen_scale,
                .y = body_cmp.y * State.screen_scale,
            },
            body_cmp.radius / 2,
        );
        if (colliding) return;

        const force = -1 * delta *
            body.mass * body_cmp.mass / std.math.pow(f32, dist, 2);
        const force_x = force * (x_dist / dist);
        const force_y = force * (y_dist / dist);

        const accel_x = force_x / body.mass;
        const accel_y = force_y / body.mass;

        body.vel_x += accel_x;
        body.vel_y += accel_y;
    }
};

pub const Creator = struct {
    active: bool = false,
    body: Body = .{},
    x_displacement: f32 = 0,
    y_displacement: f32 = 0,
};
