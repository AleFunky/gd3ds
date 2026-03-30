#include "circles.h"
#include "easing.h"
#include "utils/gfx.h"
#include "state.h"

const UseEffectDefinition pad_use_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.25f,
    .start_opacity = 1,
    .end_opacity = 0,
    .start_rad = 4,
    .end_rad = 40,
    .hollow = false,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_LINEAR,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
};
const UseEffectDefinition orb_use_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .start_opacity = 0.05f,
    .end_opacity = 1,
    .start_rad = 32,
    .end_rad = 3.2f,
    .hollow = false,
    .trifading = true,
    .start_opacity_ease = EASE_OUT,
    .end_opacity_ease = EASE_IN,
    .start_rad_ease = EASE_IN,
    .end_rad_ease = EASE_LINEAR,
};
const UseEffectDefinition portal_use_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .start_opacity = 0.05f,
    .end_opacity = 1,
    .start_rad = 50,
    .end_rad = 5,
    .hollow = false,
    .trifading = true,
    .start_opacity_ease = EASE_LINEAR,
    .end_opacity_ease = EASE_IN,
    .start_rad_ease = EASE_LINEAR,
    .end_rad_ease = EASE_LINEAR,
};
const UseEffectDefinition orb_collide_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.3f,
    .start_opacity = 1,
    .end_opacity = 0,
    .start_rad = 5,
    .end_rad = 60,
    .hollow = true,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_OUT,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
};
UseEffect use_effects_top[MAX_USE_EFFECTS];
UseEffect use_effects_bot[MAX_USE_EFFECTS];

UseEffect *add_use_effect(float x, float y, const UseEffectDefinition *def, int screen) {
    UseEffect *ptr = (screen == GFX_TOP) ? use_effects_top : use_effects_bot;

    for (size_t i = 0; i < MAX_USE_EFFECTS; i++) {
        UseEffect *effect = &ptr[i];
        if (!effect->active) {
            effect->active = true;

            effect->x = x;
            effect->y = y;
            
            // Struct copy
            effect->def = *def;

            effect->mid_rad = (effect->def.end_rad + effect->def.start_rad) / 2;
            effect->mid_opacity = (effect->def.end_opacity + effect->def.start_opacity) / 2;

            effect->elapsed = 0;
            return effect;
        }
    }
    return NULL;
}

void update_use_effects(float delta, int screen) {
    UseEffect *ptr = (screen == GFX_TOP) ? use_effects_top : use_effects_bot;
    for (size_t i = 0; i < MAX_USE_EFFECTS; i++) {
        UseEffect *effect = &ptr[i];
        if (effect->active) {
            float progress = (effect->elapsed / effect->def.duration);
            float duration_halved = effect->def.duration / 2;

            float opacity;
            if (effect->def.trifading) {
                if (progress < 0.5f) {
                    opacity = easeValue(effect->def.start_opacity_ease, effect->def.start_opacity, effect->def.end_opacity, effect->elapsed, duration_halved, 2.f);
                } else {
                    opacity = easeValue(effect->def.end_opacity_ease, effect->def.end_opacity, effect->def.start_opacity, effect->elapsed - duration_halved, duration_halved, 2.f);
                }
            } else {
                // Merge both easings if both are the same
                if (effect->def.start_opacity_ease == effect->def.end_opacity_ease) {
                    opacity = easeValue(effect->def.start_opacity_ease, effect->def.start_opacity, effect->def.end_opacity, effect->elapsed, effect->def.duration, 2.f);
                } else {
                    if (progress < 0.5f) {
                        opacity = easeValue(effect->def.start_opacity_ease, effect->def.start_opacity, effect->mid_opacity, effect->elapsed, duration_halved, 2.f);
                    } else {
                        opacity = easeValue(effect->def.end_opacity_ease, effect->mid_opacity, effect->def.end_opacity, effect->elapsed - duration_halved, duration_halved, 2.f);
                    }
                }
            }

            effect->opacity = get_opacity(opacity);

            float rad;

            // Merge both easings if both are the same
            if (effect->def.start_rad_ease == effect->def.end_rad_ease) {
                rad = easeValue(effect->def.start_rad_ease, effect->def.start_rad, effect->def.end_rad, effect->elapsed, effect->def.duration, 2.f);
            } else {
                if (progress < 0.5f) {
                    rad = easeValue(effect->def.start_rad_ease, effect->def.start_rad, effect->mid_rad, effect->elapsed, duration_halved, 2.f);
                } else {
                    rad = easeValue(effect->def.end_rad_ease, effect->mid_rad, effect->def.end_rad, effect->elapsed - duration_halved, duration_halved, 2.f);
                }
            }

            effect->rad = rad;

            effect->elapsed += delta;
            if (effect->elapsed >= effect->def.duration) {
                effect->active = false;
            }
        }
    }
}

void draw_use_effects(int screen) {
    UseEffect *ptr = (screen == GFX_TOP) ? use_effects_top : use_effects_bot;
    for (size_t i = 0; i < MAX_USE_EFFECTS; i++) {
        UseEffect *effect = &ptr[i];
        if (effect->active) {
            float x = effect->x;
            float y = effect->y;
            float size = effect->rad;

            float r = effect->def.colorR;
            float g = effect->def.colorG;
            float b = effect->def.colorB;
            float a = effect->opacity;

            u32 color = C2D_Color32f(r, g, b, a);

            // If stationary, dont convert to screen space
            if (screen == GFX_TOP) {
                x = get_mirror_x((x - state.camera_x), state.mirror_factor);
                y = GSP_SCREEN_WIDTH - ((y - state.camera_y));  
            }

            if (effect->def.hollow) {
                custom_circunference(x, y, size, color, 2);
            } else {
                custom_circle(x, y, size, color);
            }
        }
    }
}
