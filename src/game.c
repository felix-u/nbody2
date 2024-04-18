typedef struct Body {
    f32 mass, radius, x, y, velocity_x, velocity_y;
    Color colour;
} Body;
typedef Array(Body) Array_Body;

typedef struct Creator {
    bool active;
    Body body;
    f32 x_displacement, y_displacement;
} Creator;

typedef struct Context {
    Arena arena;
    char *name;
    int width, height, fps;

    f32 G;
    Array_Body bodies;
    Creator creator;
    f32 cursor_radius, delta;
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

static inline f32 normal_from_screen(Context *ctx, f32 s) {
    return s / (f32)ctx->height;
}

static inline f32 screen_from_normal(Context *ctx, f32 n) {
    return n * (f32)ctx->height;
}

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

    f32 x_dist = body->x - body_cmp->x;
    f32 y_dist = body->y - body_cmp->y;
    f32 dist = sqrtf(x_dist * x_dist + y_dist * y_dist);

    bool colliding = 
        dist < (body->radius + body_cmp->radius) / (f32)ctx->height / 2.0f;
    if (colliding) return;

    f32 force = -1 * ctx->delta * body->mass * body_cmp->mass / (dist * dist);
    f32 force_x = force * (x_dist / dist);
    f32 force_y = force * (y_dist / dist);

    f32 body_accel_x = force_x / body->mass;
    f32 body_accel_y = force_y / body->mass;
    body->velocity_x += body_accel_x;
    body->velocity_y += body_accel_y;

    f32 body_cmp_accel_x = force_x / body_cmp->mass;
    f32 body_cmp_accel_y = force_y / body_cmp->mass;
    body_cmp->velocity_x -= body_cmp_accel_x;
    body_cmp->velocity_y -= body_cmp_accel_y;
}

static void game_compute_screen_collision(Context *ctx, usize i) {
    Body *body = &ctx->bodies.ptr[i];

    bool colliding = CheckCollisionCircleRec(
        (Vector2){ body->x * (f32)ctx->height, body->y * (f32)ctx->height },
        body->radius,
        (Rectangle){ 0, 0, (f32)ctx->width, (f32)ctx->height }
    );
    if (!colliding) return;

    f32 x = body->x * (f32)ctx->height;
    f32 y = body->y * (f32)ctx->height;

    if (x - body->radius < 0) {
        body->x = body->radius / (f32)ctx->height;
        body->velocity_x *= -game_collision_dampen;
    } else if (x + body->radius > (f32)ctx->width) {
        body->x = ((f32)ctx->width - body->radius) / (f32)ctx->height;
        body->velocity_x *= -game_collision_dampen;
    }

    if (y - body->radius < 0) {
        body->y = body->radius / (f32)ctx->height;
        body->velocity_y *= -game_collision_dampen;
    } else if (y + body->radius > (f32)ctx->height) {
        body->y = ((f32)ctx->height - body->radius) / (f32)ctx->height;
        body->velocity_y *= -game_collision_dampen;
    }
}

static void game_render_creator(Context *ctx) {
    Creator creator = ctx->creator;

    if (!creator.active) {
        DrawRing(
            ctx->mouse_pos, ctx->cursor_radius, 
            ctx->cursor_radius + creator_ring_thickness,
            0, 360, 0, colour_creator_inactive
        );
        return;
    }

    f32 start_x = screen_from_normal(ctx, creator.body.x);
    f32 start_y = screen_from_normal(ctx, creator.body.y);
    f32 end_x = screen_from_normal(ctx, 
        creator.body.x + creator.x_displacement);
    f32 end_y = screen_from_normal(ctx, 
        creator.body.y + creator.y_displacement);
    DrawLineEx(
        (Vector2){ start_x, start_y }, (Vector2){ end_x, end_y },
        creator_displacement_line_thickness, colour_creator_displacement_line
    );

    DrawRing(
        (Vector2){ 
            screen_from_normal(ctx, creator.body.x),
            screen_from_normal(ctx, creator.body.y),
        },
        creator.body.radius, creator.body.radius + creator_ring_thickness,
        0, 360, 0, colour_creator_active
    ); 
}

static void game_update_and_render(Context *ctx) {
    ctx->delta = GetFrameTime();

    ctx->mouse_pos = GetMousePosition();
    ctx->cursor_radius += GetMouseWheelMove() * 10.0f;
    clamp(ctx->cursor_radius, game_cursor_radius_min, game_cursor_radius_max);
    
    Array_Body *bodies = &ctx->bodies;

    Creator *creator = &ctx->creator;
    Vector2 mouse = ctx->mouse_pos;
    if (creator->active) { 
        f32 factor = 300.0f;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            creator->x_displacement = 
                normal_from_screen(ctx, mouse.x) - creator->body.x;
            creator->y_displacement = 
                normal_from_screen(ctx, mouse.y) - creator->body.y;
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            creator->active = false;
            creator->body.velocity_x = creator->x_displacement / factor;
            creator->body.velocity_y = creator->y_displacement / factor;
            array_push(&ctx->arena, bodies, &creator->body);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            creator->body.velocity_x = creator->x_displacement / factor;
            creator->body.velocity_y = creator->y_displacement / factor;
            array_push(&ctx->arena, bodies, &creator->body);
        }
    } else {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            creator->active = true;
            creator->body.x = normal_from_screen(ctx, mouse.x);
            creator->body.y = normal_from_screen(ctx, mouse.y);
            creator->body.mass = powf(ctx->cursor_radius, 3) * ctx->G;
            creator->body.radius = ctx->cursor_radius;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            array_push(&ctx->arena, bodies, &((Body){
                .mass = powf(ctx->cursor_radius, 3) * ctx->G,
                .radius = ctx->cursor_radius,
                .x = normal_from_screen(ctx, mouse.x),
                .y = normal_from_screen(ctx, mouse.y),
                .colour = colour_body,
            }));
        }
    }

    for (usize i = 0; i < bodies->len; i += 1) {
        for (usize cmp_i = i + 1; cmp_i < bodies->len; cmp_i += 1) {
            game_compute_interaction(ctx, i, cmp_i);
        }

        game_compute_screen_collision(ctx, i);

        Body *body = &bodies->ptr[i];
        body->x += body->velocity_x;
        body->y += body->velocity_y;

        int x_coord = (int)screen_from_normal(ctx, body->x);
        int y_coord = (int)screen_from_normal(ctx, body->y);
        DrawCircle(x_coord, y_coord, body->radius, body->colour);
    }

    game_render_creator(ctx);
}
