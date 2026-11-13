#pragma once

#include <stddef.h>
#include <stdint.h>

#define SPRITESHEET2_START 766
#define SPRITESHEET3_START (SPRITESHEET2_START + 72)
#define SPRITESHEET4_START (SPRITESHEET3_START + 601)
#define ANIMATEDSHEET_START (SPRITESHEET4_START + 673)

namespace gd3ds
{

typedef enum {
   COLLISION_NONE = 0,
   COLLISION_BOX,
   COLLISION_CIRCLE,
   COLLISION_SLOPE
} CollisionShape;

typedef enum {
   COLOR_TYPE_BASE = 0,
   COLOR_TYPE_DETAIL,
   COLOR_TYPE_BLACK,
   COLOR_TYPE_WHITE,
   COLOR_TYPE_GLOW,
   COLOR_TYPE_ONLY_DETAIL,} ColorType;

typedef enum {
   HITBOX_SPECIAL = 0,
   HITBOX_SOLID,
   HITBOX_HAZARD
} HitboxType;

typedef enum {
   ANIMATION_NONE = 0,
   ANIMATION_MOVEMENT,
   ANIMATION_FRAME_SWAP
} AnimationType;

typedef struct {
    int texture;
    float x, y;
    float scaleX, scaleY;
    int flipX, flipY;
    int z;
    int zLayerOffset;
    float rot;
    int colorType;
    float opacity;
} ChildSprite;

typedef struct {
    size_t start;
    size_t count;
} ChildGroup;

typedef struct {
    int texture;
    int flipX, flipY;
} SwapFrame;

typedef struct {
    int slot;
    float fps;
    size_t start;
    size_t count;
} SlotFrames;

typedef struct {
   int collisionType;
   float x, y;
   float width, height;
   int type;
} ObjectHitbox;

typedef struct {
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
} GameObject;

#define TEXTURE_COUNT 2472
#define GAME_OBJECT_COUNT 1912

extern const GameObject gameObjects[GAME_OBJECT_COUNT];

}
