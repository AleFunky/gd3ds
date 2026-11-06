#pragma once
#include "text.h"

#define LEVEL_FONT_COUNT 13

extern const Charset *level_font_charsets[LEVEL_FONT_COUNT];

extern const char *level_font_paths[LEVEL_FONT_COUNT];
extern int loaded_level_font;
extern C2D_SpriteSheet level_font_sheet;
extern const Charset *level_font;
