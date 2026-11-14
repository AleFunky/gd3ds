#pragma once

#include "player_anim_data.hpp"

#define SPIDER_SPRITE_COUNT 13

namespace gd3ds
{

enum class SpiderAnimId {
    SpiderAnimRun = 0,
    SpiderAnimJump = 1,
    SpiderAnimFall = 2,
    SpiderAnimWalk = 3,
    SpiderAnimIdle = 4,
    SpiderAnimCount = 5,
};

extern const PlayerAnimation spiderAnimations[static_cast<int>(SpiderAnimId::SpiderAnimCount)];

}