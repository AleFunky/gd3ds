#pragma once
#include <citro2d.h>
#include "level_loading.hpp"
#include "triggers.hpp"
#include "utils/c2d_internal.h"
#include "animations.hpp"

#define FADING_OBJ_PADDING 45
#define FADING_OBJ_WIDTH 180

#define FADE_WIDTH 75

#define BACKGROUND_SCALE 0.9f

#define GROUND_SIZE 128
#define LINE_WIDTH 444
#define LINE_HEIGHT 2

#define MAX_SPRITES   8192

#define SCREEN_HEIGHT_AREA (320.f)
#define SCREEN_WIDTH_AREA ((SCREEN_HEIGHT_AREA / SCREEN_HEIGHT) * SCREEN_WIDTH)
#define SCREEN_WIDTH_AREA_BOT ((SCREEN_HEIGHT_AREA / SCREEN_HEIGHT) * SCREEN_BOT_WIDTH)
#define SCREEN_WIDTH_AREA_GAMEPLAY(swapScreens) ((swapScreens) ? SCREEN_WIDTH_AREA_BOT : SCREEN_WIDTH_AREA)

#define SCALE (SCREEN_HEIGHT / SCREEN_HEIGHT_AREA)


extern int currentPulserodBallImage;

typedef enum {
    FADE_STATUS_NONE,
    FADE_STATUS_OUT,
    FADE_STATUS_IN
} FadeStatus;

typedef struct {
    C2Di_Quad quadr;
    float tcTopLeft[2], tcTopRight[2], tcBotLeft[2], tcBotRight[2];
} QuadParams;

typedef struct {
    C2D_Sprite spr;
    u32 tint;
    QuadParams params;
    float opacity;
    float rotation;
    int obj;
    short colChannel;
    unsigned char colType;
    signed char zlayer;
    unsigned char layer;
    bool blending;
    bool hidden;
} SpriteObject;

typedef struct {
    SpriteObject *obj;
    uint32_t key;
} SortItem;

enum FadingEffects {
    FADE_NONE,
    FADE_SIMPLE,
    FADE_UP,
    FADE_DOWN,
    FADE_RIGHT,
    FADE_LEFT,
    FADE_SCALE_IN,
    FADE_SCALE_OUT,
    FADE_INWARDS,
    FADE_OUTWARDS,
    FADE_CIRCLE_LEFT,
    FADE_CIRCLE_RIGHT,
    FADE_UP_SLOW_LEFT,
    FADE_DOWN_SLOW_LEFT,
    FADE_UP_SLOW_RIGHT,
    FADE_DOWN_SLOW_RIGHT,
    FADE_UP_STATIONARY,
    FADE_DOWN_STATIONARY,
    FADE_COUNT
};

typedef struct {
    C2D_Sprite parentTemplate;
    C2D_Sprite glowTemplate;
    int childCount;
    C2D_Sprite *childTemplates;
} SpriteTemplate;

void cacheAllSprites();
void freeCachedSprites();
void getFadeVars(int obj, float x, float *fadeX, float *fadeY, float *fadeScale);
float objEdgeFade(float x, int rightEdge);
float getSpecialFadingVars(int obj, float fadeVal);

extern bool p1Trail;
extern float p1TrailTimer;
extern int currentFadingEffect;

extern C2D_SpriteSheet spriteSheet;
extern C2D_SpriteSheet spriteSheet2;
extern C2D_SpriteSheet spriteSheet3;
extern C2D_SpriteSheet spriteSheet4;
extern C2D_SpriteSheet animatedSheet;
extern C2D_SpriteSheet playerEffectsSheet;
extern C2D_SpriteSheet glowSheet;
extern C2D_SpriteSheet bgSheet;
extern int loadedBgSheet;
extern C2D_SpriteSheet groundSheet;
extern C2D_SpriteSheet trailSheet;
extern C2D_SpriteSheet particleSheet;

#define PLAYER_EFFECTS_FIRE_START 0
#define PLAYER_EFFECTS_BOOM_START 12
#define PLAYER_EFFECTS_OUTLINE_START 21
#define PLAYER_EFFECTS_SPIDER_START 33
#define PLAYER_EFFECTS_FIRE_BOOST 41

extern SpriteTemplate spriteTemplates[GAME_OBJECT_COUNT];

extern const Color white;

inline float normalizeAngle(float a)
{
    while (a < 0.0f)   a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

int compareIds(const void *a, const void *b);

void createObjects();
void changeBlending(bool blending);
Color getWhiteIfBlack(Color color);
Color getP1IfBlack(Color color);
Color getP2IfBlack(Color color);
void drawObjects();
void drawAttemptText();
void drawEndWall(float delta);
void drawBackground(float x, float y);
void drawGround(float camX, float camY, float y, bool isCeiling, int screenWidth);
void updatePlayerColors();
void setPlayerColors(Color p1, Color p2, Color glow);

typedef struct {
    int animID;
    int animFrame;
    float animTimer;
    int blendFromAnimID;
    int blendFromFrame;
    float blendFromTimer;
    float blendFraction;
    float robotFireScale;
} IconParameters;

void spawnIconAt(
    int gamemode,
    int id,
    bool glow,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 p1Color,
    u32 p2Color,
    u32 glowColor,
    IconParameters params
);
void spawnP1LayerAt(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 p1Color,
    IconParameters params
);

void spawnGlowLayerAt(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 glowColor,
    IconParameters params
);

Color getColorAbgr8(u32 color);

const SlotFrames* findSlotFrames(const GameObject* obj, int slot);
size_t getChildGroup(const GameObject* obj, size_t childIndex);
const Animation* getAnimationForObject(int id);

void handleMirrorTransition();

void drawPlayerEffects();
void drawPostPlayerEffects();
void drawPlayerGraphics();

void addDashBoom(float x, float y, float rotation);
void addGravityDashBoom(float x, float y, float rotation);
void addSpiderDashEffect(float x, float fromY, float toY, bool upsideDown, Color color);
void updateDashBooms(float delta);
void drawDashBooms(void);
int getCoinTexture(int tex, int ticks);
C2D_SpriteSheet getSpriteSheetEx(int index, int *relIndex);

void updateTouchEffect(float delta);
void drawTouchEffect();

void updateBottomParticles(float delta);
void drawBottomParticles();
