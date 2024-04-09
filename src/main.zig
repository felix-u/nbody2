const c = @cImport({
    @cInclude("raylib.h");
});

const nbody = @import("nbody.zig");
const render = @import("render.zig");
const std = @import("std");

pub fn main() void {
    var state = State{};

    render.init(&state);
    defer render.deinit();

    while (!render.shouldQuit()) {
        render.beginFrame();
        defer render.endFrame();

        state.step();

        render.drawAll(&state);
    }
}

pub const State = struct {
    bodies: nbody.Bodies = .{},
    creator: nbody.Creator = .{},
    cursor_radius: f32 = cursor_radius_min * 3,
    delta: f32 = 0,
    mouse_pos: c.Vector2 = undefined,
    camera: render.Camera = .{},

    pub const cursor_radius_min = 10;
    pub const cursor_radius_max = 100;
    pub const screen_collision_dampen = 0.3;
    pub const screen_scale = 900;
    pub const screen_height = screen_scale;
    pub const screen_width = 1800;
    pub const target_fps: comptime_float = 360;
    pub const G = 3 * 10e-9 / target_fps;

    pub fn step(self: *State) void {
        self.stepCursor();
        self.stepCamera();
        self.stepCreator();
        self.stepBodies();
    }

    fn stepCamera(self: *State) void {
        const camera = &self.camera;

        if (!camera.active) {
            if (!c.IsKeyDown('C')) return;
            camera.active = true;
        }

        if (!c.IsKeyDown('C')) {
            camera.active = false;
            return;
        }

        var delta = c.GetMouseDelta();
        delta.x = render.normalFromScreen(self.*, delta.x);
        delta.y = render.normalFromScreen(self.*, delta.y);

        camera.x += delta.x;
        camera.y += delta.y;

        // const wheel = c.GetMouseWheelMove();
        // if (wheel == 0) return;
    }

    fn stepCursor(self: *State) void {
        self.mouse_pos = c.GetMousePosition();
        const mouse_wheel = c.GetMouseWheelMove();
        self.cursor_radius += mouse_wheel * 10;
        if (self.cursor_radius > cursor_radius_max) {
            self.cursor_radius = cursor_radius_max;
        } else if (self.cursor_radius < cursor_radius_min) {
            self.cursor_radius = cursor_radius_min;
        }
    }

    fn stepCreator(self: *State) void {
        const creator = &self.creator;
        const body = &creator.body;
        const bodies = &self.bodies;
        const mouse = &self.mouse_pos;

        if (creator.active) {
            if (c.IsMouseButtonDown(c.MOUSE_BUTTON_RIGHT)) {
                creator.x_displacement =
                    render.normalFromScreen(self.*, mouse.x) - body.x;
                creator.y_displacement =
                    render.normalFromScreen(self.*, mouse.y) - body.y;
            } else if (c.IsMouseButtonReleased(c.MOUSE_BUTTON_RIGHT)) {
                creator.active = false;
                body.vel_x = creator.x_displacement;
                body.vel_y = creator.y_displacement;
                bodies.add(body.*);
            }

            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT)) {
                body.vel_x = creator.x_displacement;
                body.vel_y = creator.y_displacement;
                bodies.add(body.*);
            }
        } else {
            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_RIGHT)) {
                creator.active = true;
                body.x = render.normalFromScreen(self.*, mouse.x);
                body.y = render.normalFromScreen(self.*, mouse.y);
                body.mass = std.math.pow(f32, self.cursor_radius, 3) * G;
                body.radius = self.cursor_radius;
            }

            if (c.IsMouseButtonPressed(c.MOUSE_BUTTON_LEFT) and
                bodies.len < nbody.Bodies.cap)
            {
                bodies.add(.{
                    .mass = std.math.pow(f32, self.cursor_radius, 3) * G,
                    .radius = self.cursor_radius,
                    .x = render.normalFromScreen(self.*, mouse.x),
                    .y = render.normalFromScreen(self.*, mouse.y),
                });
            }
        }
    }

    fn stepBodies(self: *State) void {
        for (0..self.bodies.len) |body_i| {
            const body = &self.bodies.bodies[body_i];
            for (0..self.bodies.len) |body_cmp_i| {
                if (body_cmp_i == body_i) continue;
                self.bodies.computeInteraction(body_i, body_cmp_i);
            }

            body.computeScreenCollision();

            body.x += body.vel_x * self.delta;
            body.y += body.vel_y * self.delta;
        }
    }
};
