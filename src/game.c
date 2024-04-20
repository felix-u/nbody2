typedef struct Body {
    f32 mass, radius;
    Vector2 pos, velocity;
    Color colour;
} Body;
typedef Array(Body) Array_Body;

typedef struct Creator {
    bool active;
    Body body;
    Vector2 displacement;
} Creator;

typedef struct Context {
    Arena arena;
    char *name;
    int width, height, fps;

    f32 G, delta, cursor_radius;
    Array_Body bodies;
    Creator creator;
    Vector2 mouse_pos;
} Context;

#define colour_hex(rgb) (Color){\
    .r = (rgb & 0xff0000) >> 16,\
    .g = (rgb & 0x00ff00) >> 8,\
    .b = (rgb & 0x0000ff),\
    .a = 0xff,\
}
//
#define colour_blue       colour_hex(0x00008b)
#define colour_black      colour_hex(0x000000)
#define colour_light_grey colour_hex(0xc0c0c0)
#define colour_dark_grey  colour_hex(0x555555)
//
#define colour_background colour_light_grey
#define colour_body       colour_black

#define colour_creator_inactive          colour_blue
#define colour_creator_active            colour_dark_grey
#define colour_creator_displacement_line colour_creator_active
//
#define creator_ring_thickness              5.0f
#define creator_displacement_line_thickness 4.0f

static inline f32 normal_from_screen(Context *ctx, f32 s) { return s / (f32)ctx->height; }
static inline f32 screen_from_normal(Context *ctx, f32 n) { return n * (f32)ctx->height; }

#define game_cursor_radius_min 10.0f
#define game_cursor_radius_max 100.0f
#define game_collision_dampen 0.3f

static void game_init(Context *ctx) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(ctx->fps);
    InitWindow(ctx->width, ctx->height, ctx->name);
    ctx->G = 3E-8f / (f32)ctx->fps;
    ctx->creator.body.colour = colour_body;
}

static void game_deinit(Context *ctx) { discard(ctx); CloseWindow(); }

static void game_frame_begin(Context *ctx) {
    discard(ctx);
    BeginDrawing();
    ClearBackground(colour_background);
}

static void game_frame_end(Context *ctx) { discard(ctx); EndDrawing(); }

static void game_compute_interaction(Context *ctx, usize i, usize cmp_i) {
    Body *body = &ctx->bodies.ptr[i];
    Body *body_cmp = &ctx->bodies.ptr[cmp_i];

    f32 x_dist = body->pos.x - body_cmp->pos.x;
    f32 y_dist = body->pos.y - body_cmp->pos.y;
    f32 dist = sqrtf(x_dist * x_dist + y_dist * y_dist);

    bool colliding = dist < (body->radius + body_cmp->radius) / (f32)ctx->height / 2.0f;
    if (colliding) return;

    f32 force = -1 * ctx->delta * body->mass * body_cmp->mass / (dist * dist);
    f32 force_x = force * (x_dist / dist);
    f32 force_y = force * (y_dist / dist);

    Vector2 body_accel = { force_x / body->mass, force_y / body->mass };
    body->velocity.x += body_accel.x;
    body->velocity.y += body_accel.y;

    Vector2 body_cmp_accel = { force_x / body_cmp->mass, force_y / body_cmp->mass };
    body_cmp->velocity.x -= body_cmp_accel.x;
    body_cmp->velocity.y -= body_cmp_accel.y;
}

static Vector2 Vector2_scale(Vector2 v, f32 s)       { return (Vector2){ v.x * s, v.y * s }; }
static Vector2 Vector2_add(Vector2 v1, Vector2 v2)   { return (Vector2){ v1.x + v2.x, v1.y + v2.y }; }
static Vector2 Vector2_sub(Vector2 v1, Vector2 v2)   { return (Vector2){ v1.x - v2.x, v1.y - v2.y }; }
static Vector2 Vector2_divide(Vector2 v, f32 divide) { return (Vector2){ v.x / divide, v.y / divide }; }

static inline Vector2 Vector2_normal_from_screen(Context *ctx, Vector2 v) {
    f32 scale = (f32)ctx->height;
    return Vector2_divide(v, scale);
}

static inline Vector2 Vector2_screen_from_normal(Context *ctx, Vector2 v) {
    f32 scale = (f32)ctx->height;
    return Vector2_scale(v, scale);
}

static void game_compute_screen_collision(Context *ctx, usize i) {
    Body *body = &ctx->bodies.ptr[i];
    
    Vector2 pos = Vector2_screen_from_normal(ctx, body->pos);

    bool colliding = CheckCollisionCircleRec(pos, body->radius, (Rectangle){ 0, 0, (f32)ctx->width, (f32)ctx->height });
    if (!colliding) return;

    if (pos.x - body->radius < 0) {
        body->pos.x = body->radius / (f32)ctx->height;
        body->velocity.x *= -game_collision_dampen;
    } else if (pos.x + body->radius > (f32)ctx->width) {
        body->pos.x = ((f32)ctx->width - body->radius) / (f32)ctx->height;
        body->velocity.x *= -game_collision_dampen;
    }

    if (pos.y - body->radius < 0) {
        body->pos.y = body->radius / (f32)ctx->height;
        body->velocity.y *= -game_collision_dampen;
    } else if (pos.y + body->radius > (f32)ctx->height) {
        body->pos.y = ((f32)ctx->height - body->radius) / (f32)ctx->height;
        body->velocity.y *= -game_collision_dampen;
    }
}

static void game_render_creator(Context *ctx) {
    Creator creator = ctx->creator;

    f32 outer_radius = ctx->cursor_radius + creator_ring_thickness;
    if (!creator.active) {
        DrawRing(ctx->mouse_pos, ctx->cursor_radius, outer_radius, 0, 360, 0, colour_creator_inactive);
        return;
    }

    Vector2 start = Vector2_screen_from_normal(ctx, creator.body.pos);
    Vector2 end = Vector2_screen_from_normal(ctx, Vector2_add(creator.body.pos, creator.displacement));
    DrawLineEx(start, end, creator_displacement_line_thickness, colour_creator_displacement_line);

    Vector2 pos = Vector2_screen_from_normal(ctx, creator.body.pos);
    DrawRing(pos, creator.body.radius, outer_radius, 0, 360, 0, colour_creator_active);
}

static void game_update_and_render(Context *ctx) {
    ctx->delta = GetFrameTime();

    ctx->mouse_pos = GetMousePosition();
    ctx->cursor_radius += GetMouseWheelMove() * 10.0f;
    clamp(ctx->cursor_radius, game_cursor_radius_min, game_cursor_radius_max);
    
    Array_Body *bodies = &ctx->bodies;
    
    if (IsKeyPressed('R')) bodies->len = 0;

    Creator *creator = &ctx->creator;
    Vector2 mouse = ctx->mouse_pos;
    if (creator->active) { 
        f32 factor = 300.0f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            creator->displacement = Vector2_sub(Vector2_normal_from_screen(ctx, mouse), creator->body.pos);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            creator->active = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            creator->body.velocity = Vector2_divide(creator->displacement, factor);
            array_push(&ctx->arena, bodies, &creator->body);
        }
    } else {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            creator->active = true;
            creator->body.pos = Vector2_normal_from_screen(ctx, mouse);
            creator->body.mass = powf(ctx->cursor_radius, 3) * ctx->G;
            creator->body.radius = ctx->cursor_radius;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 pos = Vector2_normal_from_screen(ctx, mouse);
            f32 mass = powf(ctx->cursor_radius, 3) * ctx->G;
            Body body = { .mass = mass, .radius = ctx->cursor_radius, .pos = pos, .colour = colour_body };
            array_push(&ctx->arena, bodies, &body);
        }
    }

    for (usize i = 0; i < bodies->len; i += 1) {
        for (usize cmp_i = i + 1; cmp_i < bodies->len; cmp_i += 1) game_compute_interaction(ctx, i, cmp_i);

        game_compute_screen_collision(ctx, i);

        Body *body = &bodies->ptr[i];
        body->pos = Vector2_add(body->pos, body->velocity);

        Vector2 pos = Vector2_screen_from_normal(ctx, body->pos);
        DrawCircle((int)pos.x, (int)pos.y, body->radius, body->colour);
    }

    game_render_creator(ctx);
}
