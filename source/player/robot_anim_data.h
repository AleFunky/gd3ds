#pragma once

#define ROBOT_SPRITE_COUNT 8

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
    ROBOT_ANIM_COUNT      = 5,
} RobotAnimId;

typedef struct {
    int   texture_idx;
    float px, py;
    float rotation;
    float scale_x, scale_y;
    int   z_order;
} RobotSpritePart;

typedef struct {
    const RobotSpritePart *parts;
    int   part_count;
    float delay;
} RobotFrame;

typedef struct {
    const RobotFrame *frames;
    int frame_count;
} RobotAnimation;

extern const RobotAnimation robot_animations[ROBOT_ANIM_COUNT];
