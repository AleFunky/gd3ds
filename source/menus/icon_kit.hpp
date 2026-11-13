#pragma once
#include "icons.hpp"
#include "menus/core/ui_stack.hpp"

#define ICONS_PER_PAGE 21

extern const int gamemodeIconCount[ICON_GAMEMODE_COUNT + 1];

extern int selectedCube;
extern int selectedShip;
extern int selectedBall;
extern int selectedUfo;
extern int selectedWave;
extern int selectedRobot;
extern int selectedSpider;
extern int selectedTrail;

extern int selectedP1;
extern int selectedP2;
extern int selectedGlow;

extern bool playerGlowEnabled;
extern bool showGlow;

extern int *currentIcons[ICON_GAMEMODE_COUNT + 1];
extern int *currentColors[3];

extern const UIScreenDefPair iconKitDef;