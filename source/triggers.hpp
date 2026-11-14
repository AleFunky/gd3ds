#pragma once
#include <3ds.h>
#include <stdbool.h>
#include "level_loading.hpp"

#define TRIGGER_BUFFER_SIZE 2048

namespace gd3ds
{

enum class ColorChannelIDs {
    None,
    Col1,
    Col2,
    Col3,
    Col4,
    ChannelNormalEnd,
    ChannelSpecialStart = 1000,
    ChannelBg = static_cast<int>(ColorChannelIDs::ChannelSpecialStart),
    ChannelGround,
    ChannelLine,
    Channel3dl,
    ChannelObj,
    ChannelP1,
    ChannelP2,
    ChannelLbg,
    ChannelLbgNolerp,
    ChannelGround2,
    ChannelBlack,
    ChannelWhite,
    ChannelLighter,
    ChannelBlueGlow,
    ChannelPinkGlow,
    ChannelInvisibleGlow,
    ChannelWhiteGlow,
    ChannelObjBlending,
    ChannelYellowGlowInternal,
    ColChannelLast,
    ColChannelNum = 1024,
};

extern int alphaTriggerCount;
extern int moveTriggerCount;
extern int spawnTriggerCount;
extern int pulseTriggerCount;
extern int rotateTriggerCount;
extern int followPlayerYTriggerCount;
extern int rotateTriggerCapacity;

struct ColorChannel {
    Color color;
    Color nonPulseColor;
    float alpha;
    bool blending;
    HSV hsv;
    int copyColorID;
    int numPulses;
};

struct ColTriggerBuffer {
    bool active;
    Color oldColor;
    Color newColor;
    float oldAlpha;
    float newAlpha;
    float seconds;
    float timeRun;
    int copiedColorID;
    HSV copiedHSV;
    int sourceObj;
};

struct AlphaTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    int targetGroup;
    int sourceObj;
    float oldAlpha;
    float newAlpha;
    float seconds;
    float timeRun;
};

struct MoveTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    int targetGroup;
    int sourceObj;   // runtime identity of the trigger object
    float offsetX;
    float offsetY;
    int easing;
    bool lockToPlayerX;
    bool lockToPlayerY;
    float moveLastX;
    float moveLastY;
    float seconds;
    float timeRun;
};

struct CountTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    bool activateGroup;
    bool multiActivate;
    int targetGroup;
    int itemID;
    int targetCount;
    int previousValue;
    int sourceObj;
};

struct TouchTriggerBuffer {
    bool applied;
    unsigned char heldMask;
    int sourceObj;
};

struct ItemCount {
    int id;
    int value;
};

extern CountTriggerBuffer *countTriggerBuffer;
extern int countTriggerCount;
extern int countTriggerCapacity;
extern TouchTriggerBuffer *touchTriggerBuffer;
extern int touchTriggerCount;
extern int touchTriggerCapacity;
extern ItemCount *itemCounterBuffer;
extern int itemCounterCount;
extern int itemCounterCapacity;

struct FollowPlayerYTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    int targetGroup;
    int sourceObj;
    float speed;
    float delay;
    float offset;
    float maxSpeed;
    float groupY;
};

struct FollowTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    int targetGroup;
    int followGroup;
    int sourceObj;
    int refObj;
    float xMod;
    float yMod;
    float seconds;
    float timeRun;
    float lastX;
    float lastY;
};

struct SpawnTriggerBuffer {
    bool active;
    bool queued;
    bool restoredFromCheckpoint;
    int targetGroup;
    int sourceObj;
    float seconds;
    float timeRun;
};

#define PULSE_TARGET_CHANNEL 0
#define PULSE_TARGET_GROUP 1
#define PULSE_MODE_COLOR 0
#define PULSE_MODE_HSV 1
#define INSTANT_COUNT_MODE_EQUALS 0
#define INSTANT_COUNT_MODE_LARGER 1
#define INSTANT_COUNT_MODE_SMALLER 2

struct PulseTriggerBuffer {
    bool active;
    Color color;
    int targetColorID;
    float fadeIn;
    float hold;
    float fadeOut;
    int pulseMode;
    int copiedColorID;
    HSV copiedHSV;
    int targetGroup;
    int pulseTargetType;
    bool mainOnly;
    bool detailOnly;
    int pulseIndex;
    float seconds;
    float timeRun;

    int sourceObj;
    bool restoredFromCheckpoint;
};

struct RotateTrigger {
    bool spawnTriggered;
    bool multiTriggered;
    float trigDuration;
    int targetGroup;
    int centerGroup;
    float degrees;
    int times360;
    bool lockObjectRotation;
    int rotateEasing;
    float easingRate;
};

struct RotateTargetState {
    int obj;
};

struct RotateTriggerBuffer {
    bool active;
    bool restoredFromCheckpoint;
    bool useCenter;
    int centerObj;
    int sourceObj;
    float totalDegrees;
    bool lockObjectRotation;
    int easingType;
    float easingRate;
    float seconds;
    float timeRun;
    int targetGroup;
    int targetCount;
    RotateTargetState *targets;
};

void freeTriggerBuffers();
void restorePulseTrigger(int obj, float timeRun);

extern ColorChannel channels[static_cast<int>(ColorChannelIDs::ColChannelNum)];
extern ColTriggerBuffer colTriggerBuffer[static_cast<int>(ColorChannelIDs::ColChannelNum)];
extern AlphaTriggerBuffer *alphaTriggerBuffer;
extern MoveTriggerBuffer *moveTriggerBuffer;
extern SpawnTriggerBuffer *spawnTriggerBuffer;
extern PulseTriggerBuffer *pulseTriggerBuffer;
extern RotateTriggerBuffer *rotateTriggerBuffer;
extern FollowPlayerYTriggerBuffer *followPlayerYTriggerBuffer;
extern FollowTriggerBuffer *followTriggerBuffer;
extern int followTriggerCount;
extern int followTriggerCapacity;
RotateTrigger *getRotateTrigger(int obj);
extern float moveLockPlayerXDelta;
extern float moveLockPlayerYDelta;
extern float moveStepDt;

extern Color p1Color;
extern Color p2Color;
extern Color glowColor;

extern float triggerDt;

#define BG_TRIGGER 29
#define GROUND_TRIGGER 30
#define LINE_TRIGGER 104
#define V2_0_LINE_TRIGGER 915
#define OBJ_TRIGGER 105
#define OBJ_2_TRIGGER 221
#define COL2_TRIGGER 717
#define COL3_TRIGGER 718
#define COL4_TRIGGER 743
#define THREEDL_TRIGGER 744
#define COL_TRIGGER 899
#define GROUND_2_TRIGGER 900
#define MOVE_TRIGGER 901
#define COUNT_TRIGGER 1611
#define INSTANT_COUNT_TRIGGER 1811
#define ALPHA_TRIGGER 1007
#define TOGGLE_TRIGGER 1049
#define TOUCH_TRIGGER 1595
#define PULSE_TRIGGER 1006
#define SPAWN_TRIGGER 1268
#define ROTATE_TRIGGER 1346
#define FOLLOW_TRIGGER 1347
#define FOLLOW_PLAYER_Y_TRIGGER 1814
#define STOP_TRIGGER 1616
#define ON_DEATH_TRIGGER 1812

#define HIDE_PLAYER_TRIGGER 1612
#define SHOW_PLAYER_TRIGGER 1613

#define TRIGGER_FADE_SIMPLE 22
#define TRIGGER_FADE_UP 23
#define TRIGGER_FADE_DOWN 24
#define TRIGGER_FADE_RIGHT 26
#define TRIGGER_FADE_LEFT 25
#define TRIGGER_FADE_SCALE_IN 27
#define TRIGGER_FADE_SCALE_OUT 28
#define TRIGGER_FADE_INWARDS 58
#define TRIGGER_FADE_OUTWARDS 59 
#define TRIGGER_FADE_LEFT_SEMICIRCLE 56
#define TRIGGER_FADE_RIGHT_SEMICIRCLE 57

#define GET_R(color) (color & 0xff)
#define GET_G(color) ((color >> 8) & 0xff)
#define GET_B(color) ((color >> 16) & 0xff)

#define ON_DEATH_MAX 16

struct OnDeathEntry {
    int sourceObj;
    int targetGroup;
    bool activate;
    bool multi;
};

extern OnDeathEntry onDeathEntries[ON_DEATH_MAX];
extern int onDeathCount;

void runOnDeathTriggers();
void clearOnDeathTriggers();

Color hsvCombine(Color base, HSV hsv);

#define TRIGGER_AT(pool, type, id) (&((type *)(pool).data)[id])


int triggerPoolAdd(TriggerPool *pool, size_t elementSize);

float getGroupOpacities(int object);
bool objectCanBeXMoved(int obj);

void calculateLbg();
int getColChannelIndex(int channel);
int getColChannelFromIndex(int index);
void initColChannels();
void handleColChannel(int chan);
void handleColTriggers(void);
void handleCopyChannels();
void handleTriggers(void);
void uploadToAlphaBuffer(int obj);
void handleAlphaTriggers(void);
void uploadToMoveBuffer(int obj);
void handleMoveTriggers(void);
int getItemCount(int id);
void addItemCount(int id, int delta);
void uploadToCountBuffer(int obj);
void handleCountTriggers(void);
void uploadToTouchBuffer(int obj);
void handleTouchTriggers(void);
void uploadToFollowPlayerYBuffer(int obj);
void handleFollowPlayerYTriggers(void);
void uploadToFollowBuffer(int obj);
void handleFollowTriggers(void);
void handleStepTriggers(void);
void uploadToSpawnBuffer(int obj, bool fromSpawn);
void handleSpawnTriggers(void);
void toggleGroup(int group, bool activate, bool spawn);
void spawnGroup(int group);
void uploadToPulseBuffer(int obj);
void handlePulseTriggers(void);
void runTrigger(int obj, bool fromSpawn);
void uploadColorToBuffer(int channel, u32 color, float seconds);
void uploadToBuffer(int obj, int channel);
int convertOnePointNineChannel(int channel);
ColTriggerBuffer *getBuffer(int chan);

}
