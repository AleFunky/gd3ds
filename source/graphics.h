#pragma once
#include <citro2d.h>
#include "level_loading.h"
#include "color_channels.h"

#define FADING_OBJ_PADDING 90
#define FADING_OBJ_WIDTH 180

#define FADE_WIDTH 75

#define BACKGROUND_SCALE 0.9f

#define GROUND_SIZE 128
#define LINE_WIDTH 444
#define LINE_HEIGHT 2

#define MAX_SPRITES   8192
#define SCALE (SCREEN_HEIGHT / (11.f * 30))

#define SCREEN_HEIGHT_AREA (11.f*30)

#define CAMERA_Y_OFFSET (SCREEN_HEIGHT_AREA - (SCREEN_HEIGHT_AREA * SCALE))

extern bool particlesDisabled;
extern bool wideEnabled;
extern bool glowEnabled; 

extern int current_pulserod_ball_image;

// Simple sprite struct
typedef struct
{
    C2D_Sprite spr;
    float dx, dy; // velocity
} Sprite;

typedef struct
{
    C2D_Sprite spr;
    C2D_ImageTint tint;
    int obj;
    int layer;
    int col_type;
    float opacity;
    int col_channel;
    int zlayer;
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
    FADE_UP_SLOW,
    FADE_DOWN_SLOW,
    FADE_UP_STATIONARY,
    FADE_DOWN_STATIONARY,
    FADE_COUNT
};

typedef struct {
    C2D_Sprite parent_template;
    C2D_Sprite glow_template;
    int child_count;
    C2D_Sprite *child_templates;
} SpriteTemplate;

void cache_all_sprites();
void free_cached_sprites();
void get_fade_vars(int obj, float x, int *fade_x, int *fade_y, float *fade_scale);
int obj_edge_fade(float x, int right_edge);

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

extern SpriteTemplate sprite_templates[GAME_OBJECT_COUNT];

extern const Color white;

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
void draw_objects();
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
void handle_mirror_transition();