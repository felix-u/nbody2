const rl = @cImport({
    @cInclude("raylib.h");
    @cInclude("raymath.h");
});
const std = @import("std");

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
mouse_pos: rl.Vector2 = .{},

const default_fps = 60;
const collision_dampen_factor = 0.3;

const Body = struct {
    mass: f32,
    radius: f32,
    pos: rl.Vector2 = .{},
    velocity: rl.Vector2 = .{},
};

const Creator = struct {
    active: bool = false,
    displacement: rl.Vector2 = .{},
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

    self.mouse_pos = self.normalFromScreenVector2(rl.GetMousePosition());
    self.cursor_radius += rl.GetMouseWheelMove() / 100;
    self.cursor_radius = std.math.clamp(self.cursor_radius, 0.01, 0.1);

    const creator = &self.creator;
    creator.body.mass = std.math.pow(f32, self.cursor_radius * 1000, 3);
    creator.body.radius = self.cursor_radius;

    if (rl.IsKeyPressed('R')) self.bodies.shrinkRetainingCapacity(0);

    if (creator.active) {
        const factor = 100;

        if (rl.IsMouseButtonDown(rl.MOUSE_BUTTON_RIGHT)) {
            creator.displacement = rl.Vector2Subtract(
                self.mouse_pos,
                creator.body.pos,
            );
        } else if (rl.IsMouseButtonReleased(rl.MOUSE_BUTTON_RIGHT)) {
            creator.active = false;
        }

        if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT)) {
            creator.body.velocity = rl.Vector2Scale(
                creator.displacement,
                1 / factor,
            );
            try self.bodies.append(creator.body);
        }
    } else {
        if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_RIGHT)) {
            creator.active = true;
            creator.displacement = .{};
            creator.body.pos = self.mouse_pos;
        } else if (rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT)) {
            creator.body.pos = self.mouse_pos;
            creator.body.velocity = .{};
            try self.bodies.append(creator.body);
            std.debug.print("{any}\n", .{self.bodies.items[self.bodies.items.len - 1]});
            std.debug.print("{any}\n", .{self.screenFromNormalVector2(creator.body.pos)});
        }
    }

    for (0..self.bodies.items.len) |i| {
        for (i + 1..self.bodies.items.len) |cmp_i| {
            self.computeInteraction(i, cmp_i);
        }
        self.computeScreenCollision(i);

        const body = &self.bodies.items[i];

        body.pos = rl.Vector2Add(body.pos, body.velocity);

        const pos = self.screenFromNormalVector2(body.pos);
        rl.DrawCircleV(pos, self.screenFromNormal(body.radius), Colour.black);
    }

    self.renderCreator();
}

fn computeInteraction(self: *@This(), i: usize, cmp_i: usize) void {
    const body = &self.bodies.items[i];
    const body_cmp = &self.bodies.items[cmp_i];

    const dist_x = body.pos.x - body_cmp.pos.x;
    const dist_y = body.pos.y - body_cmp.pos.y;
    const dist = @sqrt(
        std.math.pow(f32, dist_x, 2) + std.math.pow(f32, dist_y, 2),
    );

    const colliding = dist < (body.radius + body_cmp.radius) / 2;
    if (colliding) return;

    const force = -1 * self.delta * self.g *
        body.mass * body_cmp.mass / std.math.pow(f32, dist, 2);
    const force_x = force * (dist_x / dist);
    const force_y = force * (dist_y / dist);

    const body_accel = rl.Vector2{
        .x = force_x / body.mass,
        .y = force_y / body.mass,
    };
    body.velocity = rl.Vector2Add(body.velocity, body_accel);

    const body_cmp_accel = rl.Vector2{
        .x = force_x / body_cmp.mass,
        .y = force_y / body_cmp.mass,
    };
    body.velocity = rl.Vector2Subtract(body.velocity, body_cmp_accel);
}

fn computeScreenCollision(self: *@This(), i: usize) void {
    const body = &self.bodies.items[i];
    const normal_width = self.normalWidth();

    if (body.pos.x - body.radius < 0) {
        body.pos.x = body.radius;
        body.velocity.x *= -collision_dampen_factor;
    } else if (body.pos.x + body.radius < normal_width) {
        body.pos.x = normal_width - body.radius;
        body.velocity.x *= -collision_dampen_factor;
    }

    if (body.pos.y - body.radius < 0) {
        body.pos.y = body.radius;
        body.velocity.y *= -collision_dampen_factor;
    } else if (body.pos.y + body.radius < 1) {
        body.pos.y = 1 - body.radius;
        body.velocity.y *= -collision_dampen_factor;
    }
}

fn renderCreator(self: @This()) void {
    const creator = self.creator;
    const body = creator.body;

    const inner_radius = self.screenFromNormal(self.cursor_radius);
    const outer_radius = inner_radius +
        self.screenFromNormal(Creator.ring_thickness);
    if (!creator.active) {
        const pos = self.screenFromNormalVector2(self.mouse_pos);
        rl.DrawRing(
            pos,
            inner_radius,
            outer_radius,
            0,
            360,
            0,
            Creator.colour_inactive,
        );
        return;
    }

    const line_start = self.screenFromNormalVector2(body.pos);
    const line_end = self.screenFromNormalVector2(
        rl.Vector2Add(body.pos, creator.displacement),
    );
    const line_width = self.screenFromNormal(Creator.line_width);
    rl.DrawLineEx(line_start, line_end, line_width, Creator.colour_line);

    const radius = self.screenFromNormal(body.radius);
    rl.DrawCircleV(line_start, radius, Creator.colour_active);
}

inline fn scale(self: @This()) f32 {
    return @floatFromInt(self.height);
}

inline fn normalFromScreen(self: @This(), screen: f32) f32 {
    return screen / self.scale();
}

inline fn screenFromNormal(self: @This(), normal: f32) f32 {
    return normal * self.scale();
}

inline fn normalWidth(self: @This()) f32 {
    return @as(f32, @floatFromInt(self.width)) / self.scale();
}

inline fn normalFromScreenVector2(
    self: @This(),
    screen: rl.Vector2,
) rl.Vector2 {
    return rl.Vector2Scale(screen, 1 / self.scale());
}

inline fn screenFromNormalVector2(
    self: @This(),
    normal: rl.Vector2,
) rl.Vector2 {
    return rl.Vector2Scale(normal, self.scale());
}
