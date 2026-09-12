#pragma once

#include <stddef.h>

typedef struct {
 int atlas;
 int texture;
 float x,y;
 float scale_x, scale_y;
 int flip_x, flip_y;
 int z;
 float rot;
 int color_type;
 float opacity;
} IconPart;

typedef struct {
 int part_count;
 const IconPart* parts;
} Icon;

typedef enum {
 GAMEMODE_PLAYER,
 GAMEMODE_SHIP,
 GAMEMODE_BALL,
 GAMEMODE_UFO,
 GAMEMODE_WAVE,
 GAMEMODE_COUNT
} IconGamemode;

#define TRAIL 5

#define ICON_COUNT_PLAYER 485
#define ICON_COUNT_SHIP 169
#define ICON_COUNT_PLAYER_BALL 118
#define ICON_COUNT_BIRD 149
#define ICON_COUNT_DART 96
#define TRAIL_COUNT 17

#define ATLAS_COUNT_PLAYER 2
#define ATLAS_COUNT_SHIP 1
#define ATLAS_COUNT_PLAYER_BALL 1
#define ATLAS_COUNT_BIRD 1
#define ATLAS_COUNT_DART 1

extern const Icon* icons[GAMEMODE_COUNT];