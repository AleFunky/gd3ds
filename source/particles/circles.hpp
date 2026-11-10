#pragma once
#include <3ds.h>
#include "easing.hpp"

#define MAX_USE_EFFECTS 32
#define MAX_UI_USE_EFFECTS 32

#define USE_EFFECT_OBJ_NOTHING -3
#define USE_EFFECT_OBJ_P2 -2
#define USE_EFFECT_OBJ_P1 -1

// Stupid hack lmao
#define GFX_TOP_BUT_ABOVE_LEVEL (2)

typedef struct {
    float colorR;
    float colorG;
    float colorB;

    float duration;

    float start_opacity;
    float end_opacity;

    float start_rad;
    float end_rad;

    bool hollow;
    bool trifading;

    EaseTypes start_opacity_ease;
    EaseTypes end_opacity_ease;

    EaseTypes start_rad_ease;
    EaseTypes end_rad_ease;

    float line_thickness;
} UseEffectDefinition;

typedef struct {
    float x;
    float y;

    float elapsed;
    float opacity;

    float rad;

    float mid_rad;
    float mid_opacity;
    
    UseEffectDefinition def;

    int obj;

    //where in the pool this UseEffect is
    int index;

    bool active;
} UseEffect;

typedef struct {
    UseEffect *pool;
    size_t capacity;
    //whether the effect should have mirror, fading, and world-space transforms
    bool stationary;
} UseEffectPool;

extern const UseEffectDefinition pad_use_effect;
extern const UseEffectDefinition orb_use_effect;
extern const UseEffectDefinition orb_collide_effect;
extern const UseEffectDefinition speed_collide_effect;
extern const UseEffectDefinition portal_use_effect;
extern const UseEffectDefinition death_effect;
extern const UseEffectDefinition tap_effect;
extern const UseEffectDefinition coin_use_effect;
extern const UseEffectDefinition coin_radius_effect;
extern const UseEffectDefinition wave_radius_effect;
extern const UseEffectDefinition size_change_big_effect;
extern const UseEffectDefinition end_wall_filled_first;
extern const UseEffectDefinition end_wall_filled_second;
extern const UseEffectDefinition end_wall_filled_title;
extern const UseEffectDefinition end_wall_firework_circle;
extern const UseEffectDefinition end_wall_circunference;
extern const UseEffectDefinition respawn_effect;
extern const UseEffectDefinition spider_in_effect;
extern const UseEffectDefinition spider_out_effect;


void init_default_use_effect_pools();
void init_use_effect_pool(UseEffectPool *pool, int capacity);
UseEffectPool *get_use_effect_array_ptr(int screen);
UseEffect *add_use_effect(float x, float y, int obj, const UseEffectDefinition *def, UseEffectPool *pool);
void update_use_effects(float delta, UseEffectPool *pool);
void draw_use_effects(UseEffectPool *pool);
void clear_use_effects(UseEffectPool *pool);
