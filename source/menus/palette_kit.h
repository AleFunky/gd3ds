#pragma once
#include "icon_kit.h"

void palette_kit_init();
int palette_kit_loop();

extern const size_t NUM_COLORS;
extern const u32 colors[];

extern const int gd_to_gd3ds_color_table[];