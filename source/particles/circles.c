#include "circles.h"
#include "easing.h"
#include "utils/gfx.h"
#include "state.h"
#include "main.h"

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
    .start_opacity = 0.01f,
    .end_opacity = 1,
    .start_rad = 32,
    .end_rad = 3.2f,
    .hollow = false,
    .trifading = true,
    .start_opacity_ease = EASE_OUT,
    .end_opacity_ease = EASE_LINEAR,
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
    .start_rad = 48,
    .end_rad = 3,
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
    .duration = 0.35f,
    .start_opacity = .7f,
    .end_opacity = 0,
    .start_rad = 0,
    .end_rad = 65,
    .hollow = true,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_OUT,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
    .line_thickness = 1.f
};
const UseEffectDefinition speed_collide_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.4f,
    .start_opacity = .7f,
    .end_opacity = 0,
    .start_rad = 5,
    .end_rad = 75,
    .hollow = true,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_OUT,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
    .line_thickness = 2.0f
};
const UseEffectDefinition death_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.5f,
    .start_opacity = 0.9f,
    .end_opacity = 0,
    .start_rad = 8,
    .end_rad = 80,
    .hollow = false,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_LINEAR,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
};
const UseEffectDefinition tap_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.25f,
    .start_opacity = 1,
    .end_opacity = 0,
    .start_rad = 1,
    .end_rad = 40,
    .hollow = false,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_LINEAR,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
};
const UseEffectDefinition coin_use_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .start_opacity = 0.01f,
    .end_opacity = 1,
    .start_rad = 32,
    .end_rad = 3.2f,
    .hollow = false,
    .trifading = true,
    .start_opacity_ease = EASE_OUT,
    .end_opacity_ease = EASE_LINEAR,
    .start_rad_ease = EASE_IN,
    .end_rad_ease = EASE_LINEAR,
};
const UseEffectDefinition coin_radius_effect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.35f,
    .start_opacity = .7f,
    .end_opacity = 0,
    .start_rad = 0,
    .end_rad = 65,
    .hollow = true,
    .trifading = false,
    .start_opacity_ease = EASE_IN,
    .end_opacity_ease = EASE_OUT,
    .start_rad_ease = EASE_OUT,
    .end_rad_ease = EASE_OUT,
    .line_thickness = 1.f
};

UseEffect use_effects_top[MAX_USE_EFFECTS];
UseEffect use_effects_bot[MAX_USE_EFFECTS];

UseEffect *add_use_effect(float x, float y, int obj, const UseEffectDefinition *def, int screen) {
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

void clear_use_effects(int screen) {
    UseEffect *ptr = (screen == GFX_TOP) ? use_effects_top : use_effects_bot;
    for (size_t i = 0; i < MAX_USE_EFFECTS; i++) {
        ptr[i].active = false;
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

            // If stationary, dont convert to screen space
            if (screen == GFX_TOP) {
                x = get_mirror_x((x - state.camera_x), state.mirror_factor);
                y = GSP_SCREEN_WIDTH - ((y - state.camera_y));  
            }

            int fade_x = 0;
            int fade_y = 0;

            float fade_scale = 1.f;
            float opacity = 1.f;

            if (screen == GFX_TOP) {
                float fade_scale = 1.f;
                get_fade_vars(effect->obj, x, &fade_x, &fade_y, &fade_scale);

                opacity = obj_edge_fade(x, SCREEN_WIDTH / SCALE) / 255.f;
            }

            u32 color = C2D_Color32f(r, g, b, a * opacity);
            

            if (effect->def.hollow) {
                custom_circunference(x + fade_x, y + fade_y, size * fade_scale, color, effect->def.line_thickness);
            } else {
                custom_circle(x + fade_x, y + fade_y, size * fade_scale, color);
            }
        }
    }
}