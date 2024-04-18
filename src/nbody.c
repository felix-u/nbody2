typedef struct Body {
    f32 mass, radius, x, y, velocity_x, velocity_y;
    Color colour;
} Body;
typedef Array(Body) Array_Body;
