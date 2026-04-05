#pragma once

#include <stddef.h>
#include <stdint.h>

#define SPRITESHEET2_START 819

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
    int color_type;
    int swap_base_detail;
    float opacity;
    int child_count;
    const ChildSprite* children;
    const ObjectHitbox* hitbox;
} GameObject;

#define TEXTURE_COUNT 883
#define GAME_OBJECT_COUNT 1912

extern const GameObject game_objects[GAME_OBJECT_COUNT];
