#pragma once

#include <stddef.h>
#include <stdint.h>

#define SPRITESHEET2_START 767
#define SPRITESHEET3_START (SPRITESHEET2_START + 64)
#define ANIMATEDSHEET_START (SPRITESHEET3_START + 607)

typedef enum {
   COLLISION_NONE = 0,
   COLLISION_BOX,
   COLLISION_CIRCLE,
   COLLISION_SLOPE
} CollisionShape;

typedef enum {
   ANIMATION_NONE = 0,
   ANIMATION_MOVEMENT,
   ANIMATION_FRAME_SWAP
} AnimationType;

typedef enum {
   COLOR_TYPE_BASE = 0,
   COLOR_TYPE_DETAIL,
   COLOR_TYPE_BLACK,
   COLOR_TYPE_WHITE,
   COLOR_TYPE_GLOW,
} ColorType;

typedef enum {
   HITBOX_SPECIAL = 0,
   HITBOX_SOLID,
   HITBOX_HAZARD
} HitboxType;

typedef struct {
    int texture;
    float x, y;
    float scale_x, scale_y;
    int flip_x, flip_y;
    int z;
    int z_layer_offset;
    float rot;
    int color_type;
    float opacity;
} ChildSprite;

typedef struct {
    int start;
    int count;
} ChildGroup;

typedef struct {
    int texture;
    int flip_x, flip_y;
} SwapFrame;

typedef struct {
    int slot;
    float fps;
    int start;
    int count;
} SlotFrames;

typedef struct {
   int collision_type;
   float x, y;
   float width, height;
   int type;
} ObjectHitbox;

typedef struct {
    int texture;
    int glow_frame;
    float x, y;
    int z_layer;
    int z_order;
    int base_color;
    int detail_color;
    int color_type;
    int swap_base_detail;
    float opacity;
    int child_count;
    const ChildSprite* children;
    const ObjectHitbox* hitbox;
    int animation_type;
    int group_count;
    const ChildGroup* groups;
    int frame_count;
    const SwapFrame* swap_frames;
    int slot_count;
    const SlotFrames* slot_frames;
} GameObject;

#define TEXTURE_COUNT 1574
#define GAME_OBJECT_COUNT 1330

extern const GameObject game_objects[GAME_OBJECT_COUNT];
