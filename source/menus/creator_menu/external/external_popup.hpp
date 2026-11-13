#pragma once
#include <3ds.h>

#define MAX_STARS 10

#define NA_FACE     251
#define AUTO_FACE   267
#define EASY_FACE   252
#define NORMAL_FACE 253
#define HARD_FACE   254
#define HARDER_FACE 255
#define INSANE_FACE 256
#define DEMON_FACE  258

#define DISLIKE_ICON 56
#define MAX_DESCRIPTION_WIDTH 300

namespace gd3ds
{
extern const int difficultyStars[MAX_STARS + 1];

void showLevelLoadErrorMessage();

extern const UIScreenDefPair externalPopupDef;
}