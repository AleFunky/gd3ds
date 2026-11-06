#pragma once

#include "player_anim_data.h"

#define ROBOT_SPRITE_COUNT 8
#define ROBOT_BLEND_DURATION 0.05f
#define SPIDER_BLEND_DURATION 0.05f

typedef enum {
    ROBOT_PART1_COLOR = 0,
    ROBOT_PART1_GLOW  = 1,
    ROBOT_PART2_COLOR = 2,
    ROBOT_PART2_GLOW  = 3,
    ROBOT_PART3_COLOR = 4,
    ROBOT_PART3_GLOW  = 5,
    ROBOT_PART4_COLOR = 6,
    ROBOT_PART4_GLOW  = 7,
} RobotSpriteIdx;

typedef enum {
    ROBOT_ANIM_RUN        = 0,
    ROBOT_ANIM_JUMP_START = 1,
    ROBOT_ANIM_JUMP       = 2,
    ROBOT_ANIM_FALL_START = 3,
    ROBOT_ANIM_FALL       = 4,
    ROBOT_ANIM_IDLE       = 5,
    ROBOT_ANIM_COUNT      = 6,
} RobotAnimId;

extern const PlayerAnimation robot_animations[ROBOT_ANIM_COUNT];
