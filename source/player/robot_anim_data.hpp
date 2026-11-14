#pragma once

#include "player_anim_data.hpp"

#define ROBOT_SPRITE_COUNT 8
#define ROBOT_BLEND_DURATION 0.05f
#define SPIDER_BLEND_DURATION 0.05f

namespace gd3ds
{

enum class RobotSpriteIdx {
    RobotPart1Color = 0,
    RobotPart1Glow  = 1,
    RobotPart2Color = 2,
    RobotPart2Glow  = 3,
    RobotPart3Color = 4,
    RobotPart3Glow  = 5,
    RobotPart4Color = 6,
    RobotPart4Glow  = 7,
};

enum class RobotAnimId {
    RobotAnimRun        = 0,
    RobotAnimJumpStart = 1,
    RobotAnimJump       = 2,
    RobotAnimFallStart = 3,
    RobotAnimFall       = 4,
    RobotAnimIdle       = 5,
    RobotAnimCount      = 6,
};

extern const PlayerAnimation robotAnimations[static_cast<int>(RobotAnimId::RobotAnimCount)];

}