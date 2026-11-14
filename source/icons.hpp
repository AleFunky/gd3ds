#pragma once

#include <stddef.h>

namespace gd3ds
{

enum class IconColorType {
 IconColorWhite = 0,
 IconColorP1,
 IconColorP2,
 IconColorGlow
};

struct IconPart {
 int atlas;
 int texture;
 float x,y;
 float scaleX, scaleY;
 int flipX, flipY;
 int z;
 float rot;
 IconColorType colorType;
 float opacity;
 int animationPart;
};

struct Icon {
 size_t partCount;
 const IconPart* parts;
};

enum class IconGamemode {
 GamemodePlayer,
 GamemodeShip,
 GamemodeBall,
 GamemodeUfo,
 GamemodeWave,
 GamemodeRobot,
 GamemodeSpider,
 GamemodeCount
};

#define ICON_GAMEMODE_COUNT 7

#define TRAIL 7

#define ICON_COUNT_PLAYER 486
#define ICON_COUNT_SHIP 170
#define ICON_COUNT_PLAYER_BALL 119
#define ICON_COUNT_BIRD 150
#define ICON_COUNT_DART 97
#define ICON_COUNT_ROBOT 69
#define ICON_COUNT_SPIDER 70

#define ATLAS_COUNT_PLAYER 2
#define ATLAS_COUNT_SHIP 1
#define ATLAS_COUNT_PLAYER_BALL 1
#define ATLAS_COUNT_BIRD 1
#define ATLAS_COUNT_DART 1
#define ATLAS_COUNT_ROBOT 1
#define ATLAS_COUNT_SPIDER 1
#define TRAIL_COUNT 17

extern const Icon* icons[ICON_GAMEMODE_COUNT];

}
