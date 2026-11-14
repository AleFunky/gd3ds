#include "player.hpp"
#include "level_loading.hpp"
#include "particles/circles.hpp"
#include "player/trail.hpp"
#include "practice.hpp"
#include "profiling.hpp"
#include "state.hpp"
#include "icons.hpp"
#include "graphics.hpp"

#include "particles/particles.hpp"

#include "slope.hpp"

#include "menus/icon_kit.hpp"
#include "collision.hpp"
#include "robot_anim_data.hpp"
#include "spider_anim_data.hpp"
#include "math_helpers.hpp"

#include "main.hpp"

#include "mp3_player.hpp"
#include "menus/settings_hub/settings.hpp"
#include "utils/gfx.hpp"

#include "easing.hpp"
#include <stdint.h>

#define SPIDER_DASH_SCALE_DURATION 0.45f
#define SPIDER_DASH_SCALE_START 0.4f
#define SPIDER_DASH_ELASTIC_PERIOD 0.35f

namespace gd3ds
{

inline float gravFloor(Player *player) { return player->upsideDown ? -state.ceilingY : state.groundY; }

void animPlayerToWall(Player *player);
float expectedSlopeY(int obj, Player *player);

KeyInput currInput;
KeyInput currOldInput;

MotionTrail *trail;
MotionTrail trailP1;
MotionTrail trailP2;

MotionTrail *waveTrail;
MotionTrail waveTrailP1;
MotionTrail waveTrailP2;

ParticleSystem dragParticles[2];
ParticleSystem dragParticles2[2];
ParticleSystem shipFireParticles[2];
ParticleSystem shipSecondaryParticles[2];
ParticleSystem secondaryParticles[2];
ParticleSystem burstParticles[2];
ParticleSystem robotFireParticles[2];
ParticleSystem landParticles[2];
ParticleSystem explosionParticles[2];
ParticleSystem glitterParticles;
ParticleSystem slowSpeedParticles;
ParticleSystem normalSpeedParticles;
ParticleSystem fastSpeedParticles;
ParticleSystem fasterSpeedParticles;
ParticleSystem fastestSpeedParticles;
ParticleSystem coinPickupParticles;

PlayerAction playerActions[2][MAX_ACTIONS];
int numActions[2] = {0};

const float velocityThresholds[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
	101.541492,
	103.485494592,
	103.377492,
	103.809492,
	103.809492
};

extern const float playerSpeeds[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566,
	576.000192
};

const float cubeJumpHeights[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
    573.481728,
    603.7217172,
    616.681728,
    606.421728,
    606.421728,
};

const float cubeAccelerations[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
    -2747.52,
    -2794.1082,
    -2786.4,
    -2799.36,
    -2799.36,
};

const float robotAccelerations[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
    -2467.4583,
    -2514.6975,
    -2512.0731,
    -2522.5707,
    -2522.5707,
};

const float playerSpeedMults[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
	0.7f,
	0.9f,
	1.1f,
	1.3f,
	1.6f
};

const float cubeRotationSpeed[2] = {
    415.3848f,
    540.f
};

bool playerGamemodeIsFlying(Player *player) {
    return player->gamemode == static_cast<int>(IconGamemode::GamemodeShip) || player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo) || player->gamemode == static_cast<int>(IconGamemode::GamemodeWave);
}

void setAnimation(Player *player, int animID) {
    if (player->animID != animID) {
        player->blendFromAnimID = player->animID;
        player->blendFromFrame = player->animFrame;
        player->blendFromTimer = player->animTimer;

        player->animID = animID;
        player->animFrame = 0;
        player->animTimer = 0;
        player->blendTimer = 0;
    }
}

float getBlendDuration(Player *player) {
    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeRobot)) {
        return ROBOT_BLEND_DURATION;
    } else if (player->gamemode == static_cast<int>(IconGamemode::GamemodeSpider)) {
        return SPIDER_BLEND_DURATION;
    }
    return 0.1f;
}

void setBlendTimer(Player *player, float timer) {
    player->blendTimer = timer;
}

void playerNonFlyingLanding(Player *player) {
    if (!playerGamemodeIsFlying(player)) {
        player->landedFromJump |= player->jumped;
        player->jumped = false;
    }
}

float playerGetVel(Player *player, float vel) {
    return vel * (player->upsideDown ? -1 : 1);
}

void setPVelocity(Player *player, float vel, bool override) {
    player->velocityOverride = override;
    player->velY = vel * ((player->mini) ? 0.8 : 1);
}

void updateRotationDirection(Player *player) {
    player->rotationDirection = (player->upsideDown ? -1 : 1);
}

void stopDashing(Player *player) {
    if (!player->dashing) return;
    addDashExit(player);
    player->dashing = false;
    player->dashFireTimer = 0.f;
    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeBall)) {
        player->ballRotationSpeed = 1.f;
        player->ballSlopeRotationMult = 1.f;
    }
}

void playerFlash(Player *player, float duration, float delay) {
    player->flashActive = true;
    player->flashTimer = 0.f;
    player->flashDuration = duration;
    player->flashDelay = delay;
}

float convertToClosestRotation(float rotation, float angle) {
    int rot = (int)rotation % 360;

    if (rot < 0)
        rot += 360;

    float x = ((float) rot - angle) / 90.0f;
    float k = (x >= 0.f) ? ceilf(x - 0.5f) : floorf(x + 0.5f);

    float snapped = fmodf(k * 90.0f + angle, 360.0f);

    if (snapped < 0)
        snapped += 360.0f;

    return snapped;
}

void cubeGamemode(Player *player) {
    int mult = player->rotationDirection;
    
    trail->positionR = (Vec2D){player->x, player->y};  
    trail->startingPositionInitialized = true;

    player->gravity = cubeAccelerations[state.speed];

    player->minVel = -810;
    player->maxVel = 1080;

    // Limit player speed (there is a upwards cap interestingly)
    if (player->velY < player->minVel) player->velY = player->minVel;
    if (player->velY > player->maxVel) player->velY = player->maxVel;

    // If player fell above the level, die
    if (player->y > 2794.f) killPlayer(DeathReason::DeathFellOffLevel);

    if (player->snapRotation) {
        player->cubeTargetRotation = player->rotation;
    }

    // Do cube rotation
    if (player->slopeData.slopeID < 0 && !player->onGround && !player->dashing) {
        if (player->inverseRotation) {
            player->rotation -= (cubeRotationSpeed[player->mini] / 2) * STEPS_DT * mult;
        } else {
            player->rotation += cubeRotationSpeed[player->mini] * STEPS_DT * mult;
        }
    }

    bool jump = false;

    dragParticles[state.currentPlayer].emitterX = getLeft(player);
    dragParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -2 : 2);
    dragParticles[state.currentPlayer].emitting = player->timeSinceGround < DRAG_PARTICLES_FLOOR_DURATION;

    dragParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    dragParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    dragParticles[state.currentPlayer].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    if (currInput.holdJump && !player->dashing) {
        jump = true;
    } else if (player->onGround) {
        player->consecutiveJumps = 0;
    }

    // J block
    if (player->holdingJumpCancelled && player->bufferingState == static_cast<int>(BufferingState::BufferEnd)) {
        jump = false;
    }

    if (player->onGround) {
        motionTrailStopStroke(trail);
        updateRotationDirection(player);
    }

    bool shouldCoyote = (state.dual || player->upsideDown) && currInput.holdJump && player->coyoteFrames < 10;

    SlopeData slopeData = player->slopeData;

    // If not currently on slope, look at the last frame
    if (player->slopeData.slopeID < 0 && player->slopeSlideCoyoteTime) {
        slopeData = player->coyoteSlope;
    }

    if (((slopeData.slopeID >= 0 && gravSlopeOrient(slopeData.slopeID, player) < static_cast<int>(SlopeOrientations::OrientUdDown)) || player->onGround || shouldCoyote) && jump) {
        // If on slope, the player jumps depending on time on slope
        if (slopeData.slopeID >= 0) {
            // Slope jump
            int orient = gravSlopeOrient(slopeData.slopeID, player);
            if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp)) {
                float time = clampf(10 * (player->timeElapsed - slopeData.elapsed), 0.4f, 1.0f);
                float vel = 0.9f * MIN(1.12f / slopeAngle(slopeData.slopeID, player), 1.54f) * (objects.height[slopeData.slopeID] * playerSpeeds[state.speed] / objects.width[slopeData.slopeID]);
                setPVelocity(player, 0.25f * time * vel + cubeJumpHeights[state.speed], false);
                player->slopeSlideCoyoteTime = 0;
            } else {
                setPVelocity(player, cubeJumpHeights[state.speed], currOldInput.holdJump);
            }
        } else {
            // Normal jump
            setPVelocity(player, cubeJumpHeights[state.speed], currOldInput.holdJump);
        }
        player->inverseRotation = false;
        player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
    
        player->onGround = false;
        player->consecutiveJumps++;
        player->jumped = true;
        
        state.currentData.jumps++;
    
        if (!(currInput.pressedJump)) {
            // This prevents drag particles on succesive jumps
            player->timeSinceGround = DRAG_PARTICLES_FLOOR_DURATION;
        }
    }

    // If not in a slope, snap to nearest 90 degree angle
    if (player->onGround && player->slopeData.slopeID < 0) {
        player->cubeTargetRotation = convertToClosestRotation(player->rotation, 0);
    }

    if (player->slopeData.slopeID >= 0) {
        float base = RadToDeg(slopeSnapAngle(player->slopeData.slopeID, player));
        player->cubeTargetRotation = convertToClosestRotation(player->rotation, base);
    }
    if (player->slopeSlideCoyoteTime) {
        float base = RadToDeg(slopeSnapAngle(player->coyoteSlope.slopeID, player));
        player->cubeTargetRotation = convertToClosestRotation(player->rotation, base);
    }
}

void rotateFly(Player *player, float mult) {
    float diffX = (player->x - state.oldPlayer.x);
    float diffY = (player->y - state.oldPlayer.y);
    float angleRad = atan2f(-diffY, diffX);
 
    if (player->snapRotation) {
        player->rotation = RadToDeg(angleRad);
    } else if (STEPS_DT * 72 <= diffX * diffX + diffY * diffY) {
        // This is how gd does rotation
        if (player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) {
            if (player->slopeData.slopeID >= 0) {
                angleRad = slopeSnapAngle(player->slopeData.slopeID, player);
            } else if (player->onGround) {
                angleRad = 0;
            } else if (!player->upsideDown) {
                angleRad = MAX(angleRad * -0.4f, -0.1f);
            } else {
                angleRad = MIN(angleRad * -0.4f, 0.1f);
            }
        }

        player->rotation = RadToDeg(slerpFancy(DegToRad(player->rotation), angleRad, (STEPS_DT * 60) * mult));
	}
}

// Ball rotation multiplier
float getBallRotationSpeed(Player *player) {
    float speed = (player->mini ? 0.16f : 0.2f);
    switch (state.speed) {
        case 0:
            speed *= 1.2405638f;
            break;
        case 2:
            speed *= 0.80424345f;
            break;
        case 3:
            speed *= 0.6657693f;
            break;
        case 4:
            speed *= 0.5409375f;
            break;
        default:
            break;
    }

    return 120 / speed;
}

void shipGamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;

    float rad = C3D_AngleFromDegrees(player->rotation);
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    int flipYMult = (player->upsideDown ? -1 : 1);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = -sinR;
    float m11 = cosR;

    const float localX = -14;
    const float localY = -8 * flipYMult;

    float rotX = localX * m00 + localY * m01;
    float rotY = localX * m10 + localY * m11;

    float x = player->x + rotX * scale;
    float y = player->y + rotY * scale;
    
    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;
    float calcX = player->x - state.cameraX;
    float calcY = SCREEN_HEIGHT - (fabsf(gravBottom(player)) - state.cameraY);

    shipFireParticles[state.currentPlayer].emitterX = x;
    shipFireParticles[state.currentPlayer].emitterY = y;
    shipFireParticles[state.currentPlayer].emitting = currInput.holdJump;

    shipFireParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    shipFireParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    shipFireParticles[state.currentPlayer].cfg.sourcePositionVariancey = (player->mini ? 4.f : 3.f);

    shipSecondaryParticles[state.currentPlayer].emitterX = x;
    shipSecondaryParticles[state.currentPlayer].emitterY = y;
    shipSecondaryParticles[state.currentPlayer].emitting = true;

    shipSecondaryParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    shipSecondaryParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    shipSecondaryParticles[state.currentPlayer].cfg.sourcePositionVariancey = (player->mini ? 4.5f : 2.f);

    dragParticles2[state.currentPlayer].emitterX = calcX;
    dragParticles2[state.currentPlayer].emitterY = calcY;
    dragParticles2[state.currentPlayer].emitting = player->onGround;

    dragParticles2[state.currentPlayer].gravityFlipped = !player->upsideDown;
    dragParticles2[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    player->minVel = player->mini ? -406.566f : -345.6f;
    player->maxVel = player->mini ? 508.248f : 432.0f;

    if (player->velocityUncapped) {
        if ((player->velY >= 0.f && player->velY < player->maxVel) ||
            (player->velY <= 0.f && player->velY > player->minVel)) {
            player->velocityUncapped = false;
        }
    }

    if (player->velocityUncapped) {
        float threshold = state.dual ? -velocityThresholds[state.speed] : grav(player, velocityThresholds[state.speed]);
        bool legacyFalling = player->velY <= threshold;
        float shipMod = (player->velY < 0.f) ? -1.f : 0.8f;
        if (!currInput.holdJump && !legacyFalling) shipMod = 1.2f;
        float boost = (currInput.holdJump && legacyFalling) ? 0.5f : 0.4f;
        player->gravity = -shipMod * boost * (player->mini ? 3287.186216f : 2794.108284f);
        if (currInput.holdJump) player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
    } else if (state.dual) {
        // Make both dual players symmetric by using inverted ship gravity
        if (currInput.holdJump) {
            player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
            if (player->velY <= -velocityThresholds[state.speed])
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->velY >= -velocityThresholds[state.speed])
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (currInput.holdJump) {
            player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
            if (player->velY <= grav(player, velocityThresholds[state.speed]))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->velY >= grav(player, velocityThresholds[state.speed]))
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    }

    if (!player->velocityUncapped) {
        if (player->gravity < 0 && player->velY < player->minVel) {
            player->velY = player->minVel;
        } else if (player->gravity > 0 && player->velY > player->maxVel) {
            player->velY = player->maxVel;
        }
    }
}


static float ballJumpHeights[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
    -172.044007,
    -181.11601,
    -185.00401,
    -181.92601,
    -181.92601
};

void ballGamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};  
    trail->startingPositionInitialized = true;

    int mult = (player->upsideDown ? -1 : 1);

    // If player left slope, set gravity
    if (!state.oldPlayer.velocityOverride || state.oldPlayer.slopeData.slopeID >= 0)
        player->gravity = -1676.46672f;  
    
    if (player->onGround || player->onCeiling) {
        motionTrailStopStroke(trail);
        player->ballRotationSpeed = 1.f;
        player->ballSlopeRotationMult = 1.f;

        if (player->slopeData.slopeID >= 0) {
            float slopeRot = 1.f / cosf(slopeAngle(player->slopeData.slopeID, player));
            if (slopeRot > 2.f) slopeRot = (slopeRot - 1.f) * 0.7f + 1.f;
            player->ballSlopeRotationMult = slopeRot;
        }
    }
    
    dragParticles[state.currentPlayer].emitterX = player->x;
    dragParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -2 : 2);
    dragParticles[state.currentPlayer].emitting = player->onGround || player->onCeiling;

    dragParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    dragParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    dragParticles[state.currentPlayer].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    bool shouldCoyote = (state.dual || player->upsideDown) && player->coyoteFrames < 16;
    // If on ground (block or slope) and its buffering, do a jump
    if ((player->slopeData.slopeID >= 0 || player->onGround || player->onCeiling || shouldCoyote) && player->bufferingState == static_cast<int>(BufferingState::BufferReady) && !player->dashing) {
        player->upsideDown ^= 1;

        setPVelocity(player, ballJumpHeights[state.speed], state.oldPlayer.bufferingState == static_cast<int>(BufferingState::BufferReady));

        if (player->slopeData.slopeID >= 0 && gravSlopeOrient(player->slopeData.slopeID, player) == static_cast<int>(SlopeOrientations::OrientNormalUp))
            player->velY -= player->gravity * STEPS_DT;

        player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
        
        state.currentData.jumps++;
        
        player->ballRotationSpeed = -BALL_SLOW_ROTATION;
        player->ballSlopeRotationMult = 1.f;

        player->onGround = false;
        player->jumped = true;
    }
    
    if (!player->dashing) player->rotation += player->ballRotationSpeed * getBallRotationSpeed(player) * player->ballSlopeRotationMult * mult * STEPS_DT;

    player->minVel = -810;
    player->maxVel = 810;

    if (player->velY < player->minVel) {
        player->velY = player->minVel;
    } else if (player->velY > player->maxVel) {
        player->velY = player->maxVel;
    }
}

void ufoGamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;

    float rad = C3D_AngleFromDegrees(player->rotation);
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    int flipYMult = (player->upsideDown ? -1 : 1);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = -sinR;
    float m11 = cosR;

    const float localX = 0;
    const float localY = -8 * flipYMult;

    float rotX = localX * m00 + localY * m01;
    float rotY = localX * m10 + localY * m11;

    float x = player->x + rotX * scale;
    float y = player->y + rotY * scale;
    
    float calcX = player->x - state.cameraX;
    float calcY = SCREEN_HEIGHT - (fabsf(gravBottom(player)) - state.cameraY);

    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;

    secondaryParticles[state.currentPlayer].emitterX = player->x;
    secondaryParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -4 : 4);
    secondaryParticles[state.currentPlayer].emitting = true;

    secondaryParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    dragParticles2[state.currentPlayer].emitterX = calcX;
    dragParticles2[state.currentPlayer].emitterY = calcY;
    dragParticles2[state.currentPlayer].emitting = player->onGround;

    dragParticles2[state.currentPlayer].gravityFlipped = !player->upsideDown;
    dragParticles2[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    bool bufferingCheck = ((state.oldPlayer.gamemode == static_cast<int>(IconGamemode::GamemodePlayer) || state.oldPlayer.gamemode == static_cast<int>(IconGamemode::GamemodeShip) || state.oldPlayer.gamemode == static_cast<int>(IconGamemode::GamemodeWave) || player->bufferUfo) && (currInput.holdJump));
    // If buffering, jump
    if (player->bufferingState == static_cast<int>(BufferingState::BufferReady) && (currInput.pressedJump || bufferingCheck) && !player->dashing) {
        player->velY = fmaxf(player->velY, player->mini ? 358.992 : 371.034);
        player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
        player->velocityOverride = true;
        player->burstParticleTimer = BURST_PARTICLES_DURATION;
        player->velocityUncapped = false;
    } else {
        // Same as ship
        if (!state.dual) {
            if (player->velY > grav(player, velocityThresholds[state.speed])) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        } else {   
            if (player->velY > -velocityThresholds[state.speed]) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        }
    }

    if (player->burstParticleTimer > 0) {
        player->burstParticleTimer -= STEPS_DT;
    }

    burstParticles[state.currentPlayer].emitterX = player->x;
    burstParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -4 : 4);
    burstParticles[state.currentPlayer].emitting = player->burstParticleTimer > 0;

    burstParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    burstParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    player->minVel = player->mini ? -406.566f : -345.6f;
    player->maxVel = player->mini ? 508.248f : 432.0f;

    if (!player->velocityUncapped) {
        if (player->velY < player->minVel) {
            player->velY = player->minVel;
        } else if (player->velY > player->maxVel) {
            player->velY = player->maxVel;
        }
    }
}

void waveGamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;
    float rad = C3D_AngleFromDegrees(player->rotation);
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = -sinR;
    float m11 = cosR;

    const float localX = -8;
    const float localY = 0;

    float rotX = localX * m00 + localY * m01;
    float rotY = localX * m10 + localY * m11;

    float x = player->x + rotX * scale;
    float y = player->y + rotY * scale;
    
    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;
 
    if (gameState == static_cast<int>(GameState::StateGame) && player->cutsceneTimer == 0 && !state.mirroring) waveTrail->opacity = 1.f;

    if (player->bufferingState == static_cast<int>(BufferingState::BufferReady)) player->bufferingState = static_cast<int>(BufferingState::BufferEnd);

    bool input = (currInput.holdJump);
    player->gravity = 0;

    player->velY = (input * 2 - 1) * playerSpeeds[state.speed] * (player->mini ? 2 : 1);
}

static float robotFireInScale(float timer) {
    if (timer < 0.05f) return 0.1f + (1.44f - 0.1f) * (timer / 0.05f);
    if (timer < 0.1f) return 1.44f + (0.72f - 1.44f) * ((timer - 0.05f) / 0.05f);
    if (timer < 0.15f) return 0.72f + (1.8f - 0.72f) * ((timer - 0.1f) / 0.05f);
    if (timer < 0.2f) return 1.8f + (0.9f - 1.8f) * ((timer - 0.15f) / 0.05f);
    if (timer < 0.25f) return 0.9f + (1.8f - 0.9f) * ((timer - 0.2f) / 0.05f);
    return 1.8f;
}

static void updateRobotFire(Player *player) {
    bool show = player->gamemode == static_cast<int>(IconGamemode::GamemodeRobot)
             && !player->onGround
             && player->gravity == 0.f
             && player->robotAirTime > 0.f
             && player->robotAirTime < ROBOT_FIRE_MAX_AIR_TIME;

    if (show) {
        if (!player->robotFireVisible || player->robotFireOut) {
            player->robotFireVisible = true;
            player->robotFireOut = false;
            player->robotFireTimer = 0.f;
            player->robotFireScale = 0.1f;
        } else {
            player->robotFireTimer += STEPS_DT;
            player->robotFireScale = robotFireInScale(player->robotFireTimer);
        }
    } else if (player->robotFireVisible) {
        if (!player->robotFireOut) {
            player->robotFireOut = true;
            player->robotFireTimer = 0.f;
            player->robotFireExitScale = player->robotFireScale;
        } else {
            player->robotFireTimer += STEPS_DT;
            float progress = CLAMP(player->robotFireTimer / ROBOT_FIRE_OUT_TIME, 0.f, 1.f);
            player->robotFireScale = player->robotFireExitScale + (ROBOT_FIRE_MIN_SCALE - player->robotFireExitScale) * progress;
            if (player->robotFireTimer >= ROBOT_FIRE_OUT_TIME) player->robotFireVisible = false;
        }
    }
}

void robotGamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};
    trail->startingPositionInitialized = true;

    player->minVel = -810;
    player->maxVel = 1080;

    if (player->velY < player->minVel) player->velY = player->minVel;
    if (player->velY > player->maxVel) player->velY = player->maxVel;

    if (player->y > 2794.f) killPlayer(DeathReason::DeathFellOffLevel);

    if (player->onGround) {
        if (!player->dashing) setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimRun));
        motionTrailStopStroke(trail);
        if (player->slopeData.slopeID < 0 && !player->dashing) player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }
    
    dragParticles[state.currentPlayer].emitterX = player->x;
    dragParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -2 : 2);
    dragParticles[state.currentPlayer].emitting = player->onGround || player->onCeiling;

    dragParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    dragParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    dragParticles[state.currentPlayer].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    SlopeData slopeData = player->slopeData;

    bool shouldCoyote = (state.dual || player->upsideDown) && currInput.holdJump && player->coyoteFrames < 11;

    if (player->slopeData.slopeID < 0 && player->slopeSlideCoyoteTime) {
        slopeData = player->coyoteSlope;
    }

    if (player->snapRotation) {
        player->rotation = 0;
    }

    if ((slopeData.slopeID >= 0 || player->onGround || shouldCoyote) && (currInput.holdJump && player->bufferingState == static_cast<int>(BufferingState::BufferReady)) && !player->dashing) {
        setPVelocity(player, cubeJumpHeights[state.speed] / 2, false);
        player->inverseRotation = false;
        player->onGround = false;
        setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimJumpStart));
        player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
        player->robotAirTime = 0.f;
        player->gravity = 0;
        player->jumped = true;
    }

    if (player->robotAirTime >= 1.5f || (!currInput.holdJump)) {
        player->gravity = robotAccelerations[state.speed];
        if (player->animID == static_cast<int>(RobotAnimId::RobotAnimJump)) {
            setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimFallStart));
        }
    } else {
        player->robotAirTime += 5.4f * STEPS_DT;
    }

    float scale = (player->mini) ? 0.6f : 1.f;
    robotFireParticles[state.currentPlayer].emitterX = player->x - 4.f * scale;
    robotFireParticles[state.currentPlayer].emitterY = player->y - grav(player, 10.f) * scale;
    robotFireParticles[state.currentPlayer].emitting = player->gravity == 0;

    robotFireParticles[state.currentPlayer].scale = scale;

    if (!player->onGround && player->velY < 0
        && player->animID != static_cast<int>(RobotAnimId::RobotAnimFallStart)
        && player->animID != static_cast<int>(RobotAnimId::RobotAnimFall)) {
        setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimFallStart));
    }

    const PlayerAnimation *anim = &robotAnimations[player->animID];
    float animSpeed = (state.speed == static_cast<int>(PlayerSpeeds::SpeedSlow)) ? 0.9f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFast)) ? 1.15f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFaster)) ? 1.3f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFastest)) ? 1.5f : 1.0f;
    if (player->mini) animSpeed /= 0.85f;
    
    animSpeed *= 35.0f / 24.0f;
    player->animTimer += STEPS_DT * animSpeed;

    if (player->animTimer >= anim->frames[player->animFrame].delay) {
        player->animTimer -= anim->frames[player->animFrame].delay;
        int next = player->animFrame + 1;
        if (next >= anim->frameCount) {
            if (player->animID == static_cast<int>(RobotAnimId::RobotAnimJumpStart)) {
                setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimJump));
            } else if (player->animID == static_cast<int>(RobotAnimId::RobotAnimFallStart)) {
                setAnimation(player, static_cast<int>(RobotAnimId::RobotAnimFall));
            }
            next = 0;
        }
        player->animFrame = next;
    }
}

#define SPIDER_RAY_MAX 256

static int spiderSolidBuffer[SPIDER_RAY_MAX];
static int spiderHazardBuffer[SPIDER_RAY_MAX];

static bool spiderOverlap(float playerX, float playerY, float playerWidth, float playerHeight, float objectX, float objectY, float objectWidth, float objectHeight) {
    return fabsf(playerX - objectX) < (playerWidth + objectWidth) * 0.5f && fabsf(playerY - objectY) < (playerHeight + objectHeight) * 0.5f;
}

static bool spiderRectHits(int obj, float minX, float minY, float maxX, float maxY) {
    const ObjectHitbox *hitbox = gameObjects[objects.id[obj]].hitbox;

    float objectX = objects.x[obj] + hitbox->x * objects.scaleX[obj];
    float objectY = objects.y[obj] + hitbox->y * objects.scaleY[obj];
    float halfWidth = objects.width[obj] * 0.5f;
    float halfHeight = objects.height[obj] * 0.5f;

    return minX < objectX + halfWidth && maxX > objectX - halfWidth && minY < objectY + halfHeight && maxY > objectY - halfHeight;
}

static int spiderCollect(float minX, float minY, float maxX, float maxY, int *out, int capacity, bool isHazard) {
    int count = 0;

    int solidMinX = (int)floorf(minX / SECTION_SIZE);
    int solidMaxX = (int)floorf(maxX / SECTION_SIZE);
    int solidMinY = (int)floorf(minY / SECTION_SIZE);
    int solidMaxY = (int)floorf(maxY / SECTION_SIZE);

    for (int sy = solidMinY; sy <= solidMaxY; sy++) {
        for (int sx = solidMinX; sx <= solidMaxX; sx++) {
            Section *sec = getSection(sx, sy);

            for (int i = 0; i < sec->objectCount; i++) {
                int obj = sec->objects[i];
                int id = objects.id[obj];

                if (!isValidObject(id)) continue;

                const ObjectHitbox *hitbox = gameObjects[id].hitbox;

                if (!hitbox) continue;
                if (objects.flags[obj] & static_cast<int>(ObjectFlags::FlagToggled)) continue;
                if (isHazard) {
                    if (hitbox->collisionType != static_cast<int>(HitboxType::HitboxHazard)) continue;
                } else if (hitbox->collisionType != static_cast<int>(HitboxType::HitboxSolid)) continue;

                if (!spiderRectHits(obj, minX, minY, maxX, maxY)) continue;

                if (count < capacity) out[count++] = obj;
            }
        }
    }

    return count;
}

static void spiderSort(int *list, int count, bool upsideDown) {
    for (int i = 1; i < count; i++) {
        int obj = list[i];
        float key = upsideDown ? -objGetTop(obj) : objGetBottom(obj);
        int j = i - 1;

        while (j >= 0) {
            float prev = upsideDown ? -objGetTop(list[j]) : objGetBottom(list[j]);
            if (prev <= key) break;
            list[j + 1] = list[j];
            j--;
        }

        list[j + 1] = obj;
    }
}

static float spiderSlopeY(Player *player, int obj) {
    float ydist = (player->upsideDown ^ (gravSlopeOrient(obj, player) >= static_cast<int>(SlopeOrientations::OrientUdDown)) ? -1.f : 1.f)
                * (player->height / 2) / cosf(slopeAngle(obj, player));
    return expectedSlopeY(obj, player) - ydist - grav(player, player->height / 2);
}

void spiderTestJump(Player *player, bool isRetry) {
    if (state.dead) return;

    float playerWidth = player->width;
    float playerHeight = player->height;
    float playerLeft = player->x - playerWidth * 0.5f;
    float playerBottom = player->y - playerHeight * 0.5f;
    float playerTop = player->y + playerHeight * 0.5f;

    float min_teleport = MAX(state.groundY, player->y - 3000.0f);
    float max_teleport = MIN(state.ceilingY, player->y + 3000.0f);

    float tolerance = 2.0f;

    float solidMinY = !player->upsideDown ? (playerTop - tolerance) : min_teleport;
    float solidMaxY = !player->upsideDown ? max_teleport : (playerBottom + tolerance);

    bool goingLeft = state.mirrorMult < 0;

    float solidMinX = goingLeft ? (playerLeft - playerWidth * 0.5f - 1.0f) : (playerLeft + playerWidth * 0.5f);
    float solidMaxX = solidMinX + (playerWidth * 0.5f + 1.0f);

    float hazardMinX = playerLeft;
    float hazardMaxX = playerLeft + playerWidth;
    float extraMinX = hazardMinX;
    float extraMaxX = hazardMaxX + 4.0f;

    float hazardWidth = (player->mini ? 0.6f : 1.0f) * 8.0f;
    float hazardNarrowMinX = hazardMinX + (solidMaxX - solidMinX - hazardWidth) * 0.5f;
    float hazardNarrowMaxX = hazardNarrowMinX + hazardWidth;

    int solidCount = spiderCollect(solidMinX, solidMinY, solidMaxX, solidMaxY, spiderSolidBuffer, SPIDER_RAY_MAX, false);
    int hazardCount = spiderCollect(hazardNarrowMinX, solidMinY, hazardNarrowMaxX, solidMaxY, spiderHazardBuffer, SPIDER_RAY_MAX, true);

    bool onlyDamaging = (solidCount == 0 && hazardCount != 0);

    if (onlyDamaging || isRetry) {
        isRetry = true;
        solidCount = spiderCollect(extraMinX, solidMinY, extraMaxX, solidMaxY, spiderSolidBuffer, SPIDER_RAY_MAX, false);
    }

    spiderSort(spiderSolidBuffer, solidCount, player->upsideDown);
    spiderSort(spiderHazardBuffer, hazardCount, player->upsideDown);

    float prevLandY = 0.0f;
    float landY = 0.0f;

    if (onlyDamaging) {
        int nearest = spiderHazardBuffer[0];
        landY = objects.y[nearest];
    } else {
        int i = 0;

        for (i = 0; i < solidCount; i++) {
            int obj = spiderSolidBuffer[i];
            bool checkingSlope = false;

            if (!player->upsideDown) {
                landY = objGetBottom(obj) - playerHeight * 0.5f;
            } else {
                landY = objGetTop(obj) + playerHeight * 0.5f;
            }

            if (gameObjects[objects.id[obj]].hitbox->type == static_cast<int>(CollisionShape::CollisionSlope)) {
                checkingSlope = true;
                landY = spiderSlopeY(player, obj);
            }

            if ((!player->upsideDown && landY < player->y - 10.0f) ||
                (player->upsideDown && landY > player->y + 10.0f)) continue;

            if (!goingLeft) {
                if (objGetLeft(obj) <= player->x) break;
            } else {
                if (objGetRight(obj) >= player->x) break;
            }

            bool hit = false;

            for (int j = 0; j < solidCount; j++) {
                if (j == i) continue;

                int inner = spiderSolidBuffer[j];
                float objectHeight = objects.height[inner] - (checkingSlope ? 0.0f : 4.0f);

                if (spiderOverlap(player->x, landY, playerWidth, playerHeight, objects.x[inner], objects.y[inner], objects.width[inner], objectHeight)) {
                    if (checkingSlope) {
                        if (prevLandY != 0.0f) landY = !player->upsideDown ? MIN(landY, prevLandY) : MAX(landY, prevLandY);
                    } else {
                        landY = prevLandY;
                    }

                    hit = true;
                    prevLandY = landY;
                    break;
                }
            }

            if (!hit) break;
        }

        if (i >= solidCount) landY = 0.0f;
    }

    if (landY == 0.0f) {
        landY = player->upsideDown ? (min_teleport + playerHeight * 0.5f) : (max_teleport - playerHeight * 0.5f);
    }

    if (!onlyDamaging && hazardCount != 0) {
        float hazardY = objects.y[spiderHazardBuffer[0]];

        if ((!player->upsideDown && hazardY < landY) ||
            (player->upsideDown && hazardY > landY)) {
            if (!isRetry) {
                spiderTestJump(player, true);
                return;
            }

            landY = hazardY;
        }
    }

    float oldY = player->y;
    player->y = landY;
    player->upsideDown = !player->upsideDown;
    player->velY = player->upsideDown ? 1.0f : -1.0f;
    player->onGround = false;
    player->onCeiling = false;
    player->leftGround = true;
    player->inverseRotation = false;
    player->snapRotation = true;
    player->bufferingState = static_cast<int>(BufferingState::BufferEnd);

    Color dashColor = getWhiteIfBlack(state.currentPlayer == 1 ? p1Color : p2Color);
    addSpiderDashEffect(player->x, oldY, landY, player->upsideDown, dashColor);

    playerFlash(player, 0.3f, 0.05f);

    player->spiderScaleActive = true;
    player->spiderScaleTimer = 0.f;

    UseEffect *effect = addUseEffect(player->x, landY, USE_EFFECT_OBJ_NOTHING, &spiderInEffect, getUseEffectArrayPtr(static_cast<int>(Screens::ScreenTop)));
    if (effect) {
        Color p2NotWhite = getWhiteIfBlack(p2Color);

        effect->def.colorR = p2NotWhite.r / 255.f;
        effect->def.colorG = p2NotWhite.g / 255.f;
        effect->def.colorB = p2NotWhite.b / 255.f;
        effect->def.startRad = spiderInEffect.startRad * (player->mini ? 0.6f : 1);
    }
    effect = addUseEffect(player->x, oldY, USE_EFFECT_OBJ_NOTHING, &spiderOutEffect, getUseEffectArrayPtr(static_cast<int>(Screens::ScreenTop)));
    if (effect) {
        Color p2NotWhite = getWhiteIfBlack(p2Color);

        effect->def.colorR = p2NotWhite.r / 255.f;
        effect->def.colorG = p2NotWhite.g / 255.f;
        effect->def.colorB = p2NotWhite.b / 255.f;
        effect->def.startRad = spiderOutEffect.startRad * (player->mini ? 0.6f : 1);
    }
    updateRotationDirection(player);
    clearSlopeData(player);
    
    motionTrailClear(trail);
    trail->positionR = (Vec2D){player->x, player->y};
}

void spiderGamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};
    trail->startingPositionInitialized = true;

    player->minVel = -810;
    player->maxVel = 810;

    if (!player->upsideDown && player->velY < player->minVel) player->velY = player->minVel;
    if (player->upsideDown && player->velY > player->maxVel) player->velY = player->maxVel;

    player->gravity = -2794.1082f * 0.6f;

    if (player->y > 2794.f) killPlayer(DeathReason::DeathFellOffLevel);

    if (player->onGround) {
        if (!player->dashing) setAnimation(player, (state.speed <= static_cast<int>(PlayerSpeeds::SpeedNormal)) ? static_cast<int>(SpiderAnimId::SpiderAnimWalk) : static_cast<int>(SpiderAnimId::SpiderAnimRun));
        motionTrailStopStroke(trail);

        if (player->slopeData.slopeID < 0 && !player->dashing) player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }

    if (player->snapRotation) {
        player->rotation = 0;
    }

    dragParticles[state.currentPlayer].emitterX = player->x;
    dragParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -2 : 2);
    dragParticles[state.currentPlayer].emitting = player->onGround || player->onCeiling;

    dragParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
    dragParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);

    dragParticles[state.currentPlayer].cfg.sourcePositionVariancey = 0;

    bool cameFromBlackOrb = (player->lastOrbPressed >= 0 && objects.id[player->lastOrbPressed] == BLACK_ORB);

    if ((player->slopeData.slopeID >= 0 || player->onGround) && (currInput.holdJump && (player->bufferingState == static_cast<int>(BufferingState::BufferReady) || cameFromBlackOrb)) && !player->dashing) {
        spiderTestJump(player, false);
        player->bufferingState = static_cast<int>(BufferingState::BufferEnd);
        player->jumped = true;
        player->lastOrbPressed = -1;
    }

    if (!player->onGround && player->velY < 0 && player->animID != static_cast<int>(SpiderAnimId::SpiderAnimFall)) {
        setAnimation(player, static_cast<int>(SpiderAnimId::SpiderAnimFall));
    }

    const PlayerAnimation *anim = &spiderAnimations[player->animID];

    float animSpeed = (state.speed == static_cast<int>(PlayerSpeeds::SpeedSlow)) ? 0.9f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFast)) ? 1.15f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFaster)) ? 1.3f
                     : (state.speed == static_cast<int>(PlayerSpeeds::SpeedFastest)) ? 1.5f : 1.0f;

    if (player->mini) animSpeed /= 0.85f;
    if (state.speed >= static_cast<int>(PlayerSpeeds::SpeedFast)) animSpeed /= 1.1f;

    player->animTimer += STEPS_DT * animSpeed;

    if (player->animTimer >= anim->frames[player->animFrame].delay) {
        player->animTimer -= anim->frames[player->animFrame].delay;

        int next = player->animFrame + 1;

        if (next >= anim->frameCount) next = 0;

        player->animFrame = next;
    }
}

void clampPlayerGround(Player *player) {
    bool slopeCheck = player->slopeData.slopeID >= 0 && (gravSlopeOrient(player->slopeData.slopeID, player) == static_cast<int>(SlopeOrientations::OrientNormalDown) || gravSlopeOrient(player->slopeData.slopeID, player) == static_cast<int>(SlopeOrientations::OrientUdDown));

    // Check for ground collision
    if (getGroundBottom(player) < state.groundY && !player->justTeleported) {
        if (player->ceilingInvTime <= 0 && (player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) || player->gamemode == static_cast<int>(IconGamemode::GamemodeRobot)) && player->upsideDown) {
            killPlayer(DeathReason::DeathCeiling);
        }

        if (slopeCheck) {
            clearSlopeData(player);
        }
        
        if (player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) && grav(player, player->velY) <= 0) setPVelocity(player, 0, player->gamemode == static_cast<int>(IconGamemode::GamemodeBall));
        player->y = state.groundY + (player->height / 2) + ((player->gamemode == static_cast<int>(IconGamemode::GamemodeWave)) ? (player->mini ? 3 : 5) : 0);;
        player->snapData.playerFrame = 0;    
    }

    // Check for ceiling collision
    if (getGroundTop(player) > state.ceilingY && !player->justTeleported) {
        if (player->ceilingInvTime <= 0 && (player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) || player->gamemode == static_cast<int>(IconGamemode::GamemodeRobot)) && !player->upsideDown) {
            killPlayer(DeathReason::DeathCeiling);
        }

        if (slopeCheck) {
            clearSlopeData(player);
        }
        
        if (player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) && grav(player, player->velY) >= 0) setPVelocity(player, 0, player->gamemode == static_cast<int>(IconGamemode::GamemodeBall));
        player->y = state.ceilingY - (player->height / 2) - ((player->gamemode == static_cast<int>(IconGamemode::GamemodeWave)) ? (player->mini ? 3 : 5) : 0);
        
    } 
}
void runPlayer(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;
    trail->stroke = trail->width * scale;
    waveTrail->stroke = 18.f * mapRange(amplitude, 0.f, 1.f, 0.1f, 1.f) * scale;

    if (!player->leftGround) {
        // Ground
        if (getGroundBottom(player) <= state.groundY) {
            if (player->upsideDown) {
                player->onCeiling = true;
                player->inverseRotation = false;
            } else {
                player->onGround = true;      
                player->inverseRotation = false;
                playerNonFlyingLanding(player);
            }
            player->timeSinceGround = 0; 
        } 

        // Ceiling
        if (getGroundTop(player) >= state.ceilingY) {
            if (player->upsideDown) {
                player->onGround = true;
                player->inverseRotation = false;
                playerNonFlyingLanding(player);
            } else {
                player->onCeiling = true;     
                player->inverseRotation = false;     
            } 
            player->timeSinceGround = 0; 
        } 
    }
    
    if (state.currentPlayer == 0) handleAutoCheckpoints(STEPS_DT);
    
    // Handle land particles
    if (player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) && !state.oldPlayer.onGround && player->onGround) {
        landParticles[state.currentPlayer].emitterX = player->x;
        landParticles[state.currentPlayer].emitterY = fabsf(gravBottom(player)) + (player->upsideDown ? -4 : 4);
        landParticles[state.currentPlayer].gravityFlipped = player->upsideDown;
        landParticles[state.currentPlayer].scale = (player->mini ? 0.6f : 1.0f);
        spawnMultipleParticles(&landParticles[state.currentPlayer], 10);
    }

    // Coyote time (only applies to upside down gravity)
    if (gravBottom(&state.oldPlayer) > gravFloor(&state.oldPlayer) && player->upsideDown == state.oldPlayer.upsideDown && !player->onGround && player->velY <= 0) {
		if (state.oldPlayer.onGround && !currOldInput.holdJump)
			player->coyoteFrames = 0;
		player->coyoteFrames++;
	} else {
		player->coyoteFrames = INT32_MAX;
	}

    // Disable cap
    if (player->onGround) player->velocityUncapped = false;

    switch (player->gamemode) {
        case static_cast<int>(IconGamemode::GamemodePlayer):
            cubeGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeShip):
            glitterParticles.emitterX = state.cameraXMiddle;
            glitterParticles.emitterY = state.cameraYMiddle;
            glitterParticles.emitting = true;
            if (!state.mirroring && state.showPlayer) motionTrailResumeStroke(trail);
            shipGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeBall):
            ballGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeUfo):
            glitterParticles.emitterX = state.cameraXMiddle;
            glitterParticles.emitterY = state.cameraYMiddle;
            glitterParticles.emitting = true;
            if (!state.mirroring && state.showPlayer) motionTrailResumeStroke(trail);
            ufoGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeWave):
            glitterParticles.emitterX = state.cameraXMiddle;
            glitterParticles.emitterY = state.cameraYMiddle;
            glitterParticles.emitting = true;
            if (!state.mirroring && state.showPlayer) {
                if (settingsState.noWaveTrailBehind) {
                    motionTrailStopStroke(trail);
                } else {
                    motionTrailResumeStroke(trail);
                }
            }
            waveGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeRobot):
            robotGamemode(player);
            break;
        case static_cast<int>(IconGamemode::GamemodeSpider):
            spiderGamemode(player);
            break;
    } 
    
    if (player->flashActive) {
        player->flashTimer += STEPS_DT;
        if (player->flashTimer >= player->flashDelay + player->flashDuration) player->flashActive = false;
    }

    if (player->spiderScaleActive) {
        player->spiderScaleTimer += STEPS_DT;
        if (player->spiderScaleTimer >= SPIDER_DASH_SCALE_DURATION) player->spiderScaleActive = false;
    }

    updateRobotFire(player);

    player->blendTimer += STEPS_DT;
    if (player->blendTimer > getBlendDuration(player)) {
        setBlendTimer(player, getBlendDuration(player));
    }
    
    player->timeSinceGround += STEPS_DT;

    // Fade wave trail if not in wave anymore or end animation started or mirror portal animation is happening
    if (player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) || state.mirroring || player->cutsceneTimer > 0) {
        if (waveTrail->opacity > 0) waveTrail->opacity -= 0.02f;
        
        if (waveTrail->opacity <= 0) {
            waveTrail->opacity = 0;
            waveTrail->nuPoints = 0;
        }
    }

    // Weird stuff indeed
    if (!player->velocityOverride && !player->dashing) {
		float newVel = player->velY + player->gravity * STEPS_DT;

		// Player will fall off blocks a frame faster than expected
		if (!(player->onGround || player->onCeiling) && (state.oldPlayer.onGround || state.oldPlayer.onCeiling) && ((!currInput.holdJump && (currOldInput.pressedJump || currInput.pressedJump)) || player->bufferingState == static_cast<int>(BufferingState::BufferReady)) && gravBottom(&state.oldPlayer) > gravFloor(&state.oldPlayer) && player->mini == state.oldPlayer.mini) {
			player->y += grav(&state.oldPlayer, state.oldPlayer.gravity) * STEPS_DT * STEPS_DT;

			if (player->velY == 0)
				newVel += state.oldPlayer.gravity * STEPS_DT;
		}

        player->velY = newVel;
	}

    if (player->cutsceneTimer > 0) return;

    player->rotation = normalizeAngle(player->rotation);

    player->leftGround = false;

    // If the player hits a gravity changing object, theres a small period of time where the cube doesn't die to ceilings
    if (player->ceilingInvTime > 0) {
        player->ceilingInvTime -= STEPS_DT;
    } else {
        player->ceilingInvTime = 0;
    }

    clampPlayerGround(player);
    
    // Coyote time for slopes, so you can jump if you only touch the slope for a single frame (and fixes some visual bugs)
    if (player->slopeSlideCoyoteTime) {
        player->slopeSlideCoyoteTime--;

        if (!player->slopeSlideCoyoteTime) {
            player->coyoteSlope.slopeID = -1;
            player->coyoteSlope.elapsed = 0;
            player->coyoteSlope.snapDown = false;
        }
    }

    // If player is on slope, do slope stuff
    if (player->slopeData.slopeID >= 0) {
        slopeCalc(player->slopeData.slopeID, player);
    }

    // Handle wave trail point adding
    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeWave) && !state.mirroring) {
        waveTrail->positionR = (Vec2D){player->x, player->y};  
        waveTrail->startingPositionInitialized = true;
        if (player->velY != state.oldPlayer.velY || player->onGround != state.oldPlayer.onGround || player->onCeiling != state.oldPlayer.onCeiling) {
            motionTrailAddWavePoint(waveTrail);
        }
    }

    float lerpSpeed = playerSpeedMults[state.speed] * 0.175f;

    // Handle rotation for ship and wave
    if (!player->dashing) switch (player->gamemode) {
        case static_cast<int>(IconGamemode::GamemodePlayer):
            if (player->onGround || player->slopeData.slopeID >= 0) {
                if (player->slopeData.slopeID < 0 && !player->slopeSlideCoyoteTime) lerpSpeed *= 3.0f;
                player->rotation = RadToDeg(slerpFancy(DegToRad(player->rotation), DegToRad(player->cubeTargetRotation), MIN(STEPS_DT, STEPS_DT * lerpSpeed) * 60));
            } else {
                player->cubeTargetRotation = player->rotation;
            }
            break;
        case static_cast<int>(IconGamemode::GamemodeShip):
            rotateFly(player, 0.15f);
            break;
        case static_cast<int>(IconGamemode::GamemodeUfo):
            rotateFly(player, 0.07f);
            break;
        case static_cast<int>(IconGamemode::GamemodeWave):
            rotateFly(player, player->mini ? 0.4f : 0.25f);
            break;
        case static_cast<int>(IconGamemode::GamemodeRobot):
        case static_cast<int>(IconGamemode::GamemodeSpider):
            float angleRad = 0;
            if (player->slopeData.slopeID >= 0) {
                angleRad = slopeSnapAngle(player->slopeData.slopeID, player);
            }
            player->rotation = RadToDeg(slerpFancy(DegToRad(player->rotation), angleRad, MIN(STEPS_DT, STEPS_DT * lerpSpeed) * 60));
            break;       
    }

    player->snapRotation = false;

    player->bufferUfo = false;

    if (player->onGround) player->lastOrbPressed = -1;
}

void handlePlayer(Player *player) {
    u64 startPlayer = svcGetSystemTick();

    if (currInput.holdJump) {    
        if (player->bufferingState == static_cast<int>(BufferingState::BufferNone)) {
            player->bufferingState = static_cast<int>(BufferingState::BufferReady);
        }
    } else {
        player->holdingJumpCancelled = false;
        player->bufferingState = static_cast<int>(BufferingState::BufferNone);
        stopDashing(player);
    }
    
    player->onGround = false;
    player->onCeiling = false;
    player->landedFromJump = false;
    
    player->allowDartSlide = false;
    player->allowHeadHit = false;

    player->velocityOverride = false;

    player->gravObjID = -1;
    player->potentialSlopeID = -1;
    
    player->timeElapsed += STEPS_DT;

    player->velX = playerSpeeds[state.speed]; 
    player->x += player->velX * STEPS_DT;
    if (player->dashing) {
        player->velY = 0;
        player->y += (player->velX * STEPS_DT) * player->dashSlope;
        player->dashFireTimer += STEPS_DT;
        if (player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) || player->gamemode == static_cast<int>(IconGamemode::GamemodeBall)) {
            player->rotation += (player->upsideDown ? -1.f : 1.f) * (288.f / 0.3f) * STEPS_DT;
        } else {
            player->rotation = -player->dashAngle;
        }
    } else {
        player->y += playerGetVel(player, player->velY) * STEPS_DT;
    }

    clampPlayerGround(player);

    player->justTeleported = false;

    player->frame++;

    // If new vel y is no the max (not set) set vel y
    if (player->newVelY != __FLT_MAX__) {
        player->velY = player->newVelY;
        player->newVelY = __FLT_MAX__;
    }

    // Run actions that were queued last frame
    for (int i = 0; i < numActions[state.currentPlayer]; i++) {
        playerActions[state.currentPlayer][i].func(player);
    }
    numActions[state.currentPlayer] = 0;

    // The svc calls are for profiling
    u64 start = svcGetSystemTick();
    collideWithObjects(player);
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.collisionMs += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip || state.endWallAnimPlaying) state.dead = false;
    
    if (state.dead) {
        stopDashing(player);
        return;
    }

    start = svcGetSystemTick();

    if (player->x >= levelInfo.wallX - END_ANIMATION_X_START) {
        stopDashing(player);
        state.endWallAnimPlaying = true;
        p1Trail = true;
        if (player->cutsceneTimer == 0) {
            // Add a trail point for wave
            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeWave)) motionTrailAddWavePoint(waveTrail);

            player->cutsceneInitialPlayerX = player->x;
            player->cutsceneInitialPlayerY = player->y;
        }
        animPlayerToWall(player);
        player->rotation += easeValue(EaseTypes::EaseIn, 0, 415.3848f, player->cutsceneTimer, 0.5f, 2.f) * STEPS_DT;
        player->cutsceneTimer += STEPS_DT;
        
        // End level
        if (player->x > levelInfo.wallX) {
            levelInfo.completing = true;
        }
    } 

    runPlayer(player);

    if (state.dead) stopDashing(player);

    end = svcGetSystemTick();
    ticks = end - start;
    snapshot.playMs += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    // Run dual ball being repelled when touching
    doBallReflection();

    player->deltaY = player->y - state.oldPlayer.y;
    
    u64 endPlayer = svcGetSystemTick();
    ticks = endPlayer - startPlayer;

    // Add player hitboxes to hitbox trail
    if (state.hitboxDisplay == 2) addNewHitbox(player);
    snapshot.handlerMs += ticks / CPU_TICKS_PER_MSEC;
}

void animPlayerToWall(Player *player) {
    float t = CLAMP(powf(player->cutsceneTimer, 1.2f), 0, 1);

    // (1 - t) and powers
    float oneMinusT = 1.0f - t;
    float oneMinusTSquared = oneMinusT * oneMinusT;
    float oneMinusTCubed = oneMinusTSquared * oneMinusT;

    float tSquared = t * t;
    float tCubed = tSquared * t;

    // Start point
    float startX = player->cutsceneInitialPlayerX;
    float startY = player->cutsceneInitialPlayerY;

    // Final;destination
    float finalX = levelInfo.wallX + 50.f;
    float finalY = levelInfo.wallY - 20.f;

    // Mid point
    float midX = startX + 40.f;
    float midY = levelInfo.wallY + 150.f;

    // Cubic Bezier interpolation
    player->x = 
        oneMinusTCubed * startX + 
        oneMinusTSquared * 3 * t * startX +
        oneMinusT * 3 * tSquared * midX + tCubed * finalX;

    player->y = 
        oneMinusTCubed * startY + 
        oneMinusTSquared * 3 * t * startY +
        oneMinusT * 3 * tSquared * midY + tCubed * finalY;
}


void spawnP1Trail(Player *player, int playerID) {
    P1Trail *trailData = &state.p1TrailData[playerID][state.p1TrailPos[playerID]];
    
    float scale = (player->mini) ? 0.6f : 1.f;

    switch (player->gamemode) {
        case static_cast<int>(IconGamemode::GamemodePlayer):
        case static_cast<int>(IconGamemode::GamemodeBall):
        case static_cast<int>(IconGamemode::GamemodeWave):
            trailData->gamemode = player->gamemode;
            trailData->scale = scale;
            trailData->upsideDown = false;
            break;
        case static_cast<int>(IconGamemode::GamemodeShip):
        case static_cast<int>(IconGamemode::GamemodeUfo):
            trailData->gamemode = static_cast<int>(IconGamemode::GamemodePlayer);
            trailData->scale = scale * 0.5f;
            trailData->upsideDown = player->upsideDown;
            break;
        case static_cast<int>(IconGamemode::GamemodeRobot):
        case static_cast<int>(IconGamemode::GamemodeSpider):
            trailData->gamemode = player->gamemode;
            trailData->scale = scale;
            trailData->upsideDown = player->upsideDown;
            break;
    }

    float end_scale = trailData->scale * P1_TRAIL_END_SCALE;

    trailData->deltaScale = (end_scale - trailData->scale) / P1_TRAIL_DURATION;
    
    trailData->x = player->x;
    trailData->y = player->y;
    trailData->rot = player->rotation;
    trailData->opacity = 0.7f;
    trailData->life = P1_TRAIL_DURATION;

    trailData->active = true;

    state.p1TrailPos[playerID]++;
    if (state.p1TrailPos[playerID] >= P1_TRAIL_LENGTH) {
        state.p1TrailPos[playerID] = 0;
    }
}

#define P1_TRAIL_RATE 0.05f

void updateP1Trail(Player *player, int playerID) {
    // Spawn new p1 icon 
    if (!state.dead && p1Trail && p1TrailTimer <= 0) {
        p1TrailTimer = P1_TRAIL_RATE;
        // If current player is player 2, dual must be true
        if (playerID == 0 || state.dual) {
            spawnP1Trail(player, playerID);
        }
    }

    p1TrailTimer -= delta;

    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trailData = &state.p1TrailData[playerID][i];

        if (trailData->active) {
            trailData->opacity -= (0.8f / P1_TRAIL_DURATION) * delta;
            trailData->scale += trailData->deltaScale * delta;

            trailData->life -= delta;

            if (trailData->life <= 0) {
                trailData->active = false;
            }
        }
    }
}

void drawP1Trail(Player *player, int playerID) {
    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trailData = &state.p1TrailData[playerID][i];

        if (trailData->active) {
            float calcX = ((trailData->x - state.cameraX));
            float calcY = SCREEN_HEIGHT - ((trailData->y - state.cameraY));
            bool flipX = (state.mirrorMult < 0);

            u32 color = (playerID == 1) ? C2D_Color32(p2Color.r, p2Color.g, p2Color.b, trailData->opacity * 255) : C2D_Color32(p1Color.r, p1Color.g, p1Color.b, trailData->opacity * 255);

            spawnP1LayerAt(
                trailData->gamemode, ((settingsState.defaultMiniIcon && player->mini && (trailData->gamemode == 0 || trailData->gamemode == 2)) ? 0 : *currentIcons[trailData->gamemode]), 
                getMirrorX(calcX, state.mirrorFactor), calcY, 
                trailData->rot,  
                flipX, trailData->upsideDown,
                trailData->scale,
                color,
                (IconParameters) { 
                    .animID = player->animID,
                    .animFrame = player->animFrame,
                    .animTimer = player->animTimer,
                    .blendFromAnimID = player->blendFromAnimID,
                    .blendFromFrame = player->blendFromFrame,
                    .blendFromTimer = player->blendFromTimer,
                    .blendFraction = player->blendTimer / getBlendDuration(player)
                 }
            );
        }
    }
}


void drawPlayer(Player *player) {
    // Don't draw player if dead
    if (state.dead) return;

    // Don't draw player if respawning and should hide
    if (state.respawnEffectData.active && state.respawnEffectData.hidePlayer) return;

    float calcX = ((player->x - state.cameraX));
    float calcY = SCREEN_HEIGHT - ((player->y - state.cameraY));

    Color p1Color = player->playerIcons.p1Color;
    Color p2Color = player->playerIcons.p2Color;
    Color glowColor = player->playerIcons.glowColor;

    if (player->flashActive) {
        float flashT = player->flashTimer - player->flashDelay;
        float flash_k = (flashT <= 0.f) ? 0.f : CLAMP(flashT / player->flashDuration, 0.f, 1.f);

        p1Color.r = (unsigned char)(255.f + ((float)p1Color.r - 255.f) * flash_k);
        p1Color.g = (unsigned char)(255.f + ((float)p1Color.g - 255.f) * flash_k);
        p1Color.b = (unsigned char)(255.f + ((float)p1Color.b - 255.f) * flash_k);

        p2Color.r = (unsigned char)(255.f + ((float)p2Color.r - 255.f) * flash_k);
        p2Color.g = (unsigned char)(255.f + ((float)p2Color.g - 255.f) * flash_k);
        p2Color.b = (unsigned char)(255.f + ((float)p2Color.b - 255.f) * flash_k);
    }

    u32 primaryColor = C2D_Color32(p1Color.r, p1Color.g, p1Color.b, 255);
    u32 secondaryColor = C2D_Color32(p2Color.r, p2Color.g, p2Color.b, 255);

    if (state.currentPlayer == 1) {
        u32 tmp = primaryColor;
        primaryColor = secondaryColor;
        secondaryColor = tmp;
    }

    float scale = (player->mini) ? 0.6f : 1.f;

    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeSpider) && player->spiderScaleActive) {
        float scale_t = CLAMP(player->spiderScaleTimer / SPIDER_DASH_SCALE_DURATION, 0.f, 1.f);
        scale *= SPIDER_DASH_SCALE_START + (1.f - SPIDER_DASH_SCALE_START) * elasticEaseOut(scale_t, SPIDER_DASH_ELASTIC_PERIOD);
    }

    // All of this is just for calculating the cube position in ufo and ship
    float rad = C3D_AngleFromDegrees(player->rotation);
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    bool flipX = (state.mirrorMult < 0);
    int flipYMult = (player->upsideDown ? -1 : 1);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = sinR;
    float m11 = -cosR;

    const float localX = 0;
    const float localY = ((player->gamemode == static_cast<int>(IconGamemode::GamemodeShip)) ? 6 : 4) * flipYMult;

    float rotX = localX * m00 + localY * m01;
    float rotY = localX * m10 + localY * m11;

    float pX = getMirrorX(calcX + rotX * scale, state.mirrorFactor);
    float pY = calcY + rotY * scale;

    float calcXMirror = getMirrorX(calcX, state.mirrorFactor);
    float pRot = player->rotation;

    int selectedCube = player->playerIcons.cube;
    int selectedShip = player->playerIcons.ship;
    int selectedBall = player->playerIcons.ball;
    int selectedUfo =  player->playerIcons.ufo;
    int selectedWave = player->playerIcons.wave;
    int selectedRobot = player->playerIcons.robot;
    int selectedSpider = player->playerIcons.spider;

    bool glowEnabled = (player->playerIcons.glow || ((player->playerIcons.p1Color.r | player->playerIcons.p1Color.g | player->playerIcons.p1Color.b) == 0));

    switch (player->gamemode) {
        case static_cast<int>(IconGamemode::GamemodePlayer):
            spawnIconAt(static_cast<int>(IconGamemode::GamemodePlayer), (settingsState.defaultMiniIcon && player->mini) ? 0 : selectedCube, glowEnabled, calcXMirror, calcY, pRot, flipX, false, scale,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case static_cast<int>(IconGamemode::GamemodeShip):
            if (glowEnabled) {
                spawnGlowLayerAt(static_cast<int>(IconGamemode::GamemodeShip), selectedShip, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                    C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255), 
                    (IconParameters) { 0 }
                );
            }
            spawnIconAt(static_cast<int>(IconGamemode::GamemodePlayer), (settingsState.defaultMiniIcon && player->mini) ? 0 : selectedCube, glowEnabled, pX, pY, pRot, flipX, player->upsideDown, scale * 0.5f,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 0 }
            );
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeShip), selectedShip, glowEnabled, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                primaryColor,
                secondaryColor,
                0,
                (IconParameters) { 0 }
            );
            break;
        case static_cast<int>(IconGamemode::GamemodeBall):
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeBall), (settingsState.defaultMiniIcon && player->mini) ? 0 : selectedBall, glowEnabled, calcXMirror, calcY, pRot, flipX, false, scale,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case static_cast<int>(IconGamemode::GamemodeUfo):
            if (glowEnabled) {
                spawnGlowLayerAt(static_cast<int>(IconGamemode::GamemodeUfo), selectedUfo, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                    C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                    (IconParameters) { 0 }
            );
            }
            spawnIconAt(static_cast<int>(IconGamemode::GamemodePlayer), (settingsState.defaultMiniIcon && player->mini) ? 0 : selectedCube, glowEnabled, pX, pY, pRot, flipX, player->upsideDown, scale * 0.5f,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 0 }
            );
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeUfo), selectedUfo, glowEnabled, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                primaryColor,
                secondaryColor,
                0,
                (IconParameters) { 0 }
            );
            break;    
        case static_cast<int>(IconGamemode::GamemodeWave):
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeWave), selectedWave, glowEnabled, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case static_cast<int>(IconGamemode::GamemodeRobot): {
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeRobot), selectedRobot, glowEnabled, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) { 
                    .animID = player->animID,
                    .animFrame = player->animFrame,
                    .animTimer = player->animTimer,
                    .blendFromAnimID = player->blendFromAnimID,
                    .blendFromFrame = player->blendFromFrame,
                    .blendFromTimer = player->blendFromTimer,
                    .blendFraction = player->blendTimer / getBlendDuration(player),
                    .robotFireScale = player->robotFireVisible ? player->robotFireScale : 0.f
                 }
            );
            break;
        }
        case static_cast<int>(IconGamemode::GamemodeSpider): {
            spawnIconAt(static_cast<int>(IconGamemode::GamemodeSpider), selectedSpider, glowEnabled, calcXMirror, calcY, pRot, flipX, player->upsideDown, scale,
                primaryColor,
                secondaryColor,
                C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
                (IconParameters) {
                    .animID = player->animID,
                    .animFrame = player->animFrame,
                    .animTimer = player->animTimer,
                    .blendFromAnimID = player->blendFromAnimID,
                    .blendFromFrame = player->blendFromFrame,
                    .blendFromTimer = player->blendFromTimer,
                    .blendFraction = player->blendTimer / getBlendDuration(player)
                 }
            );
            break;
        }
    }
}

extern void getCorners(float cx, float cy, float w, float h, float angle, Vec2D out[4]);


void drawTriangleFromRect(Vec2D rect[4], int skipIndex, uint32_t color, uint32_t hypoColor, float depth) {
    Vec2D tri[3];
    u32 col[3];
    int ti = 0;

    int hypoID = skipIndex - 1;
    if (hypoID < 0) hypoID = 3;

    // Collect 3 points that are not the skipped one
    for (int i = 0; i < 4; ++i) {
        if (i == skipIndex) continue;
        tri[ti].x = mirrorXOnScreen(rect[i].x);
        tri[ti].y = calcYOnScreen(rect[i].y);
        
        if (i == hypoID) col[ti++] = hypoColor;
        else col[ti++] = color;
    }

    drawPolygonInwardMitered(tri, 3, 2.f, col, depth);
}

void drawSquare(Vec2D rect[4], uint32_t color, float depth) {
    Vec2D center = {
        (rect[0].x + rect[1].x + rect[2].x + rect[3].x) / 4.f,
        (rect[0].y + rect[1].y + rect[2].y + rect[3].y) / 4.f
    };

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;

        drawHitboxLineInward(rect,
            mirrorXOnScreen(rect[i].x), calcYOnScreen(rect[i].y),
            mirrorXOnScreen(rect[j].x), calcYOnScreen(rect[j].y),
            1.5f, center.x, center.y, color, depth
        );
    }
}

static u32 getIDColor(int id, float *outDepth) {
    switch (id) {
        case J_MODIFIERBLOCK:
        case D_MODIFIERBLOCK:
        case S_MODIFIERBLOCK:
        case H_MODIFIERBLOCK:
            *outDepth = 0.75f;
            return C2D_Color32(0xff, 0xff, 0x00, 0xff);
    }
    return 0;
}

void drawHitbox(int obj) {
    int id = objects.id[obj];
    if (!isValidObject(id)) return;

    Vec2D rect[4];
    if (isTriggerObject(id) && objects.touchTriggered[obj]) {
        getCorners(objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], objects.rotation[obj], rect);
        drawSquare(rect, C2D_Color32(0x00, 0xff, 0xff, 0xff), 3.f);
        return;
    }

    const ObjectHitbox *hitbox = gameObjects[id].hitbox;

    if (!hitbox) return;

    float depth = 0.f;

    float angle = -objects.rotation[obj];
    float rot_rad = C3D_AngleFromDegrees(adjustAngleY(angle, objects.flippedH[obj]));
    float cosR = cosf(rot_rad), sinR = sinf(rot_rad);

    float rawOffX = hitbox->x * objects.scaleX[obj];
    float rawOffY = hitbox->y * objects.scaleY[obj];

    float offX = rawOffX * cosR - rawOffY * sinR;
    float offY = rawOffX * sinR + rawOffY * cosR;

    float x = objects.x[obj] + offX;
    float y = objects.y[obj] + offY;
    float w = objects.width[obj];
    float h = objects.height[obj];

    float rot = objects.rotation[obj];

    if (hitbox->collisionType == static_cast<int>(HitboxType::HitboxSolid)) {
        rot = 0;
    }
    unsigned int color = C2D_Color32(0x00, 0xff, 0xff, 0xff);

    int hitboxType = hitbox->collisionType;
    if (hitboxType == static_cast<int>(HitboxType::HitboxHazard)) {
        color = C2D_Color32(0xff, 0x00, 0x00, 0xff);
        depth = 0.5f;
    }
    if (hitboxType == static_cast<int>(HitboxType::HitboxSolid)) {
        color = C2D_Color32(0x00, 0x00, 0xff, 0xff);
        depth = 0.f;
    }
    if (hitboxType == static_cast<int>(HitboxType::HitboxSpecial)) {
        color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
        depth = 1.f;
    }

    u32 idColor = getIDColor(id, &depth);
    if (idColor) color = idColor;

    // Current slope and x snapped obj gets its own colors
    if (obj == state.player.slopeData.slopeID || obj == state.player2.slopeData.slopeID) color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
    if (obj == state.player.snapData.snappedObj || obj == state.player2.snapData.snappedObj) color = C2D_Color32(0xff, 0xff, 0x00, 0xff);

    if (hitbox->type == static_cast<int>(CollisionShape::CollisionSlope)) {
        w = objects.width[obj];
        h = objects.height[obj];
        getCorners(x, y, w, h, 0, rect);

        depth = 0.f;

        if (isSpikeSlope(obj)) {
            depth += 0.5f;
            drawTriangleFromRect(rect, 3 - objects.orientation[obj], color, C2D_Color32(0xff, 0x00, 0x00, 0xff), depth);
        } else {
            drawTriangleFromRect(rect, 3 - objects.orientation[obj], color, color, depth);
        }
    } else if (hitbox->type == static_cast<int>(CollisionShape::CollisionCircle)) {
        float calcRadius = objects.width[obj];

        customCircunference(mirrorXOnScreen(x), calcYOnScreen(y), calcRadius, color, 2.f);
    } else if (w != 0 && h != 0) {
        getCorners(x, y, w, h, rot, rect);
        drawSquare(rect, color, depth);
    }
}

void drawPlayerHitbox(Player *player) {
    Vec2D rect[4];

    // Unrotated hitbox
    getCorners(player->x, player->y, player->width, player->height, 0, rect);

    drawSquare(rect, C2D_Color32(0xff, 0x00, 0x00, 0xff), 0.f);
}

void drawInternalHitbox(Player *player) {
    InternalHitbox internal = player->internalHitbox;
    Vec2D rect[4];
    // Internal hitbox
    getCorners(player->x, player->y, internal.width, internal.height, 0, rect);

    drawSquare(rect, C2D_Color32(0x00, 0x00, 0xff, 0xff), 0.f);
}

void drawRotatedHitbox(Player *player) {
    Vec2D rect[4];
    // Rotated hitbox
    getCorners(player->x, player->y, player->width, player->height, player->rotation, rect);

    drawSquare(rect, C2D_Color32(0x7f, 0x00, 0x00, 0xff), 0.f);
}


void addNewHitbox(Player *player) {
    for (int i = HITBOX_TRAIL_SIZE - 2; i > 0; i--) {
        state.hitboxTrailPlayers[state.currentPlayer][i] = state.hitboxTrailPlayers[state.currentPlayer][i - 1];
    }
    PlayerHitboxTrail hitbox;
    hitbox.x = player->x;
    hitbox.y = player->y;
    hitbox.width = player->width;
    hitbox.height = player->height;
    hitbox.rotation = player->rotation;
    hitbox.internalHitbox = player->internalHitbox;

    state.hitboxTrailPlayers[state.currentPlayer][0] = hitbox;

    state.lastHitboxTrail++;

    if (state.lastHitboxTrail >= HITBOX_TRAIL_SIZE) state.lastHitboxTrail = HITBOX_TRAIL_SIZE - 1;
}

void drawHitboxTrail(int player) {
    // Draw normal hitbox
    for (int i = state.lastHitboxTrail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitboxTrailPlayers[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.rotation = hitbox.rotation;

        drawPlayerHitbox(&player);
    }
    // Draw internal
    for (int i = state.lastHitboxTrail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitboxTrailPlayers[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.internalHitbox = hitbox.internalHitbox;
        player.rotation = hitbox.rotation;

        drawInternalHitbox(&player);
    }
}

void pushPlayerAction(void (*func)(Player *)) {
    int count = numActions[state.currentPlayer];
    if (count < MAX_ACTIONS) {
        playerActions[state.currentPlayer][numActions[state.currentPlayer]++].func = func;
    }
}

}