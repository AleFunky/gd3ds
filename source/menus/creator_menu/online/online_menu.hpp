#pragma once
#include "menus/core/ui_stack.hpp"

#define EASY_DEMON_FACE 260
#define MEDIUM_DEMON_FACE 262
#define HARD_DEMON_FACE 258
#define INSANE_DEMON_FACE 264
#define EXTREME_DEMON_FACE 266

#define FEATURED_GLOW 72
#define EPIC_GLOW 68
#define SUPER_GLOW 427
#define LEGENDARY_GLOW 69
#define MYTHIC_GLOW 70

namespace gd3ds
{

extern int currSearchID;
extern int redownload;

extern const int demonFaces[6];
extern const int difficultyFaces[6];
extern const int epics[4];

extern const UIScreenDefPair onlineDef;

typedef struct {
    bool wasUpdated;
    float version;
} VersionWarningData;

typedef struct {
    int entryId;
    int levelId;
    int redownload;
} OnlineCardData;

}