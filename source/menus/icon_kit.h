#pragma once
#include "icons.h"
#include "menus/core/ui_stack.h"

#define ICONS_PER_PAGE 21

extern const int gamemode_icon_count[GAMEMODE_COUNT + 1];

extern int selected_cube;
extern int selected_ship;
extern int selected_ball;
extern int selected_ufo;
extern int selected_wave;
extern int selected_trail;

extern int selected_p1;
extern int selected_p2;
extern int selected_glow;

extern bool player_glow_enabled;
extern bool show_glow;

extern int *current_icons[GAMEMODE_COUNT + 1];
extern int *current_colors[3];

extern const UIScreenDefPair icon_kit_def;