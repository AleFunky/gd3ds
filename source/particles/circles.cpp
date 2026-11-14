#include "circles.hpp"
#include "easing.hpp"
#include "utils/gfx.hpp"
#include "state.hpp"
#include "main.hpp"
#include "level_loading.hpp"
#include <stdlib.h>

namespace gd3ds
{

extern const UseEffectDefinition padUseEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.3f,
    .startOpacity = 1,
    .endOpacity = 0,
    .startRad = 4,
    .endRad = 40,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
};
extern const UseEffectDefinition orbUseEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .startOpacity = 0.03f,
    .endOpacity = 0.90f,
    .startRad = 37,
    .endRad = 3,
    .hollow = false,
    .trifading = true,
    .startOpacityEase = EaseTypes::CubicOut,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
};
extern const UseEffectDefinition portalUseEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .startOpacity = 0.01f,
    .endOpacity = 0.91f,
    .startRad = 48,
    .endRad = 0,
    .hollow = false,
    .trifading = true,
    .startOpacityEase = EaseTypes::CubicOut,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
};
extern const UseEffectDefinition orbCollideEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.35f,
    .startOpacity = .7f,
    .endOpacity = 0,
    .startRad = 0,
    .endRad = 65,
    .hollow = true,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseOut,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
    .lineThickness = 1.f
};
extern const UseEffectDefinition speedCollideEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.4f,
    .startOpacity = .7f,
    .endOpacity = 0,
    .startRad = 5,
    .endRad = 75,
    .hollow = true,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseOut,
    .startRadEase = EaseTypes::EaseOut,
    .endRadEase = EaseTypes::EaseOut,
    .lineThickness = 2.0f
};
extern const UseEffectDefinition deathEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.5f,
    .startOpacity = 0.9f,
    .endOpacity = 0,
    .startRad = 8,
    .endRad = 80,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
};
extern const UseEffectDefinition tapEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.25f,
    .startOpacity = 1,
    .endOpacity = 0,
    .startRad = 1,
    .endRad = 40,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
};
extern const UseEffectDefinition coinUseEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .startOpacity = 0.03f,
    .endOpacity = 0.90f,
    .startRad = 30,
    .endRad = 4,
    .hollow = false,
    .trifading = true,
    .startOpacityEase = EaseTypes::CubicOut,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
};
extern const UseEffectDefinition coinRadiusEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.35f,
    .startOpacity = .7f,
    .endOpacity = 0,
    .startRad = 0,
    .endRad = 65,
    .hollow = true,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseOut,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
    .lineThickness = 2.5f
};
extern const UseEffectDefinition waveRadiusEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.44f,
    .startOpacity = .7f,
    .endOpacity = 0,
    .startRad = 5,
    .endRad = 80,
    .hollow = true,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseOut,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
    .lineThickness = 2.0f
};
extern const UseEffectDefinition sizeChangeBigEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.4f,
    .startOpacity = 0.95f,
    .endOpacity = 0,
    .startRad = 5,
    .endRad = 40,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseOut,
    .endRadEase = EaseTypes::EaseOut,
}; 

extern const UseEffectDefinition endWallFilledFirst = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.5f,
    .startOpacity = 1.f,
    .endOpacity = 0,
    .startRad = 2.5f,
    .endRad = 250,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseOut,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
}; 

extern const UseEffectDefinition endWallFilledSecond = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.8f,
    .startOpacity = 1.f,
    .endOpacity = 0,
    .startRad = 2.5f,
    .endRad = SCREEN_WIDTH_AREA,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
}; 

extern const UseEffectDefinition endWallFilledTitle = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 1.f,
    .startOpacity = 0.9f,
    .endOpacity = 0,
    .startRad = 25.f,
    .endRad = 250.f,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::QuadOut,
    .endRadEase = EaseTypes::QuadOut,
}; 

extern const UseEffectDefinition endWallFireworkCircle = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.5f,
    .startOpacity = 0.f,
    .endOpacity = 1.f,
    .startRad = 10.f,
    .endRad = 42.5f,
    .hollow = false,
    .trifading = true,
    .startOpacityEase = EaseTypes::CubicOut,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
}; 

extern const UseEffectDefinition endWallCircunference = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 0,
    .duration = 0.5f,
    .startOpacity = 0.f,
    .endOpacity = 1.f,
    .startRad = 2.5f,
    .endRad = SCREEN_WIDTH_AREA,
    .hollow = true,
    .trifading = true,
    .startOpacityEase = EaseTypes::EaseLinear,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
    .lineThickness = 1.8f,
}; 

extern const UseEffectDefinition respawnEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.3f,
    .startOpacity = 0.1f,
    .endOpacity = 0.95f,
    .startRad = 67.f,
    .endRad = 3.f,
    .hollow = true,
    .trifading = true,
    .startOpacityEase = EaseTypes::CubicOut,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseLinear,
    .endRadEase = EaseTypes::EaseLinear,
    .lineThickness = 1.f
}; 

const UseEffectDefinition spiderInEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.2f,
    .startOpacity = 0.95f,
    .endOpacity = 0.f,
    .startRad = 25.f,
    .endRad = 3.f,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseIn,
    .endRadEase = EaseTypes::EaseLinear,
}; 

const UseEffectDefinition spiderOutEffect = {
    .colorR = 1, 
    .colorG = 1,
    .colorB = 1,
    .duration = 0.1f,
    .startOpacity = 0.95f,
    .endOpacity = 0.f,
    .startRad = 10.f,
    .endRad = 3.f,
    .hollow = false,
    .trifading = false,
    .startOpacityEase = EaseTypes::EaseIn,
    .endOpacityEase = EaseTypes::EaseLinear,
    .startRadEase = EaseTypes::EaseIn,
    .endRadEase = EaseTypes::EaseLinear,
}; 

UseEffectPool useEffectsTop = {
    .stationary = false
};
UseEffectPool useEffectsTopAboveLevel = {
    .stationary = false
};
UseEffectPool useEffectsBot = {
    .stationary = true
};

void initDefaultUseEffectPools() {
    initUseEffectPool(&useEffectsTop, MAX_USE_EFFECTS);
    initUseEffectPool(&useEffectsTopAboveLevel, MAX_USE_EFFECTS);
    initUseEffectPool(&useEffectsBot, MAX_USE_EFFECTS);
}

UseEffectPool *getUseEffectArrayPtr(int screen) {
    switch (screen) {
        case GFX_TOP:
            return &useEffectsTop;
            
        case GFX_BOTTOM:
            return &useEffectsBot;
        
        case GFX_TOP_BUT_ABOVE_LEVEL:
            return &useEffectsTopAboveLevel;
    }
    return &useEffectsTop;
}

void initUseEffectPool(UseEffectPool *pool, int capacity) {
    pool->pool = (UseEffect *)malloc(sizeof(UseEffect) * capacity);
    if (pool->pool) {
        memset(pool->pool, 0, sizeof(UseEffect) * capacity);
    }
    pool->capacity = capacity;
}

UseEffect *addUseEffect(float x, float y, int obj, const UseEffectDefinition *def, UseEffectPool *pool) {
    UseEffect *ptr = pool->pool;
    if (!ptr) return NULL;

    for (size_t i = 0; i < pool->capacity; i++) {
        UseEffect *effect = &ptr[i];
        if (!effect->active) {
            effect->active = true;

            effect->x = x;
            effect->y = y;
            
            // Struct copy
            effect->def = *def;

            effect->obj = obj;

            effect->midRad = (effect->def.endRad + effect->def.startRad) / 2;
            effect->midOpacity = (effect->def.endOpacity + effect->def.startOpacity) / 2;

            effect->elapsed = 0;

            effect->index = i;
            return effect;
        }
    }
    return NULL;
}

void updateUseEffects(float delta, UseEffectPool *pool) {
    UseEffect *ptr = pool->pool;
    if (!ptr) return;

    for (size_t i = 0; i < pool->capacity; i++) {
        UseEffect *effect = &ptr[i];
        if (effect->active) {
            if (effect->obj >= 0) {
                effect->x = objects.x[effect->obj];
                effect->y = objects.y[effect->obj];
            }
            float progress = (effect->elapsed / effect->def.duration);
            float durationHalved = effect->def.duration / 2;

            float opacity;
            if (effect->def.trifading) {
                if (progress < 0.5f) {
                    opacity = easeValue(effect->def.startOpacityEase, effect->def.startOpacity, effect->def.endOpacity, effect->elapsed, durationHalved, 2.f);
                } else {
                    opacity = easeValue(effect->def.endOpacityEase, effect->def.endOpacity, effect->def.startOpacity, effect->elapsed - durationHalved, durationHalved, 2.f);
                }
            } else {
                // Merge both easings if both are the same
                if (effect->def.startOpacityEase == effect->def.endOpacityEase) {
                    opacity = easeValue(effect->def.startOpacityEase, effect->def.startOpacity, effect->def.endOpacity, effect->elapsed, effect->def.duration, 2.f);
                } else {
                    if (progress < 0.5f) {
                        opacity = easeValue(effect->def.startOpacityEase, effect->def.startOpacity, effect->midOpacity, effect->elapsed, durationHalved, 2.f);
                    } else {
                        opacity = easeValue(effect->def.endOpacityEase, effect->midOpacity, effect->def.endOpacity, effect->elapsed - durationHalved, durationHalved, 2.f);
                    }
                }
            }

            effect->opacity = opacity;

            float rad;

            // Merge both easings if both are the same
            if (effect->def.startRadEase == effect->def.endRadEase) {
                rad = easeValue(effect->def.startRadEase, effect->def.startRad, effect->def.endRad, effect->elapsed, effect->def.duration, 2.f);
            } else {
                if (progress < 0.5f) {
                    rad = easeValue(effect->def.startRadEase, effect->def.startRad, effect->midRad, effect->elapsed, durationHalved, 2.f);
                } else {
                    rad = easeValue(effect->def.endRadEase, effect->midRad, effect->def.endRad, effect->elapsed - durationHalved, durationHalved, 2.f);
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

void clearUseEffects(UseEffectPool *pool) {
    UseEffect *ptr = pool->pool;
    if (!ptr) return;

    for (size_t i = 0; i < pool->capacity; i++) {
        ptr[i].active = false;
    }
}

void drawUseEffects(UseEffectPool *pool) {
    UseEffect *ptr = pool->pool;
    if (!ptr) return;

    for (size_t i = 0; i < pool->capacity; i++) {
        UseEffect *effect = &ptr[i];
        if (effect->active) {
            float x = effect->x;
            float y = effect->y;
            float size = effect->rad;

            float r = effect->def.colorR;
            float g = effect->def.colorG;
            float b = effect->def.colorB;
            float a = effect->opacity;

            float fadeX = 0;
            float fadeY = 0;

            float fadeScale = 1.f;
            float opacity = 1.f;

            // If stationary, dont convert to screen space
            if (!pool->stationary) {
                // If obj is positive, it is an object, else it is a player
                if (effect->obj >= 0) { // Obj
                    float tmpX = (x - state.cameraX);
                    float fadeScale = 1.f;
                    getFadeVars(effect->obj, tmpX, &fadeX, &fadeY, &fadeScale);

                    opacity = objEdgeFade(tmpX, SCREEN_WIDTH / SCALE) / 255.f;

                    x = getMirrorX(tmpX + fadeX, state.mirrorFactor);
                    y = GSP_SCREEN_WIDTH - ((y - state.cameraY));  
                } else if (effect->obj == USE_EFFECT_OBJ_NOTHING) { // No objects
                    x = getMirrorX((x - state.cameraX), state.mirrorFactor);
                    y = GSP_SCREEN_WIDTH - ((y - state.cameraY));  
                } else { // Player
                    float tmpX;
                    float tmpY;
                    if (effect->obj == USE_EFFECT_OBJ_P1) {
                        tmpX = state.player.x;
                        tmpY = state.player.y;
                    } else { // P2
                        tmpX = state.player2.x;
                        tmpY = state.player2.y;
                    }
                    
                    x = getMirrorX(tmpX - state.cameraX, state.mirrorFactor);
                    y = GSP_SCREEN_WIDTH - ((tmpY - state.cameraY));
                }
            }

            u32 color = C2D_Color32f(r, g, b, a * opacity);

            if (effect->def.hollow) {
                customCircunference(x, y + fadeY, size * fadeScale, color, effect->def.lineThickness);
            } else {
                customCircle(x, y + fadeY, size * fadeScale, color);
            }
        }
    }
}

}