#pragma once

#include "player.h"

typedef struct {
    float x, y;
} Vec2D;

enum JumpType {
    JUMP_YELLOW_PAD,
    JUMP_YELLOW_ORB,
    JUMP_BLUE_PAD,
    JUMP_BLUE_ORB,
    JUMP_PINK_PAD,
    JUMP_PINK_ORB,
    JUMP_TYPES_COUNT
};

#define GET_ACTIVATED(obj) ((objects.activated[obj] >> state.current_player) & 1)
#define SET_ACTIVATED(obj, val) (objects.activated[obj] |= (val << state.current_player))

#define YELLOW_PAD 35
#define PINK_PAD 140
#define BLUE_PAD 67

#define YELLOW_ORB 36
#define PINK_ORB 141
#define BLUE_ORB 84

#define CUBE_PORTAL 12
#define SHIP_PORTAL 13
#define BALL_PORTAL 47
#define UFO_PORTAL 111
#define WAVE_PORTAL 660

#define BLUE_GRAVITY_PORTAL 10
#define YELLOW_GRAVITY_PORTAL 11

#define ORANGE_MIRROR_PORTAL 45
#define BLUE_MIRROR_PORTAL 46

#define BIG_PORTAL 99
#define MINI_PORTAL 101

#define SECRET_COIN 142

#define SLOW_SPEED_PORTAL 200
#define NORMAL_SPEED_PORTAL 201
#define FAST_SPEED_PORTAL 202
#define FASTER_SPEED_PORTAL 203

#define DUAL_PORTAL 286
#define DIVORCE_PORTAL 287


#define MAX_COLLIDED_OBJECTS 256

void collide_with_objects(Player *player);
bool intersect(float x1, float y1, float w1, float h1, float angle1,
               float x2, float y2, float w2, float h2, float angle2);

bool intersect_rect_circle(float rx, float ry, float rw, float rh, float rangle,
                          float cx, float cy, float cradius);