#pragma once

#include <stdbool.h>
#include "player/player.hpp"
#include "graphics.hpp"

#define MAX_LEVEL_HEIGHT 2400.f
#define HITBOX_TRAIL_SIZE 64

#define MIRROR_DURATION 0.4f

#define USER_COIN_UNRATED_R 255
#define USER_COIN_UNRATED_G 175
#define USER_COIN_UNRATED_B 75
#define USER_COIN_UNRATED_TINT C2D_Color32(USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B, 255)

#define USER_COIN_UNRATED_EMPTY_R 165
#define USER_COIN_UNRATED_EMPTY_G 113
#define USER_COIN_UNRATED_EMPTY_B 48
#define USER_COIN_UNRATED_EMPTY_TINT C2D_Color32(USER_COIN_UNRATED_EMPTY_R, USER_COIN_UNRATED_EMPTY_G, USER_COIN_UNRATED_EMPTY_B, 255)

namespace gd3ds
{

extern bool levelUnratedOnline;
extern bool levelVerifiedCoins;
bool levelHasUnverifiedCoins(void);
void levelUnratedOnlineRefresh(void);

u32 userCoinIconTint(bool collected, bool hasRate);

struct PlayerHitboxTrail {
    float x;
    float y;
    float width;
    float height;
    float rotation;
    InternalHitbox internalHitbox;
};

struct KeyInput {
    u8 pressedJump:1;
    u8 holdJump:1;
};

// First flash
#define FLASH_TIME_1 (20.f / STEPS_HZ)
// Rest of timers
#define FLASH_TIME_2 (14.f / STEPS_HZ)

#define RESPAWN_EFFECT_REPEAT 4
#define RESPAWN_EFFECT_DURATION (12.f / STEPS_HZ)

enum class DeathReason {
    DeathTitleScreenKill,
    DeathSpike,
    DeathSaw,
    DeathBlock,
    DeathSlope,
    DeathFellOffLevel,
    DeathCeiling,
    DeathReasonCount
};

enum class FlashState {
    FlashNone,
    FlashFirstLight,
    FlashUnlighted,
    FlashSecondLight
};

struct BGFlashData {
    bool flashing;
    FlashState state;
    float timer;
    bool useLbg;
};

enum class RespawnEffectState {
    RespawnEffectNone,
    RespawnEffectShowPlayer,
    RespawnEffectHidePlayer
};

struct RespawnEffectData {
    bool active;
    bool hidePlayer;
    RespawnEffectState state;
    float timer;
    int remaining;
};

struct StateLevelData {
    int attempts;
    int jumps;
    float timeStart;
    float timeEnd;
    int maxNormal;
    int maxPractice;
    bool coin1;
    bool coin2;
    bool coin3;
};

struct ShakeData {
    bool active;
    float timer;
    float strength;
};

struct GamePlayState {
    float unmodCamX;
    float unmodCamY;

    float cameraX;
    float cameraY;

    float oldCameraX;
    float oldCameraY;

    float cameraIntendedY;

    float cameraXMiddle;
    float cameraYMiddle;

    float intermediateCameraY;
    
    float cameraWallTimer;
    float cameraWallInitialY;

    float groundX;
    float groundWallInitialX;

    float backgroundX;
    float backgroundWallInitialX;
    
    Player player;
    Player player2;

    Vec2D attemptTextPos;

    unsigned char currentPlayer;

    Player oldPlayer;
    Player deathPlayer;

    StateLevelData currentData;

    bool mirroring;
    int mirrorMult;    
    float mirrorTimer;

    float originalMirrorFactor;
    float intendedMirrorFactor;

    float mirrorSpeedFactor;
    float mirrorFactor;

    float groundY;
    float ceilingY;
    float groundYGfx;

    float dualPortalY;
    unsigned char speed;

    bool endWallAnimPlaying;
    
    bool noclip;
    int hitboxDisplay;
    bool dual;
    bool dead;

    bool profiling;
    bool practiceMode;

    float levelProgress;

    float timer;
    float deathTimer;

    bool hitboxEnabledWhenDead;
    bool pendingAutoCheckpoint;

    bool showPlayer;

    bool onlineLevel;

    bool customLevel;
    char customLevelPath[256];

    BGFlashData flashData;
    RespawnEffectData respawnEffectData;
    ShakeData shakeData;

    int lastHitboxTrail;
    PlayerHitboxTrail hitboxTrailPlayers[2][HITBOX_TRAIL_SIZE];
    
    int p1TrailPos[2];
    P1Trail p1TrailData[2][P1_TRAIL_LENGTH];

    KeyInput input;
    KeyInput inputP2;
    KeyInput oldInput;
    KeyInput oldInputP2;

    DeathReason deathReason;
};

#define CEILING_INVUL_TIME 0.1f
#define GAMEMODE_INVUL_TIME 0.1f

extern GamePlayState state;

void setIntendedCeiling();
void setGamemode(Player *player, int gamemode);
void setMini(Player *player, bool mini);
void firstLoadInitVariables();
void initVariables();
void runCamera();

void handleDeath(Player *player, bool pauseSong);

void startBgFlash();
void handleBgFlash();
void clearBgFlash();

void startRespawnEffect();
void handleRespawnEffect();
void clearRespawnEffect();

void initPlayer(Player *player);
void clearP1Trail();
void initState();
void initTrails(int trail);
void initWaveTrails();

void playLevelSong(float seek);
void playPracticeSong();
void playMenuSong();
void updateAttemptTextPos();

bool isCoinCollected(int obj);

void killPlayer(DeathReason reason);

void setCameraX(float x);
void setCameraY(float y);

void initShake();
void startShake(float duration, float strength);
void handleShake(float delta);

extern bool disableHitboxesAfterRespawn;

}
