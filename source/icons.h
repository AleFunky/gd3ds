#pragma once

#include <stddef.h>

typedef struct {
 int texture;
 float x,y;
 float scale_x, scale_y;
 int flip_x, flip_y;
 int z;
 float rot;
 int color_type;
 float opacity;
} IconPart;

typedef struct {;
 int part_count;
 const IconPart* parts;
} Icon;

typedef enum {
 GAMEMODE_PLAYER,
 GAMEMODE_SHIP,
 GAMEMODE_PLAYER_BALL,
 GAMEMODE_BIRD,
 GAMEMODE_DART,
 GAMEMODE_COUNT
} IconGamemode;

#define TRAIL 5

#define ICON_COUNT_PLAYER 142
#define ICON_COUNT_SHIP 51
#define ICON_COUNT_PLAYER_BALL 43
#define ICON_COUNT_BIRD 35
#define ICON_COUNT_DART 35
#define TRAIL_COUNT 17

extern const Icon* icons[GAMEMODE_COUNT];