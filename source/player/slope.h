#pragma once

#include <stdbool.h>
#include "player.h"

enum SlopeOrientations {
    ORIENT_NORMAL_UP,
    ORIENT_NORMAL_DOWN,
    ORIENT_UD_DOWN,
    ORIENT_UD_UP
};

#define SLOPE_HEAD_TOLERANCE 2.f

float slope_angle(int obj, Player *player);
void slope_collide(int obj, Player *player);
int grav_slope_orient(int obj, Player *player);
void clear_slope_data(Player *player);
void clear_coyote_slope_data(Player *player);
void slope_calc(int obj, Player *player);
bool slope_touching(int obj, Player *player);
float slope_snap_angle(int obj, Player *player);

int get_player_touching_slopes(Player *player);