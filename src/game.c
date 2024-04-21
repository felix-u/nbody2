typedef Vector2 V2;

typedef struct Body {
    f32 mass, radius;
    V2 pos, velocity;
    Color colour;
} Body;
typedef Array(Body) Array_Body;

typedef struct Creator {
    bool active;
    Body body;
    V2 displacement;
} Creator;

typedef struct Context {
    Arena arena;
    char *name;
    int width, height, fps;

    f32 G, delta, cursor_radius;
    Array_Body bodies;
    Creator creator;
    V2 mouse_pos;
} Context;

static inline f32 scale(Context *ctx) { return (f32)ctx->height; }
static f32 normal_from_screen(Context *ctx, f32 n) { return n / scale(ctx); }
static f32 screen_from_normal(Context *ctx, f32 n) { return n * scale(ctx); }
static f32 normal_width(Context *ctx) { return (f32)ctx->width / scale(ctx); }

static void game_init(Context *ctx) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
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

    bool colliding = dist < (body->radius + body_cmp->radius) / 2.0f;
    if (colliding) return;

    f32 factor = -1 * ctx->delta * ctx->G;
    f32 force = factor * body->mass * body_cmp->mass / (dist * dist);
    f32 force_x = force * (x_dist / dist);
    f32 force_y = force * (y_dist / dist);

    V2 body_accel = { force_x / body->mass, force_y / body->mass };
    body->velocity.x += body_accel.x;
    body->velocity.y += body_accel.y;

    V2 body_cmp_accel = { force_x / body_cmp->mass, force_y / body_cmp->mass };
    body_cmp->velocity.x -= body_cmp_accel.x;
    body_cmp->velocity.y -= body_cmp_accel.y;
}

static V2 V2_scale(V2 v, f32 n)  { return (V2){ v.x * n, v.y * n }; }
static V2 V2_divide(V2 v, f32 n) { return (V2){ v.x / n, v.y / n }; }
static V2 V2_add(V2 v1, V2 v2)   { return (V2){ v1.x + v2.x, v1.y + v2.y }; }
static V2 V2_sub(V2 v1, V2 v2)   { return (V2){ v1.x - v2.x, v1.y - v2.y }; }

static inline V2 V2_normal_from_screen(Context *ctx, V2 v) { 
    return V2_divide(v, scale(ctx)); 
}
static inline V2 V2_screen_from_normal(Context *ctx, V2 v) { 
    return V2_scale(v, scale(ctx)); 
}

static void game_compute_screen_collision(Context *ctx, usize i) {
    Body *body = &ctx->bodies.ptr[i];
    f32 norm_width = normal_width(ctx);

    if (body->pos.x - body->radius < 0) {
        body->pos.x = body->radius;
        body->velocity.x *= -game_collision_dampen;
    } else if (body->pos.x + body->radius > norm_width) {
        body->pos.x = norm_width - body->radius;
        body->velocity.x *= -game_collision_dampen;
    }

    if (body->pos.y - body->radius < 0) {
        body->pos.y = body->radius;
        body->velocity.y *= -game_collision_dampen;
    } else if (body->pos.y + body->radius > 1.0f) {
        body->pos.y = 1.0f - body->radius;
        body->velocity.y *= -game_collision_dampen;
    }
}

static void game_render_creator(Context *ctx) {
    Creator creator = ctx->creator;
    Body body = creator.body;

    f32 inner = screen_from_normal(ctx, ctx->cursor_radius);
    f32 outer = inner + screen_from_normal(ctx, creator_ring_thickness);
    if (!creator.active) {
        V2 pos = V2_screen_from_normal(ctx, ctx->mouse_pos);
        DrawRing(pos, inner, outer, 0, 360, 0, colour_creator_inactive);
        return;
    }

    V2 start = V2_screen_from_normal(ctx, body.pos);
    V2 end = V2_screen_from_normal(
        ctx, V2_add(body.pos, creator.displacement)
    );
    f32 line_width = screen_from_normal(ctx, creator_line_width);
    DrawLineEx(start, end, line_width, colour_creator_displacement_line);

    f32 radius = screen_from_normal(ctx, body.radius);
    DrawCircleV(start, radius, colour_creator_active);
}

static void game_update_and_render(Context *ctx) {
    ctx->delta = GetFrameTime();
    ctx->width = GetScreenWidth();
    ctx->height = GetScreenHeight();

    ctx->mouse_pos = V2_normal_from_screen(ctx, GetMousePosition());
    ctx->cursor_radius += GetMouseWheelMove() / 100.0f;
    clamp(ctx->cursor_radius, game_cursor_radius_min, game_cursor_radius_max);

    Creator *creator = &ctx->creator;
    creator->body.mass = powf(ctx->cursor_radius * 1000.0f, 3);
    creator->body.radius = ctx->cursor_radius;

    Array_Body *bodies = &ctx->bodies;
    if (IsKeyPressed('R')) bodies->len = 0;

    if (creator->active) {
        f32 factor = 100.0f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            creator->displacement = V2_sub(ctx->mouse_pos, creator->body.pos);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            creator->active = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            creator->body.velocity = V2_divide(creator->displacement, factor);
            array_push(&ctx->arena, bodies, &creator->body);
        }
    } else {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            creator->active = true;
            creator->displacement = (V2){0};
            creator->body.pos = ctx->mouse_pos;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            creator->body.pos = ctx->mouse_pos;
            creator->body.velocity = (V2){0};
            array_push(&ctx->arena, bodies, &creator->body);
        }
    }

    for (usize i = 0; i < bodies->len; i += 1) {
        for (usize cmp_i = i + 1; cmp_i < bodies->len; cmp_i += 1) {
            game_compute_interaction(ctx, i, cmp_i);
        }
        game_compute_screen_collision(ctx, i);

        Body *body = &bodies->ptr[i];
        body->pos = V2_add(body->pos, body->velocity);

        V2 pos = V2_screen_from_normal(ctx, body->pos);
        DrawCircleV(pos, screen_from_normal(ctx, body->radius), body->colour);
    }

    game_render_creator(ctx);
}
