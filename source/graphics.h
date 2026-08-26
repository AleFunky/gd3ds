#pragma once
#include <citro2d.h>
#include "level_loading.h"
#include "color_channels.h"

#define FADING_OBJ_PADDING 45
#define FADING_OBJ_WIDTH 180

#define FADE_WIDTH 75

#define BACKGROUND_SCALE 0.9f

#define GROUND_SIZE 128
#define LINE_WIDTH 444
#define LINE_HEIGHT 2

#define MAX_SPRITES   8192

#define SCREEN_HEIGHT_AREA (320.f)
#define SCREEN_WIDTH_AREA ((SCREEN_HEIGHT_AREA / SCREEN_HEIGHT) * SCREEN_WIDTH)
#define SCREEN_WIDTH_AREA_BOT ((SCREEN_HEIGHT_AREA / SCREEN_HEIGHT) * SCREEN_BOT_WIDTH)

#define SCALE (SCREEN_HEIGHT / SCREEN_HEIGHT_AREA)


extern int current_pulserod_ball_image;

// Simple sprite struct
typedef struct
{
    C2D_Sprite spr;
    float dx, dy; // velocity
} Sprite;

typedef struct SpriteObject
{
    C2D_Image image;
    float x;
    float y;
    float offset_x;
    float offset_y;
    float half_width;
    float half_height;
    float rotation_sin;
    float rotation_cos;
    u32 color_meta;
    u32 fade_meta;
    int obj;
    int layer;
    float opacity;
    int col_channel;
    int render_slot;
    bool flip_x;
    bool flip_y;
    bool blending;
    bool visible;
} SpriteObject;

typedef struct {
    SpriteObject *obj;
    uint32_t key;
} SortItem;

enum FadingEffects {
    FADE_NONE,
    FADE_UP,
    FADE_DOWN,
    FADE_RIGHT,
    FADE_LEFT,
    FADE_SCALE_IN,
    FADE_SCALE_OUT,
    FADE_INWARDS,
    FADE_OUTWARDS,
    FADE_CIRCLE_LEFT,
    FADE_CIRCLE_RIGHT,
    FADE_UP_SLOW_LEFT,
    FADE_DOWN_SLOW_LEFT,
    FADE_UP_SLOW_RIGHT,
    FADE_DOWN_SLOW_RIGHT,
    FADE_UP_STATIONARY,
    FADE_DOWN_STATIONARY,
    FADE_COUNT
};

typedef struct {
    C2D_Sprite sprite;
    float rotation_sin;
    float rotation_cos;
} ChildSpriteTemplate;

typedef struct {
    C2D_Sprite parent_template;
    C2D_Sprite glow_template;
    int child_count;
    ChildSpriteTemplate *child_templates;
    float rotation_speed;
    int glow_channel;
    u8 pulse_mode;
    u8 fade_opacity_mode;
    bool fades;
    bool has_particles;
} SpriteTemplate;

void cache_all_sprites();
void free_cached_sprites();
void get_fade_vars(int obj, float x, int *fade_x, int *fade_y, float *fade_scale);
int obj_edge_fade(float x, int right_edge);
void get_special_fading_vars(int obj, float fade_val, float *calc_x);

extern bool p1_trail;
extern int current_fading_effect;

extern int sprite_count;
extern C2D_SpriteSheet spriteSheet;
extern C2D_SpriteSheet spriteSheet2;
extern C2D_SpriteSheet glowSheet;
extern C2D_SpriteSheet bgSheet;
extern C2D_SpriteSheet bg2Sheet;
extern C2D_SpriteSheet groundSheet;
extern C2D_SpriteSheet iconSheet;
extern C2D_SpriteSheet trailSheet;
extern C2D_SpriteSheet particleSheet;

extern SpriteTemplate sprite_templates[GAME_OBJECT_COUNT];

extern const Color white;

extern float object_pulse_scales[4];

extern float object_creating_time;
extern float object_sorting_time;
extern float object_drawing_time;

inline float normalize_angle(float a)
{
    while (a < 0.0f)   a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

void create_objects();
void change_blending(bool blending);
Color get_white_if_black(Color color);
Color get_p1_if_black(Color color);
Color get_p2_if_black(Color color);
void draw_objects();
void draw_attempt_text();
void draw_end_wall(float delta);
void draw_background(float x, float y);
void draw_ground(float cam_x, float cam_y, float y, bool is_ceiling, int screen_width);
void update_player_colors();
void set_player_colors(Color p1, Color p2, Color glow);
void spawn_icon_at(
    int gamemode,
    int id,
    bool glow,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 p1_color,
    u32 p2_color,
    u32 glow_color
);
void spawn_p1_layer_at(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 p1_color
);

void spawn_glow_layer_at(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 glow_color
);

Color get_color_abgr8(u32 color);

void handle_mirror_transition();

void draw_player_effects();
void draw_post_player_effects();
void draw_player_graphics();
int get_coin_texture(int tex, int ticks);

void update_touch_effect(float delta);
void draw_touch_effect();

void update_bottom_particles(float delta);
void draw_bottom_particles();
