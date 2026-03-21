#pragma once
#include <citro2d.h>
#include "level_loading.h"

#define CAM_SPEED 5.19300155f

#define DT (1.f/60)

extern float delta;

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

enum GameState {
    STATE_MAIN_MENU,
    STATE_LEVEL_SELECT,
    STATE_ICON_KIT,
    STATE_GAME,
    STATE_EXIT
};

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* bot;

extern int game_state;
extern bool playing_menu_loop;

int output_log(const char *fmt, ...);

bool is_citra();