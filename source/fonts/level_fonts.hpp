#pragma once
#include "text.hpp"

#define LEVEL_FONT_COUNT 13

extern const Charset *levelFontCharsets[LEVEL_FONT_COUNT];

extern const char *levelFontPaths[LEVEL_FONT_COUNT];
extern int loadedLevelFont;
extern C2D_SpriteSheet levelFontSheet;
extern const Charset *levelFont;
