#pragma once
#include <citro2d.h>
#include "arena.hpp"

#include "particles/particles.hpp"

#define CAM_SPEED 5.19300155f

#define CAMERA_X_OFFSET (0)
#define CAMERA_X_WALL_OFFSET (2 * 30.F)
#define CAMERA_WALL_ANIM_DURATION 1.f

#define LEVEL_Y_OFFSET 90.f
#define CAM_Y_MTX_OFFSET ((constants::ScreenHeight / SCALE - constants::ScreenHeight) - LEVEL_Y_OFFSET)

#define GD_VERSION 2.1
#define LAST_GD_VERSION_ID 97454394  // Last 2.1 id (skibidi level)

// When making a release, uncomment this please thanks
// #define IS_RELEASE

// Increment this every time new properties are added to the server entries
#define SAVED_LEVELS_VERSION 2

namespace gd3ds
{

extern ParticleSystem touchDragParticles;
extern ParticleSystem touchExplosionParticles;
extern ParticleSystem glitterParticlesBottom;
extern ParticleSystem slowSpeedParticlesBottom;
extern ParticleSystem normalSpeedParticlesBottom;
extern ParticleSystem fastSpeedParticlesBottom;
extern ParticleSystem fasterSpeedParticlesBottom;
extern ParticleSystem fastestSpeedParticlesBottom;
extern ParticleSystem endWallParticles;
extern ParticleSystem endWallFirework;
extern ParticleSystem levelCompleteEffectP1;
extern ParticleSystem levelCompleteEffectP2;

extern float slowSpeedParticlesTimer;
extern float normalSpeedParticlesTimer;
extern float fastSpeedParticlesTimer;
extern float fasterSpeedParticlesTimer;
extern float fastestSpeedParticlesTimer;

extern Arena levelArena;

enum class GameState {
    Menu,
    Game,
    Exit
};

enum class Cheats {
    CheatNoclip,
    CheatHitboxDisplay,
    CheatCount
};

enum class Screens {
    ScreenTop,
    ScreenBtm
};

extern bool cheatsUsed[static_cast<int>(Cheats::CheatCount)];
extern const char *cheatNames[static_cast<int>(Cheats::CheatCount)];

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* topRight;
extern C3D_RenderTarget* bot;

extern int gameState;
extern bool escapeState;
extern bool playingMenuLoop;
extern char menuLoopPath[32];

extern int levelResult;

}
