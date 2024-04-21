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

#define creator_ring_thickness              0.004f
#define creator_displacement_line_thickness 0.004f

#define game_cursor_radius_min 0.01f
#define game_cursor_radius_max 0.1f
#define game_collision_dampen 0.3f

