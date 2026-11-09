#pragma once

#include "player_anim_data.hpp"

#define SPIDER_SPRITE_COUNT 13

typedef enum {
    SPIDER_ANIM_RUN = 0,
    SPIDER_ANIM_JUMP = 1,
    SPIDER_ANIM_FALL = 2,
    SPIDER_ANIM_WALK = 3,
    SPIDER_ANIM_IDLE = 4,
    SPIDER_ANIM_COUNT = 5,
} SpiderAnimId;

extern const PlayerAnimation spider_animations[SPIDER_ANIM_COUNT];
