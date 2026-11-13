#pragma once

#include "player.hpp"

namespace gd3ds
{

enum JumpType {
    JUMP_YELLOW_PAD,
    JUMP_YELLOW_ORB,
    JUMP_BLUE_PAD,
    JUMP_BLUE_ORB,
    JUMP_PINK_PAD,
    JUMP_PINK_ORB,
    JUMP_RED_ORB,
    JUMP_RED_PAD,
    JUMP_TYPES_COUNT
};

#define MINIMUM_OBJECT_SPEED 315

#define GET_ACTIVATED(obj) \
    ((objects.activated[(obj)] >> state.currentPlayer) & 1)

#define SET_ACTIVATED(obj, val) \
    (objects.activated[(obj)] = \
        (objects.activated[(obj)] & ~(1 << state.currentPlayer)) | \
        (((val) & 1) << state.currentPlayer))

#define GET_COLLIDED(obj) \
    ((objects.collided[(obj)] >> state.currentPlayer) & 1)

#define SET_COLLIDED(obj, val) \
    (objects.collided[(obj)] = \
        (objects.collided[(obj)] & ~(1 << state.currentPlayer)) | \
        (((val) & 1) << state.currentPlayer))

#define GET_HITBOX_COUNTER(obj) \
    ((objects.hitboxCounter[(obj)] >> (state.currentPlayer * 8)) & 0xff)

#define SET_HITBOX_COUNTER(obj, val) \
    (objects.hitboxCounter[(obj)] = \
        (objects.hitboxCounter[(obj)] & ~(0xff << (state.currentPlayer * 8))) | \
        (((val) & 0xff) << (state.currentPlayer * 8)))
        
#define YELLOW_PAD 35
#define PINK_PAD 140
#define BLUE_PAD 67
#define RED_PAD 1332
#define S_MODIFIERBLOCK 1829
#define D_MODIFIERBLOCK 1755
#define J_MODIFIERBLOCK 1813
#define H_MODIFIERBLOCK 1859

#define YELLOW_ORB 36
#define PINK_ORB 141
#define BLUE_ORB 84
#define GREEN_ORB 1022
#define BLACK_ORB 1330
#define RED_ORB 1333
#define TOGGLE_ORB 1594
#define DASH_ORB 1704
#define GRAVITY_DASH_ORB 1751

#define CUBE_PORTAL 12
#define SHIP_PORTAL 13
#define BALL_PORTAL 47
#define UFO_PORTAL 111
#define WAVE_PORTAL 660
#define ROBOT_PORTAL 745
#define SPIDER_PORTAL 1331

#define BLUE_TP_PORTAL 747
#define ORANGE_TP_PORTAL 749

#define TEXT_OBJECT 914

#define BLUE_GRAVITY_PORTAL 10
#define YELLOW_GRAVITY_PORTAL 11

#define ORANGE_MIRROR_PORTAL 45
#define BLUE_MIRROR_PORTAL 46

#define BIG_PORTAL 99
#define MINI_PORTAL 101

#define SECRET_COIN 142
#define KEY_OBJ 1275
#define HEART_COLLECTABLE 1587
#define POTION_COLLECTABLE 1589
#define SKULL_COLLECTABLE 1598
#define SMALL_COIN 1614

static inline bool isCollectableObject(int id) {
    return id == KEY_OBJ || id == HEART_COLLECTABLE || id == POTION_COLLECTABLE || id == SKULL_COLLECTABLE;
}

#define SLOW_SPEED_PORTAL 200
#define NORMAL_SPEED_PORTAL 201
#define FAST_SPEED_PORTAL 202
#define FASTER_SPEED_PORTAL 203
#define FASTEST_SPEED_PORTAL 1334

#define DUAL_PORTAL 286
#define DIVORCE_PORTAL 287

#define BREAKABLE_BLOCK 143

#define ENABLE_TRAIL 32
#define DISABLE_TRAIL 33

#define MAX_COLLIDED_OBJECTS 512

extern int potentialSlopesBuffer[2][MAX_COLLIDED_OBJECTS];
extern int potentialSlopes[2];

extern const float gamemodeHeights[GAMEMODE_COUNT];

void setupDual();
void setDualBounds();
void doBallReflection();

void collideWithObjects(Player *player);
bool intersect(float x1, float y1, float w1, float h1, float angle1,
               float x2, float y2, float w2, float h2, float angle2);
bool intersectExclusive(float x1, float y1, float w1, float h1, float angle1,
               float x2, float y2, float w2, float h2, float angle2);

bool intersectRectCircle(float rx, float ry, float rw, float rh, float rangle,
                          float cx, float cy, float cradius);

bool circleRectCollision(float cx, float cy, float radius,
                           float x1, float y1, float x2, float y2);

}