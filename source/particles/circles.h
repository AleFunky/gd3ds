#pragma once
#include <3ds.h>
#include "easing.h"

#define MAX_USE_EFFECTS 32

typedef struct {
    float duration;

    float start_rad;
    float end_rad;

    float colorR;
    float colorG;
    float colorB;

    float start_opacity;
    float end_opacity;

    bool trifading;
    bool hollow;

    EaseTypes start_rad_ease;
    EaseTypes end_rad_ease;
    EaseTypes start_opacity_ease;
    EaseTypes end_opacity_ease;
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

    bool active;
} UseEffect;

extern const UseEffectDefinition pad_use_effect;
extern const UseEffectDefinition orb_use_effect;
extern const UseEffectDefinition orb_collide_effect;
extern const UseEffectDefinition portal_use_effect;

UseEffect *add_use_effect(float x, float y, const UseEffectDefinition *def, int screen);
void update_use_effects(float delta, int screen);
void draw_use_effects(int screen);
