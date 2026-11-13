#pragma once

#include <stdbool.h>
#include "level_loading.hpp"
#include "icons.hpp"
#include "particles/particles.hpp"

#include "trail.hpp"

#define STEPS_HZ 240
#define STEPS_DT (1.f / STEPS_HZ) // 1/240 seconds per physics step

#define FOLLOW_Y_SAMPLES 200
#define FOLLOW_Y_INTERVAL 0.01f

#define P1_TRAIL_LENGTH 10
#define P1_TRAIL_DURATION 0.45f
#define P1_TRAIL_END_SCALE 0.8f

#define BURST_PARTICLES_DURATION 0.10f

#define DRAG_PARTICLES_FLOOR_DURATION 0.1f

#define ROBOT_FIRE_MAX_AIR_TIME 1.23f
#define ROBOT_FIRE_OUT_TIME 0.15f
#define ROBOT_FIRE_MIN_SCALE 0.05f

#define BALL_SLOW_ROTATION 0.7f

#define MAX_ACTIONS 64

#define END_ANIMATION_TIME 1.f
#define END_ANIMATION_X_START (10.f * 30.f)

namespace gd3ds
{

typedef struct {
    int slopeID;
    float elapsed;
    bool snapDown;
} SlopeData;

typedef struct {
    int objectID;
    int playerFrame;
    float playerSnapDiff;
    int snappedObj;
} SnapData;

typedef struct {
    float width;
    float height;
} InternalHitbox;

typedef struct {
    int gamemode;
    float x;
    float y;
    float rot;

    float scale;
    float deltaScale;

    float opacity;

    float life;

    bool upsideDown;
    bool active;
} P1Trail;

typedef struct {
    int cube;
    int ship;
    int ball;
    int ufo;
    int wave;
    int robot;
    int spider;

    Color p1Color;
    Color p2Color;
    Color glowColor;

    bool glow;
} IconsDrawing;

typedef struct {
    float x;
    float y;
    
    float velX;
    float velY;

    float newVelY;

    float deltaY;

    float platformVelocity, lastPlatformVelocity;
    
    float gravity;

    float rotation;
    float cubeTargetRotation;

    int consecutiveJumps;
    
    float width;
    float height;

    InternalHitbox internalHitbox;

    int gamemode;
    
    int rotationDirection;

    bool onGround;
    bool onCeiling;
    bool mini;
    bool upsideDown;
    bool touchingSlope;
    bool inverseRotation;
    bool snapRotation;
    
    int potentialSlopeID;

    bool leftGround;

    float ballRotationSpeed;
    float ballSlopeRotationMult;

    float cutsceneTimer;

    int bufferingState;

    float timeSinceGround;

    float ceilingInvTime;

    float timeElapsed;

    int gravObjID;

    float burstParticleTimer;

    float cutsceneInitialPlayerX;
    float cutsceneInitialPlayerY;

    int slopeSlideCoyoteTime;
    int allowDartSlide;
    int allowHeadHit;

    int frame;

    SlopeData coyoteSlope;
    SlopeData slopeData;

    SnapData snapData;

    bool velocityOverride;
    bool velocityUncapped;
    bool holdingJumpCancelled;
    bool dashing;
    float dashSlope;
    float dashAngle;
    float dashFireTimer;

    bool flashActive;
    float flashTimer;
    float flashDuration;
    float flashDelay;

    bool spiderScaleActive;
    float spiderScaleTimer;

    bool jumped;
    bool landedFromJump;

    int lastOrbPressed;

    bool bufferUfo;

    bool justTeleported;

    float robotAirTime;
    bool robotFireVisible;
    bool robotFireOut;
    float robotFireTimer;
    float robotFireScale;
    float robotFireExitScale;
    int animID;
    int animFrame;
    float animTimer;
    int blendFromAnimID;
    int blendFromFrame;
    float blendFromTimer;
    float blendTimer;

    float coyoteFrames;

    float followY[FOLLOW_Y_SAMPLES];
    int followYIndex;
    float followYTimer;

    IconsDrawing playerIcons;

    int collidedBlock;
    int lastCollidedBlock;

    bool gravityChangedMove;

    float minVel;
    float maxVel;
} Player;

typedef struct {
    void (*func)(Player *);
} PlayerAction;

enum BufferingState {
    BUFFER_NONE,
    BUFFER_READY,
    BUFFER_END
};

enum PlayerSpeeds {
    SPEED_SLOW,
    SPEED_NORMAL,
    SPEED_FAST,
    SPEED_FASTER,
    SPEED_FASTEST,
    SPEED_COUNT
};

extern PlayerAction playerActions[2][MAX_ACTIONS];
extern int numActions[2];

extern MotionTrail *trail;
extern MotionTrail trailP1;
extern MotionTrail trailP2;

extern MotionTrail *waveTrail;
extern MotionTrail waveTrailP1;
extern MotionTrail waveTrailP2;

extern ParticleSystem dragParticles[2];
extern ParticleSystem dragParticles2[2];
extern ParticleSystem shipFireParticles[2];
extern ParticleSystem shipSecondaryParticles[2];
extern ParticleSystem secondaryParticles[2];
extern ParticleSystem burstParticles[2];
extern ParticleSystem robotFireParticles[2];
extern ParticleSystem landParticles[2];
extern ParticleSystem explosionParticles[2];
extern ParticleSystem glitterParticles;
extern ParticleSystem slowSpeedParticles;
extern ParticleSystem normalSpeedParticles;
extern ParticleSystem fastSpeedParticles;
extern ParticleSystem fasterSpeedParticles;
extern ParticleSystem fastestSpeedParticles;
extern ParticleSystem coinPickupParticles;

typedef struct KeyInput KeyInput;

extern KeyInput currInput;
extern KeyInput currOldInput;

extern const float playerSpeeds[SPEED_COUNT];
extern const float playerSpeedMults[SPEED_COUNT];

inline float getTop(Player *player)  { return player->y + player->height / 2; }
inline float getBottom(Player *player)  { return player->y - player->height / 2; }

inline float getGroundTop(Player *player)  { return player->y + (player->height / 2) + ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }
inline float getGroundBottom(Player *player)  { return player->y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }

inline float getRight(Player *player)  { return player->x + player->width / 2; }
inline float getLeft(Player *player)  { return player->x - player->width / 2; }

inline float getInternalTop(Player *player)  { return player->y + player->internalHitbox.height / 2; }
inline float getInternalBottom(Player *player)  { return player->y - player->internalHitbox.height / 2; }
inline float getInternalRight(Player *player)  { return player->x + player->internalHitbox.width / 2; }
inline float getInternalLeft(Player *player)  { return player->x - player->internalHitbox.width / 2; }

inline float gravBottom(Player *player) { return player->upsideDown ? -getTop(player) : getBottom(player); }
inline float gravTop(Player *player) { return player->upsideDown ? -getBottom(player) : getTop(player); }

inline float gravInternalBottom(Player *player) { return player->upsideDown ? -getInternalTop(player) : getInternalBottom(player); }
inline float gravInternalTop(Player *player) { return player->upsideDown ? -getInternalBottom(player) : getInternalTop(player); }

inline float grav(Player *player, float val) { return player->upsideDown ? -val : val; }

inline float objGetTop(int object)  { 
    return objects.y[object] + objects.height[object] / 2; 
}
inline float objGetBottom(int object)  { 
    return objects.y[object] - objects.height[object] / 2; 
}
inline float objGetRight(int object)  {  
    return objects.x[object] + objects.width[object] / 2; 
}
inline float objGetLeft(int object)  { 
    return objects.x[object] - objects.width[object] / 2; 
}
inline float objGravBottom(Player *player, int object) { return player->upsideDown ? -objGetTop(object) : objGetBottom(object); }
inline float objGravTop(Player *player, int object) { return player->upsideDown ? -objGetBottom(object) : objGetTop(object); }

void handlePlayer(Player *player);
void drawPlayer(Player *player);
void runPlayer(Player *player);

void playerFlash(Player *player, float duration, float delay);

void drawDashFire(Player *player);
void addDashExit(Player *player);

void drawHitbox(int obj);
void drawPlayerHitbox(Player *player);
void drawInternalHitbox(Player *player);
void drawRotatedHitbox(Player *player);
void drawHitboxTrail(int player);
void addNewHitbox(Player *player);

void setAnimation(Player *player, int robotAnimID);
void setAnimation(Player *player, int spiderAnimID);

void updateP1Trail(Player *player, int playerID);
void drawP1Trail(Player *player, int playerID);

void updateRotationDirection(Player *player);

void pushPlayerAction(void (*func)(Player *));

float convertToClosestRotation(float rotation, float angle);

bool playerGamemodeIsFlying(Player *player);
void playerNonFlyingLanding(Player *player);

float getBlendDuration(Player *player);
void setBlendTimer(Player *player, float timer);

void stopDashing(Player *player);

}