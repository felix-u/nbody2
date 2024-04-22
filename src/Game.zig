const rl = @cImport({
    @cInclude("raylib.h");
    @cInclude("raymath.h");
});
const std = @import("std");

const pow = std.math.pow;
const V2 = @Vector(2, f32);

allocator: std.mem.Allocator,
name: []const u8,
width: c_int,
height: c_int,
fps: c_int = default_fps,
g: f32 = 3e-8 / @as(f32, default_fps),
delta: f32 = 0,
cursor_radius: f32 = 0,
bodies: std.ArrayList(Body) = undefined,
creator: Creator = undefined,
mouse_pos: V2 = .{ 0, 0 },

const default_fps = 60;
const collision_dampen_factor = 0.3;

const Body = struct {
    mass: f32,
    radius: f32,
    pos: V2 = .{ 0, 0 },
    velocity: V2 = .{ 0, 0 },
};

const Creator = struct {
    active: bool = false,
    displacement: V2 = .{ 0, 0 },
    body: Body,

    pub const colour_inactive = Colour.blue;
    pub const colour_active = Colour.grey_dark;
    pub const colour_line = colour_active;
    pub const ring_thickness = 0.004;
    pub const line_width = 0.004;
};

const Colour = struct {
    pub inline fn fromHex(rgb: u24) rl.Color {
        return .{
            .r = (rgb & 0xff0000) >> 16,
            .g = (rgb & 0x00ff00) >> 8,
            .b = (rgb & 0x0000ff),
            .a = 0xff,
        };
    }

    pub const blue = fromHex(0x00008b);
    pub const black = fromHex(0x000000);
    pub const grey_light = fromHex(0xc0c0c0);
    pub const grey_dark = fromHex(0x555555);

    pub const background = grey_light;
    pub const body = black;
};

pub fn init(game: @This()) @This() {
    rl.SetConfigFlags(rl.FLAG_MSAA_4X_HINT | rl.FLAG_WINDOW_RESIZABLE);
    rl.SetTargetFPS(game.fps);
    rl.InitWindow(game.width, game.height, @ptrCast(game.name));

    var result = game;
    result.bodies = std.ArrayList(Body).init(result.allocator);
    return result;
}

pub fn deinit(_: *@This()) void {
    rl.CloseWindow();
}

pub inline fn shouldQuit(_: *@This()) bool {
    return rl.WindowShouldClose();
}

pub fn frameBegin(_: *@This()) void {
    rl.BeginDrawing();
    rl.ClearBackground(Colour.background);
}

pub fn frameEnd(_: *@This()) void {
    rl.EndDrawing();
}

pub fn updateAndRender(self: *@This()) !void {
    self.delta = rl.GetFrameTime();
    self.width = rl.GetScreenWidth();
    self.height = rl.GetScreenHeight();

    self.mouse_pos = self.normalFromScreen(
        v2fromRaylib(rl.GetMousePosition()),
    );
    self.cursor_radius += rl.GetMouseWheelMove() / 100;
    self.cursor_radius = std.math.clamp(self.cursor_radius, 0.01, 0.1);

    const creator = &self.creator;
    creator.body.mass = std.math.pow(f32, self.cursor_radius * 1000, 3);
    creator.body.radius = self.cursor_radius;

    if (rl.IsKeyPressed('R')) self.bodies.shrinkRetainingCapacity(0);

    if (creator.active) {
        if (rl.IsMouseButtonDown(rl.MOUSE_BUTTON_RIGHT)) {
            creator.displacement = self.mouse_pos - creator.body.pos;
        } else if (rl.IsMouseButtonReleased(rl.MOUSE_BUTTON_RIGHT)) {
            creator.active = false;
        }

        if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT)) {
            const factor: V2 = @splat(100);
            creator.body.velocity = creator.displacement / factor;
            try self.bodies.append(creator.body);
        }
    } else {
        if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_RIGHT)) {
            creator.active = true;
            creator.displacement = .{ 0, 0 };
            creator.body.pos = self.mouse_pos;
        } else if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT)) {
            creator.body.pos = self.mouse_pos;
            creator.body.velocity = .{ 0, 0 };
            try self.bodies.append(creator.body);
        }
    }

    for (0..self.bodies.items.len) |i| {
        for (i + 1..self.bodies.items.len) |cmp_i| {
            self.computeInteraction(i, cmp_i);
        }
        self.computeScreenCollision(i);

        const body = &self.bodies.items[i];

        body.pos += body.velocity;
        const pos = self.screenFromNormal(body.pos);
        rl.DrawCircleV(
            raylibFromV2(pos),
            self.screenFromNormal(body.radius),
            Colour.black,
        );
    }

    self.renderCreator();
}

fn computeInteraction(self: *@This(), i: usize, cmp_i: usize) void {
    const body = &self.bodies.items[i];
    const body_cmp = &self.bodies.items[cmp_i];

    const dist_xy = body.pos - body_cmp.pos;
    const dist = @sqrt(pow(f32, dist_xy[0], 2) + pow(f32, dist_xy[1], 2));

    const colliding = dist < (body.radius + body_cmp.radius) / 2;
    if (colliding) return;

    const force = -1 * self.delta * self.g *
        body.mass * body_cmp.mass / pow(f32, dist, 2);
    const force_xy = V2{
        force * (dist_xy[0] / dist),
        force * (dist_xy[1] / dist),
    };

    const body_accel = force_xy / @as(V2, @splat(body.mass));
    body.velocity += body_accel;

    const body_cmp_accel = force_xy / @as(V2, @splat(body_cmp.mass));
    body_cmp.velocity -= body_cmp_accel;
}

fn computeScreenCollision(self: *@This(), i: usize) void {
    const body = &self.bodies.items[i];
    const dimensions = [2]f32{ self.normalWidth(), 1 };
    inline for (0..2) |axis| {
        if (body.pos[axis] - body.radius < 0) {
            body.pos[axis] = body.radius;
            body.velocity[axis] *= -collision_dampen_factor;
        } else if (body.pos[axis] + body.radius > dimensions[axis]) {
            body.pos[axis] = dimensions[axis] - body.radius;
            body.velocity[axis] *= -collision_dampen_factor;
        }
    }
}

fn renderCreator(self: @This()) void {
    const creator = self.creator;
    const body = creator.body;

    const inner_radius = self.screenFromNormal(self.cursor_radius);
    const outer_radius = inner_radius +
        self.screenFromNormal(@as(f32, Creator.ring_thickness));
    if (!creator.active) {
        const pos = self.screenFromNormal(self.mouse_pos);
        rl.DrawRing(
            raylibFromV2(pos),
            inner_radius,
            outer_radius,
            0,
            360,
            0,
            Creator.colour_inactive,
        );
        return;
    }

    const line_start = self.screenFromNormal(body.pos);
    const line_end = self.screenFromNormal(body.pos + creator.displacement);
    const line_width = self.screenFromNormal(@as(f32, Creator.line_width));
    rl.DrawLineEx(
        raylibFromV2(line_start),
        raylibFromV2(line_end),
        line_width,
        Creator.colour_line,
    );

    const radius = self.screenFromNormal(body.radius);
    rl.DrawCircleV(
        raylibFromV2(line_start),
        radius,
        Creator.colour_active,
    );
}

inline fn scale(self: @This(), comptime T: type) T {
    const scalar: f32 = @floatFromInt(self.height);
    return switch (T) {
        inline f32 => scalar,
        inline V2 => @as(V2, @splat(scalar)),
        inline else => @compileError("invalid type passed to Game.scale()"),
    };
}

inline fn normalFromScreen(self: @This(), screen: anytype) @TypeOf(screen) {
    return screen / self.scale(@TypeOf(screen));
}

inline fn screenFromNormal(self: @This(), normal: anytype) @TypeOf(normal) {
    return normal * self.scale(@TypeOf(normal));
}

inline fn normalWidth(self: @This()) f32 {
    return @as(f32, @floatFromInt(self.width)) / self.scale(f32);
}

inline fn v2fromRaylib(vector2: rl.Vector2) V2 {
    return .{ vector2.x, vector2.y };
}

inline fn raylibFromV2(v2: V2) rl.Vector2 {
    return .{ .x = v2[0], .y = v2[1] };
}
