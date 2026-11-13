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

namespace gd3ds
{

typedef struct {
    float colorR;
    float colorG;
    float colorB;

    float duration;

    float startOpacity;
    float endOpacity;

    float startRad;
    float endRad;

    bool hollow;
    bool trifading;

    EaseTypes startOpacityEase;
    EaseTypes endOpacityEase;

    EaseTypes startRadEase;
    EaseTypes endRadEase;

    float lineThickness;
} UseEffectDefinition;

typedef struct {
    float x;
    float y;

    float elapsed;
    float opacity;

    float rad;

    float midRad;
    float midOpacity;
    
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

extern const UseEffectDefinition padUseEffect;
extern const UseEffectDefinition orbUseEffect;
extern const UseEffectDefinition orbCollideEffect;
extern const UseEffectDefinition speedCollideEffect;
extern const UseEffectDefinition portalUseEffect;
extern const UseEffectDefinition deathEffect;
extern const UseEffectDefinition tapEffect;
extern const UseEffectDefinition coinUseEffect;
extern const UseEffectDefinition coinRadiusEffect;
extern const UseEffectDefinition waveRadiusEffect;
extern const UseEffectDefinition sizeChangeBigEffect;
extern const UseEffectDefinition endWallFilledFirst;
extern const UseEffectDefinition endWallFilledSecond;
extern const UseEffectDefinition endWallFilledTitle;
extern const UseEffectDefinition endWallFireworkCircle;
extern const UseEffectDefinition endWallCircunference;
extern const UseEffectDefinition respawnEffect;
extern const UseEffectDefinition spiderInEffect;
extern const UseEffectDefinition spiderOutEffect;


void initDefaultUseEffectPools();
void initUseEffectPool(UseEffectPool *pool, int capacity);
UseEffectPool *getUseEffectArrayPtr(int screen);
UseEffect *addUseEffect(float x, float y, int obj, const UseEffectDefinition *def, UseEffectPool *pool);
void updateUseEffects(float delta, UseEffectPool *pool);
void drawUseEffects(UseEffectPool *pool);
void clearUseEffects(UseEffectPool *pool);

}