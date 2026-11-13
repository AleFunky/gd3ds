#pragma once
#include <citro2d.h>
#include "arena.hpp"
#include "level_loading.hpp"

#include "particles/particles.hpp"

#include "save/saving.hpp"

#define CAM_SPEED 5.19300155f

#define DT (delta)

#define CAMERA_X_OFFSET (0)
#define CAMERA_X_WALL_OFFSET (2 * 30.F)
#define CAMERA_WALL_ANIM_DURATION 1.f

#define LEVEL_Y_OFFSET 90.f
#define CAM_Y_MTX_OFFSET ((SCREEN_HEIGHT / SCALE - SCREEN_HEIGHT) - LEVEL_Y_OFFSET)


#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define UNUSED __attribute__((unused)) 

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define IN_BOUNDS(index, arr) ((size_t)index < ARRAY_LEN(arr))

#define GD_VERSION 2.1
#define LAST_GD_VERSION_ID 97454394  // Last 2.1 id (skibidi level)

#define GAME_TITLE_ID 0x000400000BB41C00

// When making a release, uncomment this please thanks
// #define IS_RELEASE

// Increment this every time new properties are added to the server entries
#define SAVED_LEVELS_VERSION 2

typedef struct {
    float x, y;
} Vec2D;

extern float delta;
extern float frameTimer;
extern unsigned int frameCounter;

extern int steps;
extern int lastSteps;
extern unsigned int levelFrame;

extern bool exitingLevel;

extern bool songLoaded;

extern bool altTitleScreen;
extern bool isN3Ds;
extern bool is3Dsx;
extern bool isNightly;

extern bool queuedRestart;

extern bool threeDWarningPassed;

extern char *threeDsxPath;

extern float globalVolume;
extern float musicVolume;
extern float soundVolume;
extern float dimSecondaryScreen;

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

#define SCREEN_WIDTH  400
#define SCREEN_BOT_WIDTH  320
#define SCREEN_HEIGHT 240
#define SWAP_SIDE_OFFSET_X ((SCREEN_WIDTH - SCREEN_BOT_WIDTH) / 2.f)

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_EXIT
};

typedef enum Cheats {
    CHEAT_NOCLIP,
    CHEAT_HITBOX_DISPLAY,
    CHEAT_COUNT
} Cheats;

typedef enum Screens {
    SCREEN_TOP,
    SCREEN_BTM
} Screens;

extern bool cheatsUsed[CHEAT_COUNT];
extern const char *cheatNames[CHEAT_COUNT];

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* topRight;
extern C3D_RenderTarget* bot;

extern int gameState;
extern bool escapeState;
extern bool playingMenuLoop;
extern char menuLoopPath[32];

extern int levelResult;

extern bool cheated;

extern bool gamePaused;

extern bool inLevelComplete;

extern float accumulator;
extern bool fixedDt;

void allocateParticles();
void freeParticles();
void initParticles(Color p1Color, Color p2Color);
void updatePlayerEffects(float delta);

void applyVolumeSettings();
float getVolumeSlider();

int outputLog(const char *fmt, ...);
u32 jumpKeyMaskP1(void);
u32 jumpKeyMaskP2(void);
u32 jumpKeyMask(void);
void syncPreciseInput(bool suppressHeld);

void enterExclusiveState();
void exitExclusiveState(bool force);

bool isCitra();

void loadGdpsInfo();

extern ServerFile *currentServerFile;

extern ExternalLevelFile externalFile;
extern ServerFile gdServerFile;
extern ServerFile gdpsFile;