#include <citro2d.h>
#include "triggers.hpp"
#include "arena.hpp"
#include "color.h"
#include "icons.hpp"
#include "math_helpers.hpp"
#include <float.h>
#include <math.h>
#include "level_loading.hpp"
#include "main.hpp"
#include "graphics.hpp"
#include "groups.hpp"
#include "easing.hpp"
#include "objects.hpp"
#include "player/collision.hpp"

#include <stdlib.h>
#include <string.h>

#include "player/player.hpp"
#include "player/trail.hpp"
#include "state.hpp"

#define LAUNCH_CAP 1080.f

namespace gd3ds
{

Color p1Color;
Color p2Color;
Color glowColor;

float triggerDt = 0.f;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer colTriggerBuffer[COL_CHANNEL_NUM];
AlphaTriggerBuffer *alphaTriggerBuffer = NULL;
int alphaTriggerCount = 0;
int alphaTriggerCapacity = 0;

MoveTriggerBuffer *moveTriggerBuffer = NULL;
int moveTriggerCount = 0;
int moveTriggerCapacity = 0;
CountTriggerBuffer *countTriggerBuffer = NULL;
int countTriggerCount = 0;
int countTriggerCapacity = 0;
TouchTriggerBuffer *touchTriggerBuffer = NULL;
int touchTriggerCount = 0;
int touchTriggerCapacity = 0;
ItemCount *itemCounterBuffer = NULL;
int itemCounterCount = 0;
int itemCounterCapacity = 0;

SpawnTriggerBuffer *spawnTriggerBuffer = NULL;
int spawnTriggerCount = 0;
int spawnTriggerCapacity = 0;

RotateTriggerBuffer *rotateTriggerBuffer = NULL;
int rotateTriggerCount = 0;
int rotateTriggerCapacity = 0;
FollowPlayerYTriggerBuffer *followPlayerYTriggerBuffer = NULL;
int followPlayerYTriggerCount = 0;
int followPlayerYTriggerCapacity = 0;

FollowTriggerBuffer *followTriggerBuffer = NULL;
int followTriggerCount = 0;
int followTriggerCapacity = 0;

PulseTriggerBuffer *pulseTriggerBuffer = NULL;
int pulseTriggerCount = 0;
int pulseTriggerCapacity = 0;

OnDeathEntry onDeathEntries[ON_DEATH_MAX];
int onDeathCount = 0;

AlphaTriggerBuffer *getNewAlphaTrigger() {
    if (alphaTriggerCount >= alphaTriggerCapacity) {
        size_t newCapacity = alphaTriggerCapacity == 0 ? 32 : alphaTriggerCapacity + 32;

        AlphaTriggerBuffer *newList = (AlphaTriggerBuffer *)realloc(alphaTriggerBuffer, newCapacity * sizeof(AlphaTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        alphaTriggerBuffer = newList;
        alphaTriggerCapacity = newCapacity;
    }

    AlphaTriggerBuffer *buffer = &alphaTriggerBuffer[alphaTriggerCount++];
    memset(buffer, 0, sizeof(AlphaTriggerBuffer));
    return buffer;
}

MoveTriggerBuffer *getNewMoveTrigger() {
    if (moveTriggerCount >= moveTriggerCapacity) {
        size_t newCapacity = moveTriggerCapacity == 0 ? 32 : moveTriggerCapacity + 32;

        MoveTriggerBuffer *newList = (MoveTriggerBuffer *)realloc(moveTriggerBuffer, newCapacity * sizeof(MoveTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        moveTriggerBuffer = newList;
        moveTriggerCapacity = newCapacity;
    }
    MoveTriggerBuffer *buffer = &moveTriggerBuffer[moveTriggerCount++];
    memset(buffer, 0, sizeof(MoveTriggerBuffer));
    return buffer;
}

static CountTriggerBuffer *getNewCountTrigger(void) {
    if (countTriggerCount >= countTriggerCapacity) {
        size_t newCapacity = countTriggerCapacity == 0 ? 32 : countTriggerCapacity + 32;
        CountTriggerBuffer *newList = (CountTriggerBuffer *)realloc(countTriggerBuffer, newCapacity * sizeof(CountTriggerBuffer));
        if (!newList) return NULL;
        countTriggerBuffer = newList;
        countTriggerCapacity = newCapacity;
    }
    CountTriggerBuffer *buffer = &countTriggerBuffer[countTriggerCount++];
    memset(buffer, 0, sizeof(CountTriggerBuffer));
    return buffer;
}

static TouchTriggerBuffer *getNewTouchTrigger(void) {
    if (touchTriggerCount >= touchTriggerCapacity) {
        size_t newCapacity = touchTriggerCapacity == 0 ? 32 : touchTriggerCapacity + 32;
        TouchTriggerBuffer *newList = (TouchTriggerBuffer *)realloc(touchTriggerBuffer, newCapacity * sizeof(TouchTriggerBuffer));
        if (!newList) return NULL;
        touchTriggerBuffer = newList;
        touchTriggerCapacity = newCapacity;
    }
    TouchTriggerBuffer *buffer = &touchTriggerBuffer[touchTriggerCount++];
    memset(buffer, 0, sizeof(TouchTriggerBuffer));
    return buffer;
}

static ItemCount *findItemCount(int id) {
    for (int i = 0; i < itemCounterCount; i++) {
        if (itemCounterBuffer[i].id == id) return &itemCounterBuffer[i];
    }
    return NULL;
}

static ItemCount *getNewItemCount(int id) {
    if (itemCounterCount >= itemCounterCapacity) {
        size_t newCapacity = itemCounterCapacity == 0 ? 16 : itemCounterCapacity + 16;
        ItemCount *newList = (ItemCount *)realloc(itemCounterBuffer, newCapacity * sizeof(ItemCount));
        if (!newList) return NULL;
        itemCounterBuffer = newList;
        itemCounterCapacity = newCapacity;
    }
    ItemCount *slot = &itemCounterBuffer[itemCounterCount++];
    slot->id = id;
    slot->value = 0;
    return slot;
}

int getItemCount(int id) {
    ItemCount *slot = findItemCount(id);
    return slot ? slot->value : 0;
}

void addItemCount(int id, int delta) {
    if (id < 0 || id > 9999) return;
    ItemCount *slot = findItemCount(id);
    if (!slot) slot = getNewItemCount(id);
    if (slot) slot->value += delta;
}

FollowPlayerYTriggerBuffer *getNewFollowPlayerYTrigger() {
    if (followPlayerYTriggerCount >= followPlayerYTriggerCapacity) {
        size_t newCapacity = followPlayerYTriggerCapacity == 0 ? 32 : followPlayerYTriggerCapacity + 32;

        FollowPlayerYTriggerBuffer *newList = (FollowPlayerYTriggerBuffer *)realloc(followPlayerYTriggerBuffer, newCapacity * sizeof(FollowPlayerYTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        followPlayerYTriggerBuffer = newList;
        followPlayerYTriggerCapacity = newCapacity;
    }
    FollowPlayerYTriggerBuffer *buffer = &followPlayerYTriggerBuffer[followPlayerYTriggerCount++];
    memset(buffer, 0, sizeof(FollowPlayerYTriggerBuffer));
    return buffer;
}

FollowTriggerBuffer *getNewFollowTrigger() {
    if (followTriggerCount >= followTriggerCapacity) {
        size_t newCapacity = followTriggerCapacity == 0 ? 32 : followTriggerCapacity + 32;

        FollowTriggerBuffer *newList = (FollowTriggerBuffer *)realloc(followTriggerBuffer, newCapacity * sizeof(FollowTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        followTriggerBuffer = newList;
        followTriggerCapacity = newCapacity;
    }
    FollowTriggerBuffer *buffer = &followTriggerBuffer[followTriggerCount++];
    memset(buffer, 0, sizeof(FollowTriggerBuffer));
    return buffer;
}

SpawnTriggerBuffer *getNewSpawnTrigger() {
    if (spawnTriggerCount >= spawnTriggerCapacity) {
        size_t newCapacity = spawnTriggerCapacity == 0 ? 32 : spawnTriggerCapacity + 32;

        SpawnTriggerBuffer *newList = (SpawnTriggerBuffer *)realloc(spawnTriggerBuffer, newCapacity * sizeof(SpawnTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        spawnTriggerBuffer = newList;
        spawnTriggerCapacity = newCapacity;
    }
    SpawnTriggerBuffer *buffer = &spawnTriggerBuffer[spawnTriggerCount++];
    memset(buffer, 0, sizeof(SpawnTriggerBuffer));
    return buffer;
}

RotateTriggerBuffer *getNewRotateTrigger() {
    if (rotateTriggerCount >= rotateTriggerCapacity) {
        size_t newCapacity = rotateTriggerCapacity == 0 ? 32 : rotateTriggerCapacity + 32;

        RotateTriggerBuffer *newList = (RotateTriggerBuffer *)realloc(rotateTriggerBuffer, newCapacity * sizeof(RotateTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        rotateTriggerBuffer = newList;
        rotateTriggerCapacity = newCapacity;
    }

    RotateTriggerBuffer *buffer = &rotateTriggerBuffer[rotateTriggerCount++];
    memset(buffer, 0, sizeof(RotateTriggerBuffer));
    return buffer;
}

PulseTriggerBuffer *getNewPulseTrigger() {
    if (pulseTriggerCount >= pulseTriggerCapacity) {
        size_t newCapacity = pulseTriggerCapacity == 0 ? 32 : pulseTriggerCapacity + 32;

        PulseTriggerBuffer *newList = (PulseTriggerBuffer *)realloc(pulseTriggerBuffer, newCapacity * sizeof(PulseTriggerBuffer));
        if (!newList) {
            return NULL;
        }

        pulseTriggerBuffer = newList;
        pulseTriggerCapacity = newCapacity;
    }
    PulseTriggerBuffer *buffer = &pulseTriggerBuffer[pulseTriggerCount++];
    memset(buffer, 0, sizeof(PulseTriggerBuffer));
    return buffer;
}

typedef struct {
    bool saved;
    float rotation;
    float visualRotation;
} RotateInitialRotation;

static RotateInitialRotation *rotate_initial_rotations = NULL;
static int rotateInitialRotationsCapacity = 0;

static void rememberRotateInitialRotation(int obj) {
    if (obj < 0) return;
    if (obj >= rotateInitialRotationsCapacity) {
        int newCapacity = rotateInitialRotationsCapacity ? rotateInitialRotationsCapacity : 64;
        while (newCapacity <= obj) newCapacity *= 2;
        RotateInitialRotation *newList = (RotateInitialRotation *)realloc(rotate_initial_rotations, sizeof(RotateInitialRotation) * newCapacity);
        if (!newList) return;
        memset(newList + rotateInitialRotationsCapacity, 0, sizeof(RotateInitialRotation) * (newCapacity - rotateInitialRotationsCapacity));
        rotate_initial_rotations = newList;
        rotateInitialRotationsCapacity = newCapacity;
    }
    if (rotate_initial_rotations[obj].saved) return;
    rotate_initial_rotations[obj].saved = true;
    rotate_initial_rotations[obj].rotation = objects.rotation[obj];
    rotate_initial_rotations[obj].visualRotation = objects.visualRotation[obj];
}

static void restoreRotateInitialRotations(void) {
    if (!rotate_initial_rotations) return;
    if (!objects.rotation || !objects.visualRotation) {
        free(rotate_initial_rotations);
        rotate_initial_rotations = NULL;
        rotateInitialRotationsCapacity = 0;
        return;
    }
    int limit = objects.count < rotateInitialRotationsCapacity ? objects.count : rotateInitialRotationsCapacity;
    for (int i = 0; i < limit; i++) {
        if (!rotate_initial_rotations[i].saved) continue;
        objects.rotation[i] = rotate_initial_rotations[i].rotation;
        objects.visualRotation[i] = rotate_initial_rotations[i].visualRotation;
    }
}

void freeTriggerBuffers() {
    restoreRotateInitialRotations();
    free(alphaTriggerBuffer);
    free(moveTriggerBuffer);
    free(countTriggerBuffer);
    free(touchTriggerBuffer);
    free(itemCounterBuffer);
    free(spawnTriggerBuffer);
    free(followPlayerYTriggerBuffer);
    free(followTriggerBuffer);

    if (rotateTriggerBuffer) {
        for (int i = 0; i < rotateTriggerCount; i++) {
            if (rotateTriggerBuffer[i].targets) { free(rotateTriggerBuffer[i].targets); rotateTriggerBuffer[i].targets = NULL; }
        }
    }
    free(rotateTriggerBuffer);

    free(pulseTriggerBuffer);

    alphaTriggerBuffer = NULL;
    moveTriggerBuffer = NULL;
    countTriggerBuffer = NULL;
    touchTriggerBuffer = NULL;
    itemCounterBuffer = NULL;
    spawnTriggerBuffer = NULL;
    followPlayerYTriggerBuffer = NULL;
    followTriggerBuffer = NULL;
    rotateTriggerBuffer = NULL;
    pulseTriggerBuffer = NULL;

    alphaTriggerCount = 0;
    alphaTriggerCapacity = 0;

    moveTriggerCount = 0;
    moveStepDt = 0.f;
    moveTriggerCapacity = 0;

    countTriggerCount = 0;
    countTriggerCapacity = 0;
    touchTriggerCount = 0;
    touchTriggerCapacity = 0;
    itemCounterCount = 0;
    itemCounterCapacity = 0;

    followPlayerYTriggerCount = 0;
    followPlayerYTriggerCapacity = 0;

    followTriggerCount = 0;
    followTriggerCapacity = 0;

    spawnTriggerCount = 0;
    spawnTriggerCapacity = 0;

    rotateTriggerCount = 0;
    rotateTriggerCapacity = 0;

    pulseTriggerCount = 0;
    pulseTriggerCapacity = 0;
}

float moveLockPlayerXDelta = 0.0f;
float moveLockPlayerYDelta = 0.0f;
float moveStepDt = 0.0f;

static void playerFollowYStep(Player *player, float dt) {
    if (player->followYIndex == 0 && player->followYTimer == 0.f) {
        for (int i = 0; i < FOLLOW_Y_SAMPLES; i++) player->followY[i] = player->y;
    }
    player->followYTimer += dt;
    if (player->followYTimer >= FOLLOW_Y_INTERVAL) {
        player->followYTimer -= FOLLOW_Y_INTERVAL;
        player->followYIndex++;
    }
    player->followY[player->followYIndex % FOLLOW_Y_SAMPLES] = player->y;
}

static float playerFollowYOld(Player *player, float timeAgo) {
    if (timeAgo <= 0.f) return player->y;
    int index = (int)floorf(timeAgo / FOLLOW_Y_INTERVAL);
    if (index > FOLLOW_Y_SAMPLES - 1) index = FOLLOW_Y_SAMPLES - 1;
    int i = (player->followYIndex - index) % FOLLOW_Y_SAMPLES;
    if (i < 0) i += FOLLOW_Y_SAMPLES;
    return player->followY[i];
}

static int toggleSpawnBuffer[8];
static int toggleSpawnCount = 0;
static bool toggleSpawnActive = false;

float getGroupOpacities(int object) {
    float groupOpacity = 1.f;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (!group) break;

        GroupNode *p = getGroup(group);
        if (p) groupOpacity *= p->alpha;
    }
    return groupOpacity;
}

bool getGroupToggles(int object) {
    bool toggled = false;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (!group) break;

        GroupNode *p = getGroup(group);
        if (p) toggled = toggled || p->toggled;
        if (toggled) break;
    }
    return toggled;
}

// Convert channel id to buffer index
int getColChannelIndex(int channel) {
    if (channel < 0 || channel >= COL_CHANNEL_NUM) {
        return 0;
    }
    return channel;
}

// Convert from buffer index to channel id
int getColChannelFromIndex(int index) {
    if (index < 0 || index >= COL_CHANNEL_NUM) {
        return 0;
    }
    return index;
}

int convertOnePointNineChannel(int channel) {
    switch (channel) {
        case 1: return CHANNEL_P1;
        case 2: return CHANNEL_P2;
        case 3: return COL_1;
        case 4: return COL_2;
        case 5: return CHANNEL_LBG;
        case 6: return COL_3;
        case 7: return COL_4;
        case 8: return CHANNEL_3DL;
    }

    return channel;
}

Color hsvCombine(Color base, HSV hsv) {
    if (hsv.h == 0 && hsv.s == 0 && hsv.v == 0)
        return base;

    float r = base.r / 255.f;
    float g = base.g / 255.f;
    float b = base.b / 255.f;

    float cmax = fmaxf(fmaxf(r, g), b);
    float cmin = fminf(fminf(r, g), b);
    float delta = cmax - cmin;

    float hue = 0.f;
    if (delta != 0.f) {
        if (cmax == r) hue = 60.f * fmodf((g - b) / delta, 6.f);
        else if (cmax == g) hue = 60.f * ((b - r) / delta + 2.f);
        else hue = 60.f * ((r - g) / delta + 4.f);
    }
    if (hue < 0.f) hue += 360.f;

    float sat = (cmax == 0.f) ? 0.f : delta / cmax;
    float val = cmax;

    hue += hsv.h;
    if (hsv.sChecked) sat += hsv.s;
    else sat *= hsv.s;
    if (hsv.vChecked) val += hsv.v;
    else val *= hsv.v;

    while (hue < 0.f) hue += 360.f;
    while (hue >= 360.f) hue -= 360.f;
    sat = fminf(fmaxf(sat, 0.f), 1.f);
    val = fminf(fmaxf(val, 0.f), 1.f);

    if (sat == 0.f) {
        unsigned char v = (unsigned char)(val * 255.f);
        Color c = { v, v, v };
        return c;
    }

    float h = hue / 60.f;
    float hi = floorf(h);
    float f = h - hi;
    float p = val * (1.f - sat);
    float q = val * (1.f - sat * f);
    float t = val * (1.f - sat * (1.f - f));

    float rr, gg, bb;
    switch ((int)hi) {
        case 0: case 6: rr = val; gg = t;   bb = p; break;
        case 1:         rr = q;   gg = val; bb = p; break;
        case 2:         rr = p;   gg = val; bb = t; break;
        case 3:         rr = p;   gg = q;   bb = val; break;
        case 4:         rr = t;   gg = p;   bb = val; break;
        default:        rr = val; gg = p;   bb = q; break;
    }

    Color result = {
        (unsigned char)(fminf(rr, 1.f) * 255.f),
        (unsigned char)(fminf(gg, 1.f) * 255.f),
        (unsigned char)(fminf(bb, 1.f) * 255.f)
    };
    return result;
}

int triggerPoolAdd(TriggerPool *pool, size_t elementSize) {
    if (pool->count >= pool->capacity) {
        size_t newCapacity = pool->capacity ? pool->capacity * 2 : 8;
        void *newData = arenaRealloc(&levelArena, pool->data, pool->capacity * elementSize, newCapacity * elementSize);

        if (!newData) {
            newCapacity = pool->capacity + 8;
            newData = arenaRealloc(&levelArena, pool->data, pool->capacity * elementSize, newCapacity * elementSize);
        }

        if (!newData) return -1;

        pool->data = newData;
        pool->capacity = newCapacity;
    }
    
    memset((char *)pool->data + pool->count * elementSize, 0, elementSize);
    return pool->count++;
}

void initColChannels() {
    memset(colTriggerBuffer, 0, sizeof(colTriggerBuffer));

    channels[0].color.r = 0;
    channels[0].color.g = 0;
    channels[0].color.b = 0;
    channels[0].alpha = 1.0f;
    channels[0].blending = false;
    channels[0].copyColorID = 0;
    memset(&channels[0].hsv, 0, sizeof(HSV));

    for (size_t chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        channels[chan].color.r = 255;
        channels[chan].color.g = 255;
        channels[chan].color.b = 255;
        channels[chan].nonPulseColor = channels[chan].color;
        channels[chan].alpha = 1.0f;
        channels[chan].blending = false;
        channels[chan].copyColorID = 0;
        channels[chan].numPulses = 0;
        memset(&channels[chan].hsv, 0, sizeof(HSV));
        colTriggerBuffer[chan].active = false;
    }

    int bg = getColChannelIndex(CHANNEL_BG);
    channels[bg].color.r = 0;
    channels[bg].color.g = 5;
    channels[bg].color.b = 100;
    
    int ground = getColChannelIndex(CHANNEL_GROUND);
    channels[ground].color.r = 40;
    channels[ground].color.g = 125;
    channels[ground].color.b = 255;
       
    int line = getColChannelIndex(CHANNEL_LINE);
    channels[line].color.r = 255;
    channels[line].color.g = 255;
    channels[line].color.b = 255;
    channels[line].blending = true;
    
    int obj = getColChannelIndex(CHANNEL_OBJ);
    channels[obj].color.r = 255;
    channels[obj].color.g = 255;
    channels[obj].color.b = 255;
    
    int objBlend = getColChannelIndex(CHANNEL_OBJ_BLENDING);
    channels[objBlend].color.r = 255;
    channels[objBlend].color.g = 255;
    channels[objBlend].color.b = 255;
    channels[objBlend].blending = true;
    
    int threedl = getColChannelIndex(CHANNEL_3DL);
    channels[threedl].color.r = 255;
    channels[threedl].color.g = 255;
    channels[threedl].color.b = 255;
    
    int chnP1 = getColChannelIndex(CHANNEL_P1);
    channels[chnP1].color = getP2IfBlack(p1Color);
    channels[chnP1].blending = true;
        
    int chnP2 = getColChannelIndex(CHANNEL_P2);
    channels[chnP2].color = getP1IfBlack(p2Color);
    channels[chnP2].blending = true;
    
    int lbg = getColChannelIndex(CHANNEL_LBG);
    channels[lbg].color.r = 255;
    channels[lbg].color.g = 255;
    channels[lbg].color.b = 255;
    channels[lbg].blending = true;

    
    int blackChn = getColChannelIndex(CHANNEL_BLACK);
    channels[blackChn].color.r = 0;
    channels[blackChn].color.g = 0;
    channels[blackChn].color.b = 0;
    channels[blackChn].blending = false;

    int whiteChn = getColChannelIndex(CHANNEL_WHITE);
    channels[whiteChn].color.r = 255;
    channels[whiteChn].color.g = 255;
    channels[whiteChn].color.b = 255;
    channels[whiteChn].blending = false;

    int yellowGlow = getColChannelIndex(CHANNEL_YELLOW_GLOW_INTERNAL);
    channels[yellowGlow].color.r = 255;
    channels[yellowGlow].color.g = 255;
    channels[yellowGlow].color.b = 0;
    channels[yellowGlow].blending = true;

    int blueGlow = getColChannelIndex(CHANNEL_BLUE_GLOW);
    channels[blueGlow].color.r = 0;
    channels[blueGlow].color.g = 255;
    channels[blueGlow].color.b = 255;
    channels[blueGlow].blending = true;

    int pinkGlow = getColChannelIndex(CHANNEL_PINK_GLOW);
    channels[pinkGlow].color.r = 255;
    channels[pinkGlow].color.g = 0;
    channels[pinkGlow].color.b = 255;
    channels[pinkGlow].blending = true;
    
    int invisGlow = getColChannelIndex(CHANNEL_INVISIBLE_GLOW);
    channels[invisGlow].color.r = 255;
    channels[invisGlow].color.g = 255;
    channels[invisGlow].color.b = 255;
    channels[invisGlow].blending = true;
    
    int whiteGlow = getColChannelIndex(CHANNEL_WHITE_GLOW);
    channels[whiteGlow].color.r = 255;
    channels[whiteGlow].color.g = 255;
    channels[whiteGlow].color.b = 255;
    channels[whiteGlow].blending = true;

    for (int i = 0; i < COL_CHANNEL_NUM; i++)
        channels[i].nonPulseColor = channels[i].color;
}

void handleColChannel(int chan) {
    int channel = getColChannelIndex(chan);

    if (channel == getColChannelIndex(CHANNEL_BLACK)) return;

    ColTriggerBuffer *buffer = &colTriggerBuffer[channel];

    if (buffer->active) {
        Color lerpedColor;
        float lerpedAlpha;
        Color colorToLerp = buffer->newColor;

        if (buffer->copiedColorID > 0) {
            int src = getColChannelIndex(buffer->copiedColorID);
            colorToLerp = channels[src].color;
            buffer->newAlpha = channels[src].alpha;
        }

        if (buffer->seconds > 0) {
            float multiplier = buffer->timeRun / buffer->seconds;
            lerpedColor = colorLerp(buffer->oldColor, colorToLerp, multiplier);
            lerpedAlpha = (buffer->newAlpha - buffer->oldAlpha) * multiplier + buffer->oldAlpha;
        } else {
            lerpedColor = colorToLerp;
            lerpedAlpha = buffer->newAlpha;
        }

        channels[channel].color = lerpedColor;
        channels[channel].nonPulseColor = lerpedColor;
        channels[channel].alpha = lerpedAlpha;

        buffer->timeRun += triggerDt;

        if (buffer->timeRun > buffer->seconds) {
            buffer->active = false;
            channels[channel].color = colorToLerp;
            channels[channel].nonPulseColor = colorToLerp;
            channels[channel].alpha = buffer->newAlpha;
        }
    }
}

ColTriggerBuffer *getBuffer(int chan) {
    return &colTriggerBuffer[getColChannelIndex(chan)];
}

void handleColTriggers(void) {
    for (int chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        handleColChannel(getColChannelFromIndex(chan));
    }
}

void handleCopyChannels() {
    for (int chan = 0; chan < COL_CHANNEL_NUM; chan++) {
        if (chan == getColChannelIndex(CHANNEL_BLACK)) continue;
        int copyID = channels[chan].copyColorID;
        if (copyID > 0) {
            int src = getColChannelIndex(copyID);
            Color color = channels[src].color;
            channels[chan].color = hsvCombine(color, channels[chan].hsv);
            channels[chan].nonPulseColor = channels[chan].color;
        }
    }
}

void uploadToAlphaBuffer(int obj) {
    AlphaTrigger *trigger = getAlphaTrigger(obj);
    int targetGroup = trigger->targetGroup;
    GroupNode *p = getGroup(targetGroup);
    if (!p) return;

    if (trigger->trigDuration == 0) {
        float alpha = trigger->triggerOpacity;
        p->alpha = alpha;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.alphaTriggerOpacity[object] = getGroupOpacities(object);
        }
        return;
    }

    int slot = -1;
    for (int i = 0; i < alphaTriggerCount; i++) {
        if (alphaTriggerBuffer[i].active && alphaTriggerBuffer[i].targetGroup == targetGroup) {
            slot = i;
            break;
        }
    }

    for (int i = 0; i < alphaTriggerCount; i++) {
        if (alphaTriggerBuffer[i].active &&
            alphaTriggerBuffer[i].targetGroup == targetGroup &&
            alphaTriggerBuffer[i].sourceObj == obj &&
            alphaTriggerBuffer[i].restoredFromCheckpoint) {
            alphaTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    AlphaTriggerBuffer *buffer;

    if (slot >= 0) {
        buffer = &alphaTriggerBuffer[slot];
    } else {
        buffer = getNewAlphaTrigger();
    }
    if (!buffer) return;

    buffer->targetGroup = targetGroup;
    buffer->sourceObj = obj;
    buffer->newAlpha = trigger->triggerOpacity;
    buffer->oldAlpha = p->alpha;
    buffer->seconds = trigger->trigDuration;
    buffer->timeRun = 0;
    buffer->active = true;
}

void handleAlphaTriggers(void) {
    for (int slot = 0; slot < alphaTriggerCount; slot++) {
        AlphaTriggerBuffer *buffer = &alphaTriggerBuffer[slot];
        if (!buffer->active) continue;

        float multiplier = buffer->timeRun / buffer->seconds;
        float lerped = buffer->oldAlpha + (buffer->newAlpha - buffer->oldAlpha) * multiplier;

        GroupNode *p = getGroup(buffer->targetGroup);
        if (p) {
            p->alpha = lerped;
            for (int i = 0; i < p->count; i++) {
                int object = p->objects[i];
                objects.alphaTriggerOpacity[object] = getGroupOpacities(object);
            }
        }

        buffer->timeRun += triggerDt;
        if (buffer->timeRun >= buffer->seconds) {
            if (p) {
                p->alpha = buffer->newAlpha;
                for (int i = 0; i < p->count; i++) {
                    int object = p->objects[i];
                    objects.alphaTriggerOpacity[object] = getGroupOpacities(object);
                }
            }
            buffer->active = false;
            
            for (int j = slot; j < alphaTriggerCount - 1; j++) {
                // Shift buffer array
                alphaTriggerBuffer[j] = alphaTriggerBuffer[j + 1];
            }
            
            // Make the game run the pulse moved into the current slot
            slot--;
            alphaTriggerCount--;
        }
    }
}

static int convertEase(int easing) {
    switch (easing) {
        case 0: return EASE_LINEAR;
        case 1: return EASE_IN_OUT;
        case 2: return EASE_IN;
        case 3: return EASE_OUT;
        case 4: return ELASTIC_IN_OUT;
        case 5: return ELASTIC_IN;
        case 6: return ELASTIC_OUT;
        case 7: return BOUNCE_IN_OUT;
        case 8: return BOUNCE_IN;
        case 9: return BOUNCE_OUT;
        case 10: return EXPO_IN_OUT;
        case 11: return EXPO_IN;
        case 12: return EXPO_OUT;
        case 13: return SINE_IN_OUT;
        case 14: return SINE_IN;
        case 15: return SINE_OUT;
        case 16: return BACK_IN_OUT;
        case 17: return BACK_IN;
        case 18: return BACK_OUT;
    }
    return EASE_LINEAR;
}

void uploadToMoveBuffer(int obj) {
    MoveTrigger *trigger = getMoveTrigger(obj);
    int targetGroup = trigger->targetGroup;
    if (!getGroup(targetGroup)) return;

    for (int i = 0; i < moveTriggerCount; i++) {
        if (moveTriggerBuffer[i].active &&
            moveTriggerBuffer[i].sourceObj == obj &&
            moveTriggerBuffer[i].restoredFromCheckpoint) {
            moveTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    float offsetX = trigger->moveOffsetX;
    float offsetY = trigger->moveOffsetY;

    if (trigger->targetMode) {
        GroupNode *dest = getGroup(trigger->targetGroup2);
        GroupNode *center = getGroup(trigger->centerGroup ? trigger->centerGroup : targetGroup);
        if (!dest || dest->count <= 0 || !center || center->count <= 0) return;

        offsetX = objects.x[dest->objects[0]] - objects.x[center->objects[0]];
        offsetY = objects.y[dest->objects[0]] - objects.y[center->objects[0]];
    }

    MoveTriggerBuffer *buffer = getNewMoveTrigger();
    if (!buffer) return;

    buffer->targetGroup = targetGroup;
    buffer->sourceObj = obj;
    buffer->offsetX = offsetX;
    buffer->offsetY = offsetY;
    buffer->easing = trigger->moveEasing;
    buffer->lockToPlayerX = trigger->lockToPlayerX;
    buffer->lockToPlayerY = trigger->lockToPlayerY;
    buffer->seconds = trigger->trigDuration;
    buffer->moveLastX = 0;
    buffer->moveLastY = 0;
    buffer->timeRun = 0;
    buffer->active = true;
}

void uploadToFollowPlayerYBuffer(int obj) {
    FollowPlayerYTrigger *trigger = getFollowPlayerYTrigger(obj);
    GroupNode *group = getGroup(trigger->targetGroup);
    if (!group || group->count <= 0) return;

    for (int i = 0; i < followPlayerYTriggerCount; i++) {
        if (followPlayerYTriggerBuffer[i].active &&
            followPlayerYTriggerBuffer[i].sourceObj == obj &&
            followPlayerYTriggerBuffer[i].restoredFromCheckpoint) {
            followPlayerYTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    FollowPlayerYTriggerBuffer *buffer = getNewFollowPlayerYTrigger();
    if (!buffer) return;

    buffer->targetGroup = trigger->targetGroup;
    buffer->sourceObj = obj;
    buffer->speed = trigger->followSpeed > 0.f ? trigger->followSpeed : 1.f;
    buffer->delay = trigger->followDelay;
    buffer->offset = trigger->followOffset;
    buffer->maxSpeed = trigger->followMaxSpeed;
    buffer->groupY = objects.y[group->objects[0]];
    buffer->active = true;
}

void handleMoveTriggers(void) {
    if (triggerDt <= 0.f) return;
    moveStepDt = triggerDt;
    
    beginGroupCombinationMoveStep();

    for (int i = 0; i< moveTriggerCount; i++) {
        if (!moveTriggerBuffer[i].active)
            addMoveToGroupCombinations(moveTriggerBuffer[i].targetGroup, 0.f, 0.f);
    }
    
    int count = 0;
    for (int i = 0; i < moveTriggerCount; i++) {
        if (moveTriggerBuffer[i].active) moveTriggerBuffer[count++] = moveTriggerBuffer[i];
    }

    moveTriggerCount = count;

    for (int slot = 0; slot < moveTriggerCount; slot++) {
        MoveTriggerBuffer *buffer = &moveTriggerBuffer[slot];
        buffer->timeRun += triggerDt;
        float t = buffer->seconds > 0.f ? easeTime((EaseTypes)convertEase(buffer->easing),
            buffer->timeRun, buffer->seconds, 2.f) : 1.f;
        float deltaX, deltaY;
        if (buffer->lockToPlayerX) deltaX = moveLockPlayerXDelta;
        else {
            float currentX = buffer->offsetX * t;
            deltaX = currentX - buffer->moveLastX;
            buffer->moveLastX = currentX;
        }
        if (buffer->lockToPlayerY) deltaY = moveLockPlayerYDelta;
        else {
            float currentY = buffer->offsetY * t;
            deltaY = currentY - buffer->moveLastY;
            buffer->moveLastY = currentY;
        }
        addMoveToGroupCombinations(buffer->targetGroup, deltaX, deltaY);
        if (buffer->timeRun >= buffer->seconds) buffer->active = false;
    }

    applyGroupCombinationMove();
}

void uploadToCountBuffer(int obj) {
    CountTrigger *trigger = getCountTrigger(obj);
    for (int i = 0; i < countTriggerCount; i++) {
        if (countTriggerBuffer[i].active &&
            countTriggerBuffer[i].sourceObj == obj &&
            countTriggerBuffer[i].restoredFromCheckpoint) {
            countTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    CountTriggerBuffer *buffer = getNewCountTrigger();
    if (!buffer) return;

    buffer->targetGroup = trigger->targetGroup;
    buffer->sourceObj = obj;
    buffer->itemID = trigger->itemID;
    buffer->targetCount = trigger->targetCount;
    buffer->activateGroup = trigger->activateGroup;
    buffer->multiActivate = trigger->multiActivate;
    buffer->previousValue = getItemCount(trigger->itemID);
    buffer->active = true;
}

void handleCountTriggers(void) {
    if (countTriggerCount == 0) return;

    int total = countTriggerCount;
    bool compact = false;
    for (int i = 0; i < total; i++) {
        CountTriggerBuffer buffer = countTriggerBuffer[i];
        if (!buffer.active) {
            compact = true;
            continue;
        }
        int value = getItemCount(buffer.itemID);
        bool fire = value == buffer.targetCount && value != buffer.previousValue;
        countTriggerBuffer[i].previousValue = value;
        if (fire) {
            if (!buffer.multiActivate) {
                countTriggerBuffer[i].active = false;
                compact = true;
            }
            toggleGroup(buffer.targetGroup, buffer.activateGroup, true);
        }
    }

    if (compact) {
        int count = 0;
        for (int i = 0; i < countTriggerCount; i++) {
            if (countTriggerBuffer[i].active) countTriggerBuffer[count++] = countTriggerBuffer[i];
        }
        countTriggerCount = count;
    }
}

void uploadToTouchBuffer(int obj) {
    for (int i = 0; i < touchTriggerCount; i++) {
        if (touchTriggerBuffer[i].sourceObj == obj) {
            return;
        }
    }

    TouchTriggerBuffer *buffer = getNewTouchTrigger();
    if (!buffer) return;

    buffer->sourceObj = obj;
}

static bool touchToggleValue(TouchTrigger *trigger) {
    switch (trigger->toggleMode) {
        case 1: return true;
        case 2: return false;
        default: {
            GroupNode *group = getGroup(trigger->targetGroup);
            return group && group->toggled;
        }
    }
}

void handleTouchTriggers(void) {
    if (state.dead) return;
    if (touchTriggerCount == 0) return;

    bool p1Pressed = state.input.pressedJump || (!state.oldInput.holdJump && state.input.holdJump);
    bool p1Released = state.oldInput.holdJump && !state.input.holdJump;
    bool p2Pressed = false;
    bool p2Released = false;

    if (state.dual) {
        p2Pressed = state.inputP2.pressedJump || (!state.oldInputP2.holdJump && state.inputP2.holdJump);
        p2Released = state.oldInputP2.holdJump && !state.inputP2.holdJump;
    }

    int total = touchTriggerCount;
    for (int i = 0; i < total; i++) {
        TouchTriggerBuffer buffer = touchTriggerBuffer[i];

        TouchTrigger *trigger = getTouchTrigger(buffer.sourceObj);
        if (!trigger) continue;

        unsigned char held = buffer.heldMask;
        unsigned char nextHeld = held;

        if (state.dual) {
            if (p1Pressed && !(held & 1)) nextHeld |= 1;
            if (p1Released && (held & 1)) nextHeld &= ~1;
            if (p2Pressed && !(held & 2)) nextHeld |= 2;
            if (p2Released && (held & 2)) nextHeld &= ~2;
        } else {
            if (p1Pressed && !(held & 1)) nextHeld |= 1;
            if (p1Released && (held & 1)) nextHeld &= ~1;
        }

        if (trigger->holdMode && held == 0) {
            if (state.input.holdJump) nextHeld |= 1;
            if (state.dual && state.inputP2.holdJump) nextHeld |= 2;
        }

        bool pressed = held == 0 && nextHeld != 0;
        bool released = held != 0 && nextHeld == 0;
        touchTriggerBuffer[i].heldMask = nextHeld;

        if (pressed) {
            bool applied = touchToggleValue(trigger);
            touchTriggerBuffer[i].applied = applied;
            toggleGroup(trigger->targetGroup, applied, true);
        } else if (released && trigger->holdMode) {
            toggleGroup(trigger->targetGroup, !buffer.applied, true);
        }
    }
}

void handleFollowPlayerYTriggers(void) {
    if (triggerDt <= 0.f) return;
    if (followPlayerYTriggerCount == 0) return;

    beginGroupCombinationMoveStep();

    for (int slot = 0; slot < followPlayerYTriggerCount; slot++) {
        FollowPlayerYTriggerBuffer *buffer = &followPlayerYTriggerBuffer[slot];
        if (!buffer->active) continue;
        float target = playerFollowYOld(&state.player, buffer->delay) + buffer->offset;
        float delta = (target - buffer->groupY) * buffer->speed;
        if (buffer->maxSpeed > 0.f) {
            float maxDelta = buffer->maxSpeed * triggerDt;
            if (delta > maxDelta) delta = maxDelta;
            else if (delta < -maxDelta) delta = -maxDelta;
        }
        buffer->groupY += delta;
        addMoveToGroupCombinations(buffer->targetGroup, 0.f, delta);
    }

    applyGroupCombinationMove();

    int count = 0;
    for (int slot = 0; slot < followPlayerYTriggerCount; slot++) {
        if (!followPlayerYTriggerBuffer[slot].active) continue;
        if (count != slot) followPlayerYTriggerBuffer[count] = followPlayerYTriggerBuffer[slot];
        count++;
    }

    followPlayerYTriggerCount = count;
}

void uploadToFollowBuffer(int obj) {
    FollowTrigger *trigger = getFollowTrigger(obj);
    if (trigger->trigDuration <= 0.f) return;
    if (trigger->targetGroup == trigger->followGroup) return;

    GroupNode *target = getGroup(trigger->targetGroup);
    GroupNode *follow = getGroup(trigger->followGroup);
    if (!target || target->count <= 0) return;
    if (!follow || follow->count <= 0) return;

    for (int i = 0; i < followTriggerCount; i++) {
        if (followTriggerBuffer[i].active &&
            followTriggerBuffer[i].sourceObj == obj &&
            followTriggerBuffer[i].restoredFromCheckpoint) {
            followTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    FollowTriggerBuffer *buffer = getNewFollowTrigger();
    if (!buffer) return;

    int refObj = follow->objects[0];

    buffer->targetGroup = trigger->targetGroup;
    buffer->followGroup = trigger->followGroup;
    buffer->sourceObj = obj;
    buffer->refObj = refObj;
    buffer->xMod = trigger->xMod;
    buffer->yMod = trigger->yMod;
    buffer->seconds = trigger->trigDuration;
    buffer->timeRun = 0.f;
    buffer->lastX = objects.x[refObj];
    buffer->lastY = objects.y[refObj];
    buffer->active = true;
}

static void compactFollowTriggers(void) {
    int count = 0;
    for (int i = 0; i < followTriggerCount; i++) {
        if (!followTriggerBuffer[i].active) continue;
        if (count != i) {
            followTriggerBuffer[count] = followTriggerBuffer[i];
        }
        count++;
    }
    followTriggerCount = count;
}

void handleFollowTriggers(void) {
    if (triggerDt <= 0.f) return;
    if (followTriggerCount == 0) return;

    beginGroupCombinationMoveStep();

    for (int slot = 0; slot < followTriggerCount; slot++) {
        FollowTriggerBuffer *buffer = &followTriggerBuffer[slot];
        if (!buffer->active) continue;

        GroupNode *follow = getGroup(buffer->followGroup);
        if (!follow || follow->count <= 0) {
            buffer->active = false;
            continue;
        }

        int refObj = buffer->refObj;
        if (refObj < 0 || refObj >= (int)objects.count) {
            buffer->active = false;
            continue;
        }

        float deltaX = (objects.x[refObj] - buffer->lastX) * buffer->xMod;
        float deltaY = (objects.y[refObj] - buffer->lastY) * buffer->yMod;
        buffer->lastX = objects.x[refObj];
        buffer->lastY = objects.y[refObj];

        if (deltaX != 0.f || deltaY != 0.f) {
            addMoveToGroupCombinations(buffer->targetGroup, deltaX, deltaY);
        }

        buffer->timeRun += triggerDt;
        if (buffer->timeRun >= buffer->seconds) {
            buffer->active = false;
        }
    }

    applyGroupCombinationMove();
    compactFollowTriggers();
}

void uploadToRotateBuffer(int obj) {
    RotateTrigger *trigger = getRotateTrigger(obj);
    GroupNode *group = getGroup(trigger->targetGroup);
    if (!group || group->count <= 0) return;

    for (int i = 0; i < rotateTriggerCount; i++) {
        if (rotateTriggerBuffer[i].active &&
            rotateTriggerBuffer[i].sourceObj == obj &&
            rotateTriggerBuffer[i].restoredFromCheckpoint) {
            rotateTriggerBuffer[i].restoredFromCheckpoint = false;
            return;
        }
    }

    RotateTriggerBuffer *buffer = getNewRotateTrigger();
    if (!buffer) return;

    buffer->centerObj = -1;
    buffer->useCenter = trigger->centerGroup != 0;
    buffer->sourceObj = obj;
    buffer->totalDegrees = trigger->degrees + (float)trigger->times360 * 360.f;
    buffer->lockObjectRotation = trigger->lockObjectRotation;
    buffer->easingType = convertEase(trigger->rotateEasing);
    buffer->easingRate = trigger->easingRate > 0.f ? trigger->easingRate : 2.f;
    buffer->seconds = trigger->trigDuration;
    buffer->timeRun = 0.f;
    buffer->targetGroup = trigger->targetGroup;

    buffer->targetCount = group->count;
    buffer->targets = (RotateTargetState *)malloc(sizeof(RotateTargetState) * group->count);
    if (!buffer->targets) {
        buffer->targetCount = 0;
        buffer->active = false;
        return;
    }
    for (int i = 0; i < group->count; i++) {
        int target = group->objects[i];
        rememberRotateInitialRotation(target);
        buffer->targets[i].obj = target;
    }

    if (buffer->useCenter) {
        GroupNode *center = getGroup(trigger->centerGroup);
        if (center && center->count == 1 && trigger->centerGroup != trigger->targetGroup) {
            buffer->centerObj = center->objects[0];
        } else {
            buffer->useCenter = false;
        }
    }
    buffer->active = true;
}

static void compactRotateTriggers(void) {
    int count = 0;
    for (int i = 0; i < rotateTriggerCount; i++) {
        RotateTriggerBuffer *buffer = &rotateTriggerBuffer[i];
        if (!buffer->active) {
            free(buffer->targets);
            buffer->targets = NULL;
            continue;
        }
        if (count != i) {
            rotateTriggerBuffer[count] = *buffer;
            buffer->targets = NULL;
        }
        count++;
    }
    rotateTriggerCount = count;
}

static bool rotateTargetIsSolid(int obj) {
    int id = objects.id[obj];
    if (id <= 0 || id >= GAME_OBJECT_COUNT) return false;
    const ObjectHitbox *hitbox = gameObjects[id].hitbox;
    return hitbox && hitbox->collisionType == HITBOX_SOLID;
}

void handleRotateTriggers(void) {
    if (triggerDt <= 0.f) return;

    beginGroupCombinationRotationStep();

    for (int slot = 0; slot < rotateTriggerCount; slot++) {
        RotateTriggerBuffer *buffer = &rotateTriggerBuffer[slot];
        if (!buffer->active) continue;
        buffer->timeRun += triggerDt;
        float prevTime = buffer->timeRun - triggerDt;
        if (prevTime < 0.f) prevTime = 0.f;
        float t = buffer->seconds > 0.f ? easeTime((EaseTypes)buffer->easingType,
            buffer->timeRun, buffer->seconds, buffer->easingRate) : 1.f;
        float prevT = buffer->seconds > 0.f ? easeTime((EaseTypes)buffer->easingType,
            prevTime, buffer->seconds, buffer->easingRate) : 0.f;
        float angle = buffer->totalDegrees * t;
        float prevAngle = buffer->totalDegrees * prevT;
        float angleDelta = angle - prevAngle;
        Matrix2x2 rotationMatrix = matrix2x2Identity();
        float pivotX = 0.f;
        float pivotY = 0.f;
        if (buffer->useCenter) {
            getGroupObjectPosition(buffer->centerObj, &pivotX, &pivotY);
            float rad = -angleDelta * (float)(M_PI / 180.0);
            rotationMatrix = matrix2x2Rotation(rad);
        }

        bool isLastForGroup = true;
        for (int later = slot + 1; later < rotateTriggerCount; later++) {
            if (rotateTriggerBuffer[later].active && rotateTriggerBuffer[later].targetGroup == buffer->targetGroup) {
                isLastForGroup = false;
                break;
            }
        }

        if (isLastForGroup && addRotationToGroupCombinations(buffer->targetGroup, &rotationMatrix, pivotX, pivotY)) {
            for (int i = 0; i < buffer->targetCount; i++) {
                RotateTargetState *target = &buffer->targets[i];
                int obj = target->obj;
                if (!buffer->lockObjectRotation && !rotateTargetIsSolid(obj)) {
                    objects.rotation[obj] += angleDelta;
                    objects.visualRotation[obj] += angleDelta;
                }
            }
        }
        if (buffer->timeRun >= buffer->seconds) buffer->active = false;
    }

    applyGroupCombinationRotation();
    compactRotateTriggers();
}

void uploadToSpawnBuffer(int obj, bool fromSpawn) {
    for (int i = 0; i < spawnTriggerCount; i++) {
        SpawnTriggerBuffer *existing = &spawnTriggerBuffer[i];
        if (!existing->active || existing->sourceObj != obj) continue;

        if (existing->restoredFromCheckpoint) {
            existing->restoredFromCheckpoint = false;
        }
        return;
    }

    SpawnTriggerBuffer *buffer = getNewSpawnTrigger();
    if (!buffer) return;
    
    SpawnTrigger *trigger = getSpawnTrigger(obj);
    buffer->queued = fromSpawn;
    buffer->targetGroup = trigger->targetGroup;
    buffer->sourceObj = obj;
    buffer->seconds = trigger->spawnDelay ? trigger->spawnDelay : trigger->trigDuration;
    buffer->timeRun = 0;
    buffer->active = true;
}

void handleSpawnTriggers(void) {
    int count = 0;
    for (int slot = 0; slot < spawnTriggerCount; slot++) {
        SpawnTriggerBuffer *buffer = &spawnTriggerBuffer[slot];
        if (!buffer->active) continue;
        buffer->queued = false;
        if (count != slot) spawnTriggerBuffer[count] = *buffer;
        count++;
    }

    spawnTriggerCount = count;

    for (int slot = 0; slot < spawnTriggerCount; slot++) {
        SpawnTriggerBuffer *buffer = &spawnTriggerBuffer[slot];
        if (!buffer->active || buffer->queued) continue;

        buffer->timeRun += triggerDt;
        if (buffer->timeRun > buffer->seconds) {
            buffer->active = false;

            spawnGroup(buffer->targetGroup);

            for (int j = slot; j < spawnTriggerCount - 1; j++) {
                // Shift buffer array
                spawnTriggerBuffer[j] = spawnTriggerBuffer[j + 1];
            }
            
            // Make the game run the pulse moved into the current slot
            slot--;
            spawnTriggerCount--;
        }
    }
}

static void pulseRegister(PulseTriggerBuffer *buffer, PulseTrigger *trigger) {
    int channel = trigger->targetGroup;

    buffer->targetGroup = trigger->targetGroup;

    buffer->color.r = trigger->trigColorR;
    buffer->color.g = trigger->trigColorG;
    buffer->color.b = trigger->trigColorB;

    buffer->copiedColorID = trigger->copiedColorID;
    buffer->copiedHSV = trigger->copiedHSV;

    buffer->mainOnly = trigger->pulseMainOnly;
    buffer->detailOnly = trigger->pulseDetailOnly;

    buffer->fadeIn  = trigger->pulseFadeIn;
    buffer->hold     = trigger->pulseHold;
    buffer->fadeOut = trigger->pulseFadeOut;

    buffer->pulseMode = trigger->pulseMode;
    buffer->pulseTargetType = trigger->pulseTargetType;

    buffer->targetColorID = channel;

    if (buffer->pulseTargetType == PULSE_TARGET_CHANNEL) {
        ColorChannel *chan = &channels[getColChannelIndex(buffer->targetColorID)];
        buffer->pulseIndex = chan->numPulses;
        chan->numPulses++;
    } else if (buffer->pulseTargetType == PULSE_TARGET_GROUP) {
        bool both = !buffer->mainOnly && !buffer->detailOnly;
        GroupNode *p = getGroup(buffer->targetGroup);
        if (!p) return;

        for (int i = 0; i < p->count; i++) {
            int objIdx = p->objects[i];
            if (both || buffer->mainOnly) {
                objects.numMainPulses[objIdx]++;
                objects.mainBeingPulsed[objIdx] = true;
            }

            if (both || buffer->detailOnly) {
                objects.numDetailPulses[objIdx]++;
                objects.detailBeingPulsed[objIdx] = true;
            }
        }
    }

    buffer->seconds = trigger->pulseFadeIn + trigger->pulseHold + trigger->pulseFadeOut;
}


void uploadToPulseBuffer(int obj) {
    PulseTrigger *trigger = getPulseTrigger(obj);
    int channel = trigger->targetGroup;
    if (channel == 0) return;

    for (int i = 0; i < pulseTriggerCount; i++) {
        PulseTriggerBuffer *existing = &pulseTriggerBuffer[i];
        if (!existing->active || existing->sourceObj != obj) continue;

        if (existing->restoredFromCheckpoint) {
            existing->restoredFromCheckpoint = false;
            return;
        }

        existing->timeRun = 0;
        return;
    }

    PulseTriggerBuffer *buffer = getNewPulseTrigger();
    if (!buffer) return;

    pulseRegister(buffer, trigger);

    buffer->sourceObj = obj;
    buffer->timeRun = 0;
    buffer->active = true;
}

void restorePulseTrigger(int obj, float timeRun) {
    PulseTrigger *trigger = getPulseTrigger(obj);
    if (trigger->targetGroup == 0) return;

    PulseTriggerBuffer *buffer = getNewPulseTrigger();
    if (!buffer) return;

    pulseRegister(buffer, trigger);

    buffer->sourceObj = obj;
    buffer->restoredFromCheckpoint = true;
    buffer->timeRun = timeRun;
    buffer->active = true;
}

void handlePulseTriggers(void) {
    // preprocess HSV for all active pulses
    for (int i = 0; i < pulseTriggerCount; i++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[i];
        if (!buffer->active) continue;
        if (buffer->pulseMode == PULSE_MODE_HSV) {
            int sourceID = buffer->copiedColorID;
            if (sourceID <= 0 || sourceID >= COL_CHANNEL_NUM)
                sourceID = buffer->targetColorID;
            int sourceIdx = getColChannelIndex(sourceID);
            Color copyColor = channels[sourceIdx].nonPulseColor;
            buffer->color = hsvCombine(copyColor, buffer->copiedHSV);
        }
    }

    // process channel pulses in pulse_index order
    for (int i = 0; i < pulseTriggerCount; i++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[i];
        if (!buffer->active || buffer->pulseTargetType != PULSE_TARGET_CHANNEL) continue;

        ColorChannel *chan = &channels[getColChannelIndex(buffer->targetColorID)];
        int idx = buffer->pulseIndex;
        if (idx < 0 || idx >= chan->numPulses) continue;

        Color baseColor = (idx == 0) ? chan->nonPulseColor : chan->color;

        Color resultColor;
        if (buffer->timeRun <= buffer->fadeIn) {
            float fadeTime = 1.f;
            if (buffer->fadeIn > 0) fadeTime = buffer->timeRun / buffer->fadeIn;
            float r = (buffer->color.r - (buffer->color.r - baseColor.r) * (1.f - fadeTime));
            float g = (buffer->color.g - (buffer->color.g - baseColor.g) * (1.f - fadeTime));
            float b = (buffer->color.b - (buffer->color.b - baseColor.b) * (1.f - fadeTime));
            resultColor.r = (unsigned char)r;
            resultColor.g = (unsigned char)g;
            resultColor.b = (unsigned char)b;
        } else if (buffer->timeRun >= buffer->fadeIn + buffer->hold) {
            float fadeTime = 1.f;
            if (buffer->fadeOut > 0) fadeTime = (buffer->timeRun - buffer->hold - buffer->fadeIn) / buffer->fadeOut;
            float r = (buffer->color.r - (buffer->color.r - baseColor.r) * fadeTime);
            float g = (buffer->color.g - (buffer->color.g - baseColor.g) * fadeTime);
            float b = (buffer->color.b - (buffer->color.b - baseColor.b) * fadeTime);
            resultColor.r = (unsigned char)r;
            resultColor.g = (unsigned char)g;
            resultColor.b = (unsigned char)b;
        } else {
            resultColor = buffer->color;
        }

        chan->color = resultColor;
    }

    for (int gi = 0; gi < pulseTriggerCount; gi++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[gi];
        if (!buffer->active || buffer->pulseTargetType != PULSE_TARGET_GROUP) continue;
        bool both = !buffer->mainOnly && !buffer->detailOnly;
        GroupNode *p = getGroup(buffer->targetGroup);
        if (!p) continue;
        for (int i = 0; i < p->count; i++) {
            int objIdx = p->objects[i];
            if (objIdx < 0 || objIdx >= objects.count) continue;
            if (both || buffer->mainOnly) {
                objects.mainColor[objIdx] = objects.mainNonPulseColor[objIdx];
                objects.mainBeingPulsed[objIdx] = true;
            }
            if (both || buffer->detailOnly) {
                objects.detailColor[objIdx] = objects.detailNonPulseColor[objIdx];
                objects.detailBeingPulsed[objIdx] = true;
            }
        }
    }

    for (int gi = 0; gi < pulseTriggerCount; gi++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[gi];
        if (!buffer->active || buffer->pulseTargetType != PULSE_TARGET_GROUP) continue;
        bool both = !buffer->mainOnly && !buffer->detailOnly;

        float alpha;
        if (buffer->timeRun <= buffer->fadeIn) {
            alpha = (buffer->fadeIn > 0.f) ? (buffer->timeRun / buffer->fadeIn) : 1.f;
        } else if (buffer->timeRun >= buffer->fadeIn + buffer->hold) {
            float fadeTime = 1.f;
            if (buffer->fadeOut > 0.f) fadeTime = (buffer->timeRun - buffer->hold - buffer->fadeIn) / buffer->fadeOut;
            alpha = 1.f - fadeTime;
        } else {
            alpha = 1.f;
        }
        if (alpha < 0.f) alpha = 0.f;
        else if (alpha > 1.f) alpha = 1.f;

        GroupNode *p = getGroup(buffer->targetGroup);
        if (!p) continue;
        for (int i = 0; i < p->count; i++) {
            int objIdx = p->objects[i];
            if (objIdx < 0 || objIdx >= objects.count) continue;
            if (both || buffer->mainOnly) {
                Color target = buffer->color;
                if (buffer->pulseMode == PULSE_MODE_HSV && buffer->copiedColorID == 0) {
                    target = hsvCombine(objects.mainColor[objIdx], buffer->copiedHSV);
                }

                if (objects.mainColHSVEnabled[objIdx]) target = hsvCombine(target, objects.mainColHSV[objIdx]);
                Color base = objects.mainColor[objIdx];
                objects.mainColor[objIdx].r = (unsigned char)(base.r + (target.r - base.r) * alpha);
                objects.mainColor[objIdx].g = (unsigned char)(base.g + (target.g - base.g) * alpha);
                objects.mainColor[objIdx].b = (unsigned char)(base.b + (target.b - base.b) * alpha);
                objects.mainBeingPulsed[objIdx] = true;
            }
            if (both || buffer->detailOnly) {
                Color target = buffer->color;
                if (buffer->pulseMode == PULSE_MODE_HSV && buffer->copiedColorID == 0) {
                    target = hsvCombine(objects.detailColor[objIdx], buffer->copiedHSV);
                }
                if (objects.detailColHSVEnabled[objIdx]) target = hsvCombine(target, objects.detailColHSV[objIdx]);
                Color base = objects.detailColor[objIdx];
                objects.detailColor[objIdx].r = (unsigned char)(base.r + (target.r - base.r) * alpha);
                objects.detailColor[objIdx].g = (unsigned char)(base.g + (target.g - base.g) * alpha);
                objects.detailColor[objIdx].b = (unsigned char)(base.b + (target.b - base.b) * alpha);
                objects.detailBeingPulsed[objIdx] = true;
            }
        }
    }

    // time update and cleanup for all pulses
    for (int i = 0; i < pulseTriggerCount; i++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[i];
        if (!buffer->active) continue;

        buffer->timeRun += triggerDt;
        if (buffer->timeRun > buffer->seconds) {
            if (buffer->pulseTargetType == PULSE_TARGET_GROUP) {
                bool both = !buffer->mainOnly && !buffer->detailOnly;
                GroupNode *p = getGroup(buffer->targetGroup);
                int count = p ? p->count : 0;
                for (int i = 0; i < count; i++) {
                    int objIdx = p->objects[i];
                    if (objIdx < 0 || objIdx >= objects.count) continue;
                    if (both || buffer->mainOnly) {
                        objects.numMainPulses[objIdx]--;
                        if (!objects.numMainPulses[objIdx]) {
                            objects.mainBeingPulsed[objIdx] = false;
                            objects.mainColor[objIdx] = objects.mainNonPulseColor[objIdx];
                        }
                    }
                    if (both || buffer->detailOnly) {
                        objects.numDetailPulses[objIdx]--;
                        if (!objects.numDetailPulses[objIdx]) {
                            objects.detailBeingPulsed[objIdx] = false;
                            objects.detailColor[objIdx] = objects.detailNonPulseColor[objIdx];
                        }
                    }
                }
            } else if (buffer->pulseTargetType == PULSE_TARGET_CHANNEL) {
                int chanIdx = getColChannelIndex(buffer->targetColorID);
                ColorChannel *chan = &channels[chanIdx];
                int removedIdx = buffer->pulseIndex;

                chan->numPulses--;
                if (!chan->numPulses) chan->color = chan->nonPulseColor;

                for (int j = 0; j < pulseTriggerCount; j++) {
                    if (j == i) continue;
                    PulseTriggerBuffer *other = &pulseTriggerBuffer[j];
                    if (!other->active) continue;
                    if (other->pulseTargetType != PULSE_TARGET_CHANNEL) continue;
                    if (other->targetColorID != buffer->targetColorID) continue;
                    if (other->pulseIndex > removedIdx) other->pulseIndex--;
                }
            }

            for (int j = i; j < pulseTriggerCount - 1; j++) {
                // Shift buffer array
                pulseTriggerBuffer[j] = pulseTriggerBuffer[j + 1];
            }

            pulseTriggerCount--;
            // Make the game run the pulse moved into the current slot
            i--;
        }
    }
}

void uploadToBuffer(int obj, int channel) {
    if (channel == 0) channel = 1;
    int bufferChannel = getColChannelIndex(channel);

    ColTriggerBuffer *buffer = &colTriggerBuffer[bufferChannel];
    ColorTrigger *trigger =  getColorTrigger(obj);

    buffer->oldColor = channels[bufferChannel].color;
    buffer->oldAlpha = channels[bufferChannel].alpha;
    if (trigger && trigger->p1Color) {
        buffer->newColor = getP2IfBlack(p1Color);
        buffer->newAlpha = 1.0f;
    } else if (trigger && trigger->p2Color) {
        buffer->newColor = getP1IfBlack(p2Color);
        buffer->newAlpha = 1.0f;
    } else {
        buffer->newColor.r = trigger->trigColorR;
        buffer->newColor.g = trigger->trigColorG;
        buffer->newColor.b = trigger->trigColorB;
        buffer->newAlpha = trigger->opacity;
    }

    int copyID = trigger->copiedColorID;
    if (copyID > 0) {
        buffer->copiedColorID = copyID;
        buffer->copiedHSV = trigger->copiedHSV;
        channels[bufferChannel].copyColorID = copyID;
        channels[bufferChannel].hsv = trigger->copiedHSV;
    } else {
        buffer->copiedColorID = 0;
        channels[bufferChannel].copyColorID = 0;
    }

    if (channel < CHANNEL_BG) {
        channels[bufferChannel].blending = trigger->blending;
    }
    
    
    float duration = trigger->trigDuration;
    if (duration == 0) {
        Color colorToLerp = buffer->newColor;

        if (buffer->copiedColorID > 0) {
            int src = getColChannelIndex(buffer->copiedColorID);
            colorToLerp = channels[src].color;
            buffer->newAlpha = channels[src].alpha;
        }

        channels[bufferChannel].color = colorToLerp;
        channels[bufferChannel].nonPulseColor = colorToLerp;
        channels[bufferChannel].alpha = buffer->newAlpha;
        return;
    }
    
    buffer->seconds = duration;
    buffer->timeRun = 0;
    buffer->sourceObj = obj;
    buffer->active = true;
}

void uploadColorToBuffer(int channel, u32 color, float seconds) {
    int bufferChannel = getColChannelIndex(channel);

    ColTriggerBuffer *buffer = &colTriggerBuffer[bufferChannel];
    buffer->oldColor = channels[bufferChannel].color;
    buffer->oldAlpha = channels[bufferChannel].alpha;
    buffer->newColor.r = GET_R(color);
    buffer->newColor.g = GET_G(color);
    buffer->newColor.b = GET_B(color);
    buffer->newAlpha = 1.0f;
    buffer->seconds = seconds;
    buffer->timeRun = 0;
    buffer->sourceObj = 0;
    buffer->active = true;
}

void toggleGroup(int group, bool activate, bool spawn) {
    GroupNode *p = getGroup(group);
    if (p) {
        p->toggled = !activate;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.flags[object] &= ~FLAG_TOGGLED;
            objects.flags[object] |= getGroupToggles(object) ? FLAG_TOGGLED : 0;
        }

        if (activate && spawn) {
            if (toggleSpawnCount < 8) toggleSpawnBuffer[toggleSpawnCount++] = group;
            if (toggleSpawnActive) return;

            toggleSpawnActive = true;
            int guard = 0;
            while (toggleSpawnCount > 0 && guard++ < 64) {
                int next = toggleSpawnBuffer[0];
                for (int i = 1; i < toggleSpawnCount; i++) toggleSpawnBuffer[i - 1] = toggleSpawnBuffer[i];
                toggleSpawnCount--;
                spawnGroup(next);
            }
            toggleSpawnActive = false;
        }
    }
}

void spawnGroup(int group) {
    GroupNode *p = getGroup(group);
    if (!p) return;

    for (int i = 0; i < p->count; i++) {
        int objIdx = p->objects[i];
        if (triggerIsSpawnTriggered(objects.id[objIdx], objIdx)
            && isTriggerObject(objects.id[objIdx])
            && (triggerIsMultiTriggered(objects.id[objIdx], objIdx) || !GET_ACTIVATED(objIdx))
            && !(objects.flags[objIdx] & FLAG_TOGGLED)) {
            runTrigger(objIdx, true);
        }
    }
}

static bool objectInGroup(int obj, int group) {
    if (obj < 0 || obj >= (int)objects.count) return false;
    for (int i = 0; i < objects.groupCount[obj]; i++) {
        if (objects.groups[obj][i] == group) return true;
    }
    return false;
}

static void registerOnDeathTrigger(int obj) {
    OnDeathTrigger *trigger = getOnDeathTrigger(obj);

    for (int i = 0; i < onDeathCount; i++) {
        if (onDeathEntries[i].sourceObj == obj) return;
    }

    if (onDeathCount >= ON_DEATH_MAX) return;

    OnDeathEntry *entry = &onDeathEntries[onDeathCount++];
    entry->sourceObj = obj;
    entry->targetGroup = trigger->targetGroup;
    entry->activate = trigger->activate;
    entry->multi = trigger->multiTriggered;
}

void runOnDeathTriggers() {
    int count = onDeathCount;
    int kept = 0;

    for (int i = 0; i < count; i++) {
        OnDeathEntry *entry = &onDeathEntries[i];

        toggleGroup(entry->targetGroup, entry->activate, true);

        if (!entry->multi) continue;
        if (kept != i) onDeathEntries[kept] = *entry;
        kept++;
    }

    onDeathCount = kept;
}

void clearOnDeathTriggers() {
    onDeathCount = 0;
}

static void stopActions(int group) {
    for (int i = 0; i < moveTriggerCount; i++) {
        MoveTriggerBuffer *buffer = &moveTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < rotateTriggerCount; i++) {
        RotateTriggerBuffer *buffer = &rotateTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < alphaTriggerCount; i++) {
        AlphaTriggerBuffer *buffer = &alphaTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < pulseTriggerCount; i++) {
        PulseTriggerBuffer *buffer = &pulseTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->timeRun = buffer->seconds + 0.001f;
    }

    for (int i = 0; i < followTriggerCount; i++) {
        FollowTriggerBuffer *buffer = &followTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < followPlayerYTriggerCount; i++) {
        FollowPlayerYTriggerBuffer *buffer = &followPlayerYTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < COL_CHANNEL_NUM; i++) {
        ColTriggerBuffer *buffer = &colTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < spawnTriggerCount; i++) {
        SpawnTriggerBuffer *buffer = &spawnTriggerBuffer[i];
        if (!buffer->active || !objectInGroup(buffer->sourceObj, group)) continue;
        buffer->active = false;
    }

    int kept = 0;
    for (int i = 0; i < onDeathCount; i++) {
        if (objectInGroup(onDeathEntries[i].sourceObj, group)) continue;
        if (kept != i) onDeathEntries[kept] = onDeathEntries[i];
        kept++;
    }

    onDeathCount = kept;
}

void runTrigger(int obj, bool fromSpawn) {
    if (objects.flags[obj] & FLAG_TOGGLED) return;

    switch (objects.id[obj]) {
        case TRIGGER_FADE_SIMPLE:
            currentFadingEffect = FADE_SIMPLE;
            break;
            
        case TRIGGER_FADE_UP:
            currentFadingEffect = FADE_UP;
            break;
            
        case TRIGGER_FADE_DOWN:
            currentFadingEffect = FADE_DOWN;
            break;
            
        case TRIGGER_FADE_RIGHT:
            currentFadingEffect = FADE_RIGHT;
            break;
            
        case TRIGGER_FADE_LEFT:
            currentFadingEffect = FADE_LEFT;
            break;
            
        case TRIGGER_FADE_SCALE_IN:
            currentFadingEffect = FADE_SCALE_IN;
            break;
            
        case TRIGGER_FADE_SCALE_OUT:
            currentFadingEffect = FADE_SCALE_OUT;
            break;
        
        case TRIGGER_FADE_INWARDS:
            currentFadingEffect = FADE_INWARDS;
            break;

        case TRIGGER_FADE_OUTWARDS:
            currentFadingEffect = FADE_OUTWARDS;
            break;
        
        case TRIGGER_FADE_LEFT_SEMICIRCLE:
            currentFadingEffect = FADE_CIRCLE_LEFT;
            break;

        case TRIGGER_FADE_RIGHT_SEMICIRCLE:
            currentFadingEffect = FADE_CIRCLE_RIGHT;
            break;

        case BG_TRIGGER:
            uploadToBuffer(obj, CHANNEL_BG);
            if (!getColorTrigger(obj)->tintGround) break;
        
        case GROUND_TRIGGER:
            uploadToBuffer(obj, CHANNEL_GROUND);
            break;
                    
        case LINE_TRIGGER:
        case V2_0_LINE_TRIGGER: // gd converts 1.4 line trigger to 2.0 one for some reason
            uploadToBuffer(obj, CHANNEL_LINE);
            break;
        
        case OBJ_TRIGGER:
            uploadToBuffer(obj, CHANNEL_OBJ);
            uploadToBuffer(obj, CHANNEL_OBJ_BLENDING);
            break;
        
        case OBJ_2_TRIGGER:
            uploadToBuffer(obj, 1);
            break;
        
        case COL2_TRIGGER: // col 2
            uploadToBuffer(obj, 2);
            break;

        case COL3_TRIGGER: // col 3
            uploadToBuffer(obj, 3);
            break;
            
        case COL4_TRIGGER: // col 4
            uploadToBuffer(obj, 4);
            break;
            
        case THREEDL_TRIGGER: // 3DL
            uploadToBuffer(obj, CHANNEL_3DL);
            break;

        case GROUND_2_TRIGGER:
            uploadToBuffer(obj, CHANNEL_GROUND_2);
            break;

        case ENABLE_TRAIL:
            p1Trail = true;
            break;
        
        case DISABLE_TRAIL:
            p1Trail = false;
            break;

        case COL_TRIGGER: // 2.0 color trigger
            if (getColChannelIndex(getColorTrigger(obj)->targetColorID) == getColChannelIndex(CHANNEL_BLACK)) break;
            uploadToBuffer(obj, getColorTrigger(obj)->targetColorID);
            break;
        case ALPHA_TRIGGER:
            uploadToAlphaBuffer(obj);
            break;
        case MOVE_TRIGGER:
            uploadToMoveBuffer(obj);
            break;
        case COUNT_TRIGGER:
            uploadToCountBuffer(obj);
            break;
        case INSTANT_COUNT_TRIGGER:
        {
            InstantCountTrigger *trigger = getInstantCountTrigger(obj);
            int value = getItemCount(trigger->itemID);
            bool pass = false;
            switch (trigger->instantCountMode) {
                case INSTANT_COUNT_MODE_EQUALS: pass = value == trigger->targetCount; break;
                case INSTANT_COUNT_MODE_LARGER: pass = value > trigger->targetCount; break;
                case INSTANT_COUNT_MODE_SMALLER: pass = value < trigger->targetCount; break;
            }
            if (pass) toggleGroup(trigger->targetGroup, trigger->activateGroup, true);
            break;
        }
        case FOLLOW_PLAYER_Y_TRIGGER:
            uploadToFollowPlayerYBuffer(obj);
            break;
        case FOLLOW_TRIGGER:
            uploadToFollowBuffer(obj);
            break;
        case ROTATE_TRIGGER:
            uploadToRotateBuffer(obj);
            break;
        case SPAWN_TRIGGER:
            uploadToSpawnBuffer(obj, fromSpawn);
            break;
        case PULSE_TRIGGER:
            if (getPulseTrigger(obj)->pulseTargetType == PULSE_TARGET_CHANNEL &&
                getColChannelIndex(getPulseTrigger(obj)->targetGroup) == getColChannelIndex(CHANNEL_BLACK)) break;
            uploadToPulseBuffer(obj);
            break;
        case TOGGLE_TRIGGER:
        {
            ToggleTrigger *trigger = getToggleTrigger(obj);
            toggleGroup(trigger->targetGroup, trigger->activateGroup, false);
            break;
        }
        case TOUCH_TRIGGER:
            uploadToTouchBuffer(obj);
            break;
        case STOP_TRIGGER:
        {
            StopTrigger *trigger = getStopTrigger(obj);
            stopActions(trigger->targetGroup);
            break;
        }
        case ON_DEATH_TRIGGER:
            registerOnDeathTrigger(obj);
            break;
        case HIDE_PLAYER_TRIGGER:
            motionTrailStopStroke(&trailP1);
            motionTrailStopStroke(&trailP2);
            motionTrailClear(&waveTrailP1);
            motionTrailClear(&waveTrailP2);
            motionTrailStopStroke(&waveTrailP1);
            motionTrailStopStroke(&waveTrailP2);
            state.showPlayer = false;
            break;
        case SHOW_PLAYER_TRIGGER:
            motionTrailResumeStroke(&trailP1);
            motionTrailResumeStroke(&trailP2);
            motionTrailResumeStroke(&waveTrailP1);
            motionTrailResumeStroke(&waveTrailP2);
            state.showPlayer = true;
            break;
        default:
            return;
    }
    SET_ACTIVATED(obj, true);
}

int compareTriggers(const void *a, const void *b) {
    int ta = *((int*) a);
    int tb = *((int*) b);
    
    float xa = objects.x[ta];
    float xb = objects.x[tb];

    if (xa != xb) {
        return xa - xb;
    }
    
    float ya = objects.y[ta];
    float yb = objects.y[tb];

    return yb - ya;
}

#define TOUCH_TRIGGER_EPSILON 0.5f

int triggersBuffer[TRIGGER_BUFFER_SIZE];
int triggerCount;

bool objectCanBeXMoved(int obj) {
    int objID = objects.id[obj];
    if (isTriggerObject(objID) && !objects.touchTriggered[obj]) {
        return false;
    }

    // Speed portals
    if (objID >= 200 && objID <= 203) return false;

    return true;
}

void handleTriggers(void) {
    triggerCount = 0;
    int camSx = (int)((state.player.x) / SECTION_SIZE);
    
    for (int sx = -1; sx < 1; sx++) {
        for (int sy = -(400 / SECTION_SIZE); sy <= MAX_LEVEL_HEIGHT / SECTION_SIZE; sy++) {
            int secX = camSx + sx;
            if (secX < 0) continue;

            Section *sec = getSection(secX, sy);
            const SectionTriggers *list = sec->triggers;
            if (!list) continue;
            bool cached = list->count >= 0;
            int count = cached ? list->count : sec->objectCount;
            for (int i = 0; i < count; i++) {
                int obj = sec->objects[cached ? list->indices[i] : i];
                if (!cached && !isTriggerObject(objects.id[obj])) continue;
                
                if (!GET_ACTIVATED(obj)) {
                    if (objects.touchTriggered[obj]) {
                        // Try p1
                        if (intersect(
                            state.player.x, state.player.y, state.player.width, state.player.height, 0, 
                            objects.x[obj], objects.y[obj], objects.width[obj] + TOUCH_TRIGGER_EPSILON, objects.height[obj] + TOUCH_TRIGGER_EPSILON, objects.rotation[obj]
                        )) {
                            runTrigger(obj, false);
                        } else
                        // Try now p2
                        if (intersect(
                            state.player2.x, state.player2.y, state.player2.width, state.player2.height, 0, 
                            objects.x[obj], objects.y[obj], objects.width[obj] + TOUCH_TRIGGER_EPSILON, objects.height[obj] + TOUCH_TRIGGER_EPSILON, objects.rotation[obj]
                        )) {
                            runTrigger(obj, false);
                        }
                    } else if (!triggerIsSpawnTriggered(objects.id[obj], obj) && objects.x[obj] < state.player.x) {
                        if (triggerCount < TRIGGER_BUFFER_SIZE) {
                            triggersBuffer[triggerCount++] = obj;
                        }
                    }
                }
            }
        }
    }

    qsort(triggersBuffer, triggerCount, sizeof(int), compareTriggers);

    for (int i = 0; i < triggerCount; i++) {
        runTrigger(triggersBuffer[i], false);
    }
}

void handleStepTriggers(void) {
    if (hasFollowPlayerY) {
        playerFollowYStep(&state.player, triggerDt);
        if (state.dual) playerFollowYStep(&state.player2, triggerDt);
    }
    handleTriggers();
    handleTouchTriggers();
    handleCountTriggers();
    handleSpawnTriggers();
    handleMoveTriggers();
    handleFollowPlayerYTriggers();
    handleRotateTriggers();
    handleFollowTriggers();
}

// https://github.com/gd-programming/gd.docs/issues/87
void calculateLbg() {
    ColorChannel channel = channels[getColChannelIndex(CHANNEL_BG)];
    float h,s,v;
    
    convertRGBtoHSV(channel.color.r, channel.color.g, channel.color.b, &h, &s, &v);

    s -= 0.20f;
    s = clampf(s, 0.f, 1.f);
    v += 0.20f;
    v = clampf(v, 0.f, 1.f);

    unsigned char r,g,b;

    convertHSVtoRGB(h, s, v, &r, &g, &b);

    int chanLbgNolerp = getColChannelIndex(CHANNEL_LBG_NOLERP);
    channels[chanLbgNolerp].color.r = r;
    channels[chanLbgNolerp].color.g = g;
    channels[chanLbgNolerp].color.b = b;
    channels[chanLbgNolerp].blending = true;

    float factor = (channel.color.r + channel.color.g + channel.color.b) / 150.f;

    if (factor < 1.f) {
        Color p1 = getWhiteIfBlack(p1Color);
        r = r * factor + p1.r * (1 - factor);
        g = g * factor + p1.g * (1 - factor);
        b = b * factor + p1.b * (1 - factor);
    }

    // Set here lerped LBG
    int chanLbg = getColChannelIndex(CHANNEL_LBG);
    channels[chanLbg].color.r = r;
    channels[chanLbg].color.g = g;
    channels[chanLbg].color.b = b;
    channels[chanLbg].blending = true;
}

}
