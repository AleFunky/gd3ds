#pragma once

#include <stddef.h>
#include <stdint.h>

namespace gd3ds
{

struct AnimSprite {
    size_t childSlot;
    float x, y;
    float scaleX, scaleY;
    int flipX, flipY;
    float rot;
    int z;
};

struct AnimFrame {
    const AnimSprite* sprites;
    size_t spriteCount;
};

struct Animation {
    const AnimFrame* frames;
    int frameCount;
    float fps;
};

enum class AnimationID {
    AnimBlacksludgeLoop = 0,
    AnimGjbeast01Attack01 = 1,
    AnimGjbeast01Attack01End = 2,
    AnimGjbeast01Attack01Loop = 3,
    AnimGjbeast01Bite = 4,
    AnimGjbeast01Idle01 = 5,
    AnimGjbeast02Idle01 = 6,
    AnimGjbeast02Idle02 = 7,
    AnimGjbeast03Idle01 = 8,
    AnimGjbeast03Idle02 = 9
};

#define ANIMATION_COUNT 10

extern const Animation animations[ANIMATION_COUNT];

}
