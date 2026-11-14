#pragma once

#include <stddef.h>
#include <stdint.h>

#define SPRITESHEET2_START 766
#define SPRITESHEET3_START (SPRITESHEET2_START + 72)
#define SPRITESHEET4_START (SPRITESHEET3_START + 601)
#define ANIMATEDSHEET_START (SPRITESHEET4_START + 673)

namespace gd3ds
{

enum class CollisionShape {
   CollisionNone = 0,
   CollisionBox,
   CollisionCircle,
   CollisionSlope
};

enum class ColorType {
   ColorTypeBase = 0,
   ColorTypeDetail,
   ColorTypeBlack,
   ColorTypeWhite,
   ColorTypeGlow,
   ColorTypeOnlyDetail,};

enum class HitboxType {
   HitboxSpecial = 0,
   HitboxSolid,
   HitboxHazard
};

enum class AnimationType {
   AnimationNone = 0,
   AnimationMovement,
   AnimationFrameSwap
};

struct ChildSprite {
    int texture;
    float x, y;
    float scaleX, scaleY;
    int flipX, flipY;
    int z;
    int zLayerOffset;
    float rot;
    int colorType;
    float opacity;
};

struct ChildGroup {
    size_t start;
    size_t count;
};

struct SwapFrame {
    int texture;
    int flipX, flipY;
};

struct SlotFrames {
    int slot;
    float fps;
    size_t start;
    size_t count;
};

struct ObjectHitbox {
   int collisionType;
   float x, y;
   float width, height;
   int type;
};

struct GameObject {
    int texture;
    int glowFrame;
    float x, y;
    int zLayer;
    int zOrder;
    int baseColor;
    int detailColor;
    int colorType;
    int swapBaseDetail;
    int animationType;
    float opacity;
    size_t childCount;
    const ChildSprite* children;
    size_t groupCount;
    const ChildGroup* groups;
    size_t frameCount;
    const SwapFrame*  swapFrames;
    size_t slotCount;
    const SlotFrames* slotFrames;
    const ObjectHitbox* hitbox;
};

#define TEXTURE_COUNT 2472
#define GAME_OBJECT_COUNT 1912

extern const GameObject gameObjects[GAME_OBJECT_COUNT];

}
