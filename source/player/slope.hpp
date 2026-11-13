#pragma once

#include <stdbool.h>
#include "player.hpp"

namespace gd3ds
{

enum SlopeOrientations {
    ORIENT_NORMAL_UP,
    ORIENT_NORMAL_DOWN,
    ORIENT_UD_DOWN,
    ORIENT_UD_UP
};

#define SLOPE_HEAD_TOLERANCE 2.f

float slopeAngle(int obj, Player *player);
void slopeCollide(int obj, Player *player);
int gravSlopeOrient(int obj, Player *player);
void clearSlopeData(Player *player);
void clearCoyoteSlopeData(Player *player);
void slopeCalc(int obj, Player *player);
bool slopeTouching(int obj, Player *player);
float slopeSnapAngle(int obj, Player *player);
bool isSpikeSlope(int obj);

int getPlayerTouchingSlopes(Player *player);

}