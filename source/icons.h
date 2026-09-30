#pragma once

#include <stddef.h>

typedef enum {
 ICON_COLOR_WHITE = 0,
 ICON_COLOR_P1,
 ICON_COLOR_P2,
 ICON_COLOR_GLOW
} IconColorType;

typedef struct {
 int atlas;
 int texture;
 float x,y;
 float scale_x, scale_y;
 int flip_x, flip_y;
 int z;
 float rot;
 IconColorType color_type;
 float opacity;
 int animation_part;
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
 GAMEMODE_ROBOT,
 GAMEMODE_COUNT
} IconGamemode;

#define GAMEMODE_COUNT 6
#define ICON_GAMEMODE_COUNT 6

#define TRAIL 6

#define ICON_COUNT_PLAYER 486
#define ICON_COUNT_SHIP 170
#define ICON_COUNT_PLAYER_BALL 119
#define ICON_COUNT_BIRD 150
#define ICON_COUNT_DART 97
#define ICON_COUNT_ROBOT 69

#define ATLAS_COUNT_PLAYER 2
#define ATLAS_COUNT_SHIP 1
#define ATLAS_COUNT_PLAYER_BALL 1
#define ATLAS_COUNT_BIRD 1
#define ATLAS_COUNT_DART 1
#define ATLAS_COUNT_ROBOT 1
#define TRAIL_COUNT 17

extern const Icon* icons[ICON_GAMEMODE_COUNT];