#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t childSlot;
    float x, y;
    float scaleX, scaleY;
    int flipX, flipY;
    float rot;
    int z;
} AnimSprite;

typedef struct {
    const AnimSprite* sprites;
    size_t spriteCount;
} AnimFrame;

typedef struct {
    const AnimFrame* frames;
    int frameCount;
    float fps;
} Animation;

typedef enum {
    ANIM_BLACKSLUDGE_LOOP = 0,
    ANIM_GJBEAST01_ATTACK01 = 1,
    ANIM_GJBEAST01_ATTACK01_END = 2,
    ANIM_GJBEAST01_ATTACK01_LOOP = 3,
    ANIM_GJBEAST01_BITE = 4,
    ANIM_GJBEAST01_IDLE01 = 5,
    ANIM_GJBEAST02_IDLE01 = 6,
    ANIM_GJBEAST02_IDLE02 = 7,
    ANIM_GJBEAST03_IDLE01 = 8,
    ANIM_GJBEAST03_IDLE02 = 9
} AnimationID;

#define ANIMATION_COUNT 10

extern const Animation animations[ANIMATION_COUNT];
