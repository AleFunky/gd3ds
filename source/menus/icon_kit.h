#pragma once
#include "icons.h"


#define ICONS_PER_PAGE 21

void icon_kit_loop();

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

extern int *current_icons[GAMEMODE_COUNT];
extern int *current_colors[3];

