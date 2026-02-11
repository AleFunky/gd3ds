#pragma once
#include <citro2d.h>
#include "level_loading.h"

#define FADING_OBJ_PADDING 90
#define FADING_OBJ_WIDTH 180

#define FADE_WIDTH 75

#define BACKGROUND_SCALE 0.9f

#define GROUND_SIZE 128
#define LINE_WIDTH 444
#define LINE_HEIGHT 2

#define MAX_SPRITES   8192
#define SCALE (SCREEN_HEIGHT / (11.f * 30))

// Simple sprite struct
typedef struct
{
	C2D_Sprite spr;
	float dx, dy; // velocity
} Sprite;

typedef struct
{
	C2D_Sprite spr;
	Object *obj;
	int layer;
	int col_type;
	float opacity;
	int col_channel;
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

extern int current_fading_effect;

extern int sprite_count;
extern C2D_SpriteSheet spriteSheet;
extern C2D_SpriteSheet spriteSheet2;
extern C2D_SpriteSheet bgSheet;
extern C2D_SpriteSheet groundSheet;

void draw_objects();
void draw_background(float x, float y);
void draw_ground(float y, bool is_ceiling);