#include "practice.hpp"
#include "icons.hpp"
#include <stdlib.h>
#include <string.h>
#include "groups.hpp"
#include "level_loading.hpp"
#include "main.hpp"
#include "graphics.hpp"
#include "player/player.hpp"
#include "player/collision.hpp"
#include "state.hpp"
#include "mp3_player.hpp"
#include "math_helpers.hpp"
#include "triggers.hpp"
#include "utils/gfx.hpp"
#include "menus/settings_hub/settings.hpp"

// needed to re-create the runtime trigger buffers
// after reload_level() frees them
namespace gd3ds
{

extern int alphaTriggerCapacity;
extern int moveTriggerCapacity;
extern int spawnTriggerCapacity;
extern int touchTriggerCapacity;
extern int followPlayerYTriggerCapacity;

bool restoredCheckpoint = false;

int usedCount = 0;
int channelsUsed[static_cast<int>(ColorChannelIDs::ColChannelNum)];

#define MAX_CHECKPOINTS 100
#define CHECKPOINT_GFX_ID 6

struct PulseSnapshot {
    int sourceObj;
    float timeRun;
};

struct RotateObjectSnapshot {
    int obj;
    float rotation, visualRotation;
};

struct CheckpointData {
    Player p1;
    Player p2;

    float cameraX;
    float cameraY;

    float cameraIntendedY;

    float groundY;
    float ceilingY;
    float groundYGfx;

    float wallY;

    bool mirroring;
    int mirrorMult;    
    float mirrorTimer;

    float originalMirrorFactor;
    float intendedMirrorFactor;

    float mirrorSpeedFactor;
    float mirrorFactor;
    
    bool dual;
    
    bool showPlayer;
    
    float dualPortalY;
    unsigned char speed;

    int currentFadingEffect;
    bool p1Trail;

    // snapshot only of the color channels actually used by the level to avoid a 1024 entry copy per checkpoint
    // allocated dynamically in new_checkpoint() (tis is great)
    int channelSnapshotCount;
    ColorChannel *channelSnapshot;
    ColTriggerBuffer *triggerSnapshot;

    MoveTriggerBuffer *moveTriggers;
    AlphaTriggerBuffer *alphaTriggers;
    SpawnTriggerBuffer *spawnTriggers;
    FollowPlayerYTriggerBuffer *followTriggers;
    FollowTriggerBuffer *followTriggerBuffers;
    RotateTriggerBuffer *rotateTriggers;
    RotateTargetState *rotateTargets;
    int *rotateTargetOffsets;
    RotateObjectSnapshot *rotateObjSnapshot;
    ItemCount *countItems;
    CountTriggerBuffer *countTriggers;
    TouchTriggerBuffer *touchTriggers;
    int *collectedPickups;
    int countItemsCount;
    int countTriggersCount;
    int touchTriggersCount;
    int collectedPickupsCount;
    int moveTriggersCount;
    int alphaTriggersCount;
    int spawnTriggersCount;
    int followTriggersCount;
    int followTriggerBuffersCount;
    int rotateTriggersCount;
    int rotateTargetsCount;
    int rotateObjCount;

    // per-object state for triggers that were active at checkpoint time, so they
    // can continue from the exact point instead of restarting. bounded by the
    // number of active move/alpha triggers so it's usually empty
    int moveObjCount;
    uint8_t *moveIdxStream;
    int moveIdxBytes;
    uint8_t *moveObjX;
    int moveXBytes;
    int moveXFmt;
    int moveXU;
    int moveCx;
    uint8_t *moveObjY;
    int moveYBytes;
    int moveYFmt;
    int moveYU;
    int moveCy;

    int firedMoveCount;
    int *firedMoveIndex;

    int keyCount;
    int *keyIndex;

    int toggleGroupCount;
    int *toggleGroupID;

    int alphaGroupCount;
    int *alphaGroupID;
    float *alphaGroupAlpha;

    int pulseSnapCount;
    PulseSnapshot *pulseSnap;

    float songOffset;

    void *allocBase;
};

CheckpointData checkpoints[MAX_CHECKPOINTS];

static bool isPendingPseudoCheckpoint = false;

int checkpointCount = 0;
int checkpointPointer = 0;
float checkpointTimer = 0;
bool pseudoCheckpointExists = false;

static unsigned int ckFloatBits(float f);
static int ckVarintLen(unsigned int v);
static uint8_t *ckVarintPut(uint8_t *p, unsigned int v);
static unsigned int ckVarintGet(const uint8_t **pp);
static float ckAxisGet(const uint8_t *blob, int fmt, int u, int k);

static void addUsedChannel(int *out, bool *seen, int *count, int channel) {
    if (channel <= 0 || channel >= static_cast<int>(ColorChannelIDs::ColChannelNum)) return;

    int idx = getColChannelIndex(channel);
    if (seen[idx]) return;
    seen[idx] = true;
    out[(*count)++] = idx;
}


// collects the array indices of every color channel that the level can touch
// and returns how many unique indices were written to 'out'
static int collectUsedChannels(int *out) {
    int n = 0;
    bool seen[static_cast<int>(ColorChannelIDs::ColChannelNum)] = { false };

    // normal player channels
    addUsedChannel(out, seen, &n, static_cast<int>(ColorChannelIDs::None));
    addUsedChannel(out, seen, &n, static_cast<int>(ColorChannelIDs::Col1));
    addUsedChannel(out, seen, &n, static_cast<int>(ColorChannelIDs::Col2));
    addUsedChannel(out, seen, &n, static_cast<int>(ColorChannelIDs::Col3));
    addUsedChannel(out, seen, &n, static_cast<int>(ColorChannelIDs::Col4));

    // built-in special channels
    for (int id = static_cast<int>(ColorChannelIDs::ChannelBg); id < static_cast<int>(ColorChannelIDs::ColChannelLast); id++) {
        addUsedChannel(out, seen, &n, id);
    }

    // channels declared in the level
    for (int i = 0; i < channelCount && n < static_cast<int>(ColorChannelIDs::ColChannelNum); i++) {
        addUsedChannel(out, seen, &n, colorChannels[i].channelID);
        addUsedChannel(out, seen, &n, colorChannels[i].inheritedChannelID);
    }

    // check for objects
    for (int i = 0; i < objects.count && n < static_cast<int>(ColorChannelIDs::ColChannelNum); i++) {
        addUsedChannel(out, seen, &n, objects.colChannel[i]);
        addUsedChannel(out, seen, &n, objects.detailColChannel[i]);
        addUsedChannel(out, seen, &n, objects.v1p9ColChannel[i]);

        if (isColorTrigger(objects.id[i])) {
            ColorTrigger *trigger = getColorTrigger(i);
            addUsedChannel(out, seen, &n, trigger->targetColorID);
            addUsedChannel(out, seen, &n, trigger->copiedColorID);
        } else if (objects.id[i] == PULSE_TRIGGER) {
            PulseTrigger *trigger = getPulseTrigger(i);
            if (trigger->pulseTargetType == PULSE_TARGET_CHANNEL)
                addUsedChannel(out, seen, &n, trigger->targetGroup);
            addUsedChannel(out, seen, &n, trigger->copiedColorID);
        }
    }

    return n;
}

void loadUsedChannels() {
    usedCount = collectUsedChannels(channelsUsed);
}

// frees the dynamic snapshot buffers of a checkpoint
#define CK_ALIGN8(x) (((x) + 7u) & ~(size_t)7u)

static unsigned int ckFloatBits(float f) {
    unsigned int u;
    memcpy(&u, &f, sizeof u);
    return u;
}

static int ckVarintLen(unsigned int v) {
    int n = 1;
    while (v >= 128u) { v >>= 7; n++; }
    return n;
}
static uint8_t *ckVarintPut(uint8_t *p, unsigned int v) {
    while (v >= 128u) { *p++ = (uint8_t)(v | 0x80u); v >>= 7; }
    *p++ = (uint8_t)v;
    return p;
}
static unsigned int ckVarintGet(const uint8_t **pp) {
    unsigned int v = 0;
    int shift = 0;
    const uint8_t *p = *pp;
    for (;;) {
        uint8_t b = *p++;
        v |= (unsigned int)(b & 0x7Fu) << shift;
        if (!(b & 0x80u)) break;
        shift += 7;
    }
    *pp = p;
    return v;
}

static uint32_t ckNextPow2(uint32_t x) {
    uint32_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

static int ckDedupPutH(int *h, uint32_t mask, float *tbl, int *n, float v) {
    if (*n > 65536) return 0;
    uint32_t bits = ckFloatBits(v);
    uint32_t i = (bits * 2654435761u) & mask;
    for (;;) {
        int val = h[i];
        if (val == 0) {
            h[i] = *n + 1;
            tbl[*n] = v;
            return (*n)++;
        }
        if (ckFloatBits(tbl[val - 1]) == bits) return val - 1;
        i = (i + 1) & mask;
    }
}

static void ckAxisChoose(int c, int u, int *fmt, int *bytes) {
    int best = (int)(sizeof(float) * (size_t)c);
    int bestFmt = 0;
    if (u > 0 && u <= 256) {
        int b = (int)(sizeof(float) * (size_t)u) + c;
        if (b < best) { best = b; bestFmt = 1; }
    }
    if (u > 0 && u <= 65536) {
        int b = (int)(sizeof(float) * (size_t)u) + 2 * c;
        if (b < best) { best = b; bestFmt = 2; }
    }
    *fmt = bestFmt;
    *bytes = best;
}

static float ckAxisGet(const uint8_t *blob, int fmt, int u, int k) {
    if (fmt == 0) {
        float v;
        memcpy(&v, blob + (size_t)k * sizeof(float), sizeof v);
        return v;
    }
    const float *tbl = (const float *)blob;
    unsigned int idx;
    if (fmt == 1) idx = ((const uint8_t *)(blob + (size_t)u * sizeof(float)))[k];
    else          idx = ((const uint16_t *)(blob + (size_t)u * sizeof(float)))[k];
    if (idx >= (unsigned int)u) idx = 0;
    return tbl[idx];
}

static void freeCheckpointSnapshot(CheckpointData *check) {
    if (check->allocBase) { free(check->allocBase); check->allocBase = NULL; }

    memset(&check->channelSnapshotCount, 0,
           (size_t)((char *)&check->pulseSnap + sizeof(check->pulseSnap)
                    - (char *)&check->channelSnapshotCount));
}

void setCheckpointTimer(float timer) {
    if (state.practiceMode && settingsState.autoCheckpoints) {
        // Set auto checkpoints timer
        checkpointTimer = timer;

        // Quick checkpoints halves the timer
        if (settingsState.quickCheckpoints) {
            checkpointTimer /= 2;
        }
    }
}

static bool isCheckpointPickup(int obj) {
    PickupItem *pickup = getPickupItem(obj);
    return pickup && objects.id[obj] != KEY_OBJ;
}

static bool isCollectedPickup(int obj) {
    return isCheckpointPickup(obj) &&
           (objects.flags[obj] & static_cast<int>(ObjectFlags::FlagToggled)) && (objects.activated[obj] & 3);
}

void newCheckpoint() {
    if (state.dead) return;

    flushGroupCombinationVisualMoves();

    int nextCheckpoint = checkpointPointer + 1;
    if (nextCheckpoint >= MAX_CHECKPOINTS) nextCheckpoint = 0;

    CheckpointData *check = &checkpoints[nextCheckpoint];

    // release the snapshot of the checkpoint we are about to overwrite
    freeCheckpointSnapshot(check);

    int moveCap = 0;
    int moveCx = 0;
    int moveCy = 0;
    int moveIdxBytes = 0;
    int prevIndex = 0;
    int *mv_list = NULL;
    if (objects.count > 0) mv_list = (int *)malloc(sizeof(int) * (size_t)objects.count);
    for (int i = 0; i < objects.count; i++) {
        unsigned int bx = ckFloatBits(objects.x[i]) ^ ckFloatBits(objects.originalX[i]);
        unsigned int by = ckFloatBits(objects.y[i]) ^ ckFloatBits(objects.originalY[i]);
        if (!bx && !by) continue;
        unsigned int gap = (unsigned int)(moveCap ? (i - prevIndex) : i);
        unsigned int flags = (bx ? 2u : 0u) | (by ? 1u : 0u);
        moveIdxBytes += ckVarintLen((gap << 2) | flags);
        prevIndex = i;
        if (mv_list) mv_list[moveCap] = i;
        moveCap++;
        if (bx) moveCx++;
        if (by) moveCy++;
    }

    int moveXFmt = 0, moveYFmt = 0, moveXU = 0, moveYU = 0;
    int moveXBytes = 0, moveYBytes = 0;
    int *ck_hx = NULL, *ck_hy = NULL;
    uint32_t ckHxSlots = 0, ckHySlots = 0;
    if (mv_list && (moveCx > 0 || moveCy > 0)) {
        float *tbl = (float *)malloc(sizeof(float) * (size_t)(moveCx + moveCy));
        if (tbl) {
            uint32_t xb = moveCx > 65537 ? 65537u : (uint32_t)moveCx;
            uint32_t yb = moveCy > 65537 ? 65537u : (uint32_t)moveCy;
            ckHxSlots = xb ? ckNextPow2(xb * 2u) : 0;
            ckHySlots = yb ? ckNextPow2(yb * 2u) : 0;
            if (ckHxSlots) { ck_hx = (int *)malloc(sizeof(int) * ckHxSlots); if (ck_hx) memset(ck_hx, 0, sizeof(int) * ckHxSlots); }
            if (ckHySlots) { ck_hy = (int *)malloc(sizeof(int) * ckHySlots); if (ck_hy) memset(ck_hy, 0, sizeof(int) * ckHySlots); }
            float *tx = tbl;
            float *ty = tbl + moveCx;
            for (int e = 0; e < moveCap; e++) {
                int i = mv_list[e];
                if (ck_hx && (ckFloatBits(objects.x[i]) ^ ckFloatBits(objects.originalX[i])))
                    ckDedupPutH(ck_hx, ckHxSlots - 1u, tx, &moveXU, objects.x[i]);
                if (ck_hy && (ckFloatBits(objects.y[i]) ^ ckFloatBits(objects.originalY[i])))
                    ckDedupPutH(ck_hy, ckHySlots - 1u, ty, &moveYU, objects.y[i]);
            }
            ckAxisChoose(moveCx, moveXU, &moveXFmt, &moveXBytes);
            ckAxisChoose(moveCy, moveYU, &moveYFmt, &moveYBytes);
            free(tbl);
        }
    }
    if (moveXFmt == 0) moveXBytes = (int)(sizeof(float) * (size_t)moveCx);
    if (moveYFmt == 0) moveYBytes = (int)(sizeof(float) * (size_t)moveCy);

    int firedCap = 0;
    int keyCap = 0;
    for (int i = 0; i < objects.count; i++) {
        if ((objects.id[i] == MOVE_TRIGGER || objects.id[i] == ROTATE_TRIGGER) && GET_ACTIVATED(i))
            firedCap++;
        else if (objects.id[i] == KEY_OBJ && (objects.flags[i] & static_cast<int>(ObjectFlags::FlagToggled))
                 && (objects.activated[i] & 3))
            keyCap++;
    }

    int toggleCap = 0;
    for (int g = 1; g < MAX_GROUPS; g++) {
        GroupNode *p = getGroup(g);
        if (p && p->toggled) toggleCap++;
    }

    int alphaGroupCap = 0;
    for (int g = 1; g < MAX_GROUPS; g++) {
        GroupNode *p = getGroup(g);
        if (p && p->alpha != 1.0f) alphaGroupCap++;
    }

    int pulseCap = 0;
    for (int i = 0; i < pulseTriggerCount; i++)
        if (pulseTriggerBuffer[i].active) pulseCap++;

    int rotateTargetsTotal = 0;
    for (int i = 0; i < rotateTriggerCount; i++)
        rotateTargetsTotal += rotateTriggerBuffer[i].targetCount;

    int rotateObjCap = 0;
    for (int i = 0; i < objects.count; i++) {
        if (objects.id[i] != ROTATE_TRIGGER) continue;
        RotateTrigger *rt = getRotateTrigger(i);
        GroupNode *targetGroup = getGroup(rt->targetGroup);
        GroupNode *centerGroup = getGroup(rt->centerGroup);
        if (targetGroup) rotateObjCap += targetGroup->count;
        if (centerGroup) rotateObjCap += centerGroup->count;
    }

    int pickupCap = 0;
    for (int i = 0; i < objects.count; i++) {
        if (isCollectedPickup(i)) pickupCap++;
    }

    void *ckPtr[24];
    size_t ckSize[24];
    int ckN = 0;
    size_t total = 0;
    void *base = NULL;
#define CK_ADD(pp, type, count) do { \
        if ((count) > 0) { \
            (pp) = NULL; \
            ckPtr[ckN] = (void *)&(pp); \
            ckSize[ckN] = CK_ALIGN8(sizeof(type) * (size_t)(count)); \
            total += ckSize[ckN]; \
            ckN++; \
        } else { \
            (pp) = NULL; \
        } \
    } while (0)

    CK_ADD(check->moveTriggers,     MoveTriggerBuffer,  moveTriggerCount);
    CK_ADD(check->alphaTriggers,    AlphaTriggerBuffer, alphaTriggerCount);
    CK_ADD(check->spawnTriggers,    SpawnTriggerBuffer, spawnTriggerCount);
    CK_ADD(check->followTriggers,   FollowPlayerYTriggerBuffer, followPlayerYTriggerCount);
    CK_ADD(check->followTriggerBuffers, FollowTriggerBuffer, followTriggerCount);
    CK_ADD(check->rotateTriggers,   RotateTriggerBuffer, rotateTriggerCount);
    CK_ADD(check->rotateTargets,    RotateTargetState,   rotateTargetsTotal);
    CK_ADD(check->rotateTargetOffsets, int,            rotateTriggerCount);
    CK_ADD(check->rotateObjSnapshot, RotateObjectSnapshot, rotateObjCap);
    CK_ADD(check->countItems,       ItemCount,           itemCounterCount);
    CK_ADD(check->countTriggers,    CountTriggerBuffer,  countTriggerCount);
    CK_ADD(check->touchTriggers,    TouchTriggerBuffer,  touchTriggerCount);
    CK_ADD(check->collectedPickups, int,                 pickupCap);
    CK_ADD(check->channelSnapshot,  ColorChannel,       usedCount);
    CK_ADD(check->triggerSnapshot,  ColTriggerBuffer,   usedCount);
    CK_ADD(check->moveIdxStream,   uint8_t,            moveIdxBytes);
    CK_ADD(check->moveObjX,        uint8_t,            moveXBytes);
    CK_ADD(check->moveObjY,        uint8_t,            moveYBytes);
    CK_ADD(check->firedMoveIndex,  int,                firedCap);
    CK_ADD(check->keyIndex,         int,                keyCap);
    CK_ADD(check->toggleGroupID,   int,                toggleCap);
    CK_ADD(check->alphaGroupID,    int,                alphaGroupCap);
    CK_ADD(check->alphaGroupAlpha, float,              alphaGroupCap);
    CK_ADD(check->pulseSnap,        PulseSnapshot,      pulseCap);
#undef CK_ADD

    if (total > 0) {
        base = malloc(total);
        if (!base) {
            free(mv_list);
            free(ck_hx);
            free(ck_hy);
            freeCheckpointSnapshot(check);
            return;
        }
        size_t off = 0;
        for (int k = 0; k < ckN; k++) {
            *(void **)ckPtr[k] = (char *)base + off;
            off += ckSize[k];
        }
    }
    check->allocBase = base;

    checkpointPointer = nextCheckpoint;
    if (++checkpointCount > MAX_CHECKPOINTS) checkpointCount = MAX_CHECKPOINTS;

    check->moveTriggersCount  = moveTriggerCount;
    check->alphaTriggersCount = alphaTriggerCount;
    check->spawnTriggersCount = spawnTriggerCount;
    check->followTriggersCount = followPlayerYTriggerCount;
    check->followTriggerBuffersCount = followTriggerCount;
    check->rotateTriggersCount = rotateTriggerCount;
    check->rotateTargetsCount = rotateTargetsTotal;
    check->rotateObjCount = 0;
    check->countItemsCount = itemCounterCount;
    check->countTriggersCount = countTriggerCount;
    check->touchTriggersCount = touchTriggerCount;
    check->collectedPickupsCount = pickupCap;
    check->channelSnapshotCount = usedCount;

    check->cameraX = state.cameraX;
    check->cameraY = state.cameraY;

    check->p1 = state.player;
    check->p2 = state.player2;

    check->cameraIntendedY = state.cameraIntendedY;

    check->groundY = state.groundY;
    check->ceilingY = state.ceilingY;
    check->groundYGfx = state.groundYGfx;

    check->mirroring = state.mirroring;
    check->mirrorMult = state.mirrorMult;
    check->mirrorTimer = state.mirrorTimer;
    check->originalMirrorFactor = state.originalMirrorFactor;
    check->intendedMirrorFactor = state.intendedMirrorFactor;
    check->mirrorSpeedFactor = state.mirrorSpeedFactor;
    check->mirrorFactor = state.mirrorFactor;

    check->dual = state.dual;
    check->dualPortalY = state.dualPortalY;

    check->showPlayer = state.showPlayer;

    check->speed = state.speed;

    check->currentFadingEffect = currentFadingEffect;
    check->p1Trail = p1Trail;

    check->wallY = levelInfo.wallY;

    check->songOffset = levelInfo.songOffset + state.player.timeElapsed;

    // snapshot only the channels used by this level
    for (int i = 0; i < usedCount; i++) {
        int idx = channelsUsed[i];
        check->channelSnapshot[i] = channels[idx];
        check->triggerSnapshot[i] = colTriggerBuffer[idx];
    }

    if (check->moveTriggers)  memcpy(check->moveTriggers,  moveTriggerBuffer,  sizeof(MoveTriggerBuffer)  * moveTriggerCount);
    if (check->alphaTriggers) memcpy(check->alphaTriggers, alphaTriggerBuffer, sizeof(AlphaTriggerBuffer) * alphaTriggerCount);
    if (check->spawnTriggers) memcpy(check->spawnTriggers, spawnTriggerBuffer, sizeof(SpawnTriggerBuffer) * spawnTriggerCount);
    if (check->followTriggers) memcpy(check->followTriggers, followPlayerYTriggerBuffer, sizeof(FollowPlayerYTriggerBuffer) * followPlayerYTriggerCount);
    if (check->followTriggerBuffers) memcpy(check->followTriggerBuffers, followTriggerBuffer, sizeof(FollowTriggerBuffer) * followTriggerCount);
    if (check->rotateTriggers) memcpy(check->rotateTriggers, rotateTriggerBuffer, sizeof(RotateTriggerBuffer) * rotateTriggerCount);
    if (check->countItems) memcpy(check->countItems, itemCounterBuffer, sizeof(ItemCount) * itemCounterCount);
    if (check->countTriggers) memcpy(check->countTriggers, countTriggerBuffer, sizeof(CountTriggerBuffer) * countTriggerCount);
    if (check->touchTriggers) memcpy(check->touchTriggers, touchTriggerBuffer, sizeof(TouchTriggerBuffer) * touchTriggerCount);
    if (check->collectedPickups) {
        int n = 0;
        for (int i = 0; i < objects.count && n < check->collectedPickupsCount; i++) {
            if (isCollectedPickup(i)) check->collectedPickups[n++] = i;
        }
    }

    if (check->rotateTriggers) {
        int offset = 0;
        for (int i = 0; i < rotateTriggerCount; i++) {
            check->rotateTargetOffsets[i] = offset;
            int count = rotateTriggerBuffer[i].targetCount;
            if (count > 0 && rotateTriggerBuffer[i].targets) {
                memcpy(check->rotateTargets + offset, rotateTriggerBuffer[i].targets, sizeof(RotateTargetState) * count);
            }
            offset += count;
        }
    }

    if (check->rotateObjSnapshot) {
        int n = 0;
        for (int i = 0; i < objects.count && n < rotateObjCap; i++) {
            if (objects.id[i] != ROTATE_TRIGGER) continue;
            RotateTrigger *rt = getRotateTrigger(i);
            GroupNode *targetGroup = getGroup(rt->targetGroup);
            GroupNode *centerGroup = getGroup(rt->centerGroup);
            if (targetGroup) {
                for (int j = 0; j < targetGroup->count && n < rotateObjCap; j++) {
                    int obj = targetGroup->objects[j];
                    check->rotateObjSnapshot[n].obj = obj;
                    check->rotateObjSnapshot[n].rotation = objects.rotation[obj];
                    check->rotateObjSnapshot[n].visualRotation = objects.visualRotation[obj];
                    n++;
                }
            }
            if (centerGroup) {
                for (int j = 0; j < centerGroup->count && n < rotateObjCap; j++) {
                    int obj = centerGroup->objects[j];
                    check->rotateObjSnapshot[n].obj = obj;
                    check->rotateObjSnapshot[n].rotation = objects.rotation[obj];
                    check->rotateObjSnapshot[n].visualRotation = objects.visualRotation[obj];
                    n++;
                }
            }
        }
        check->rotateObjCount = n;
    }

    check->moveObjCount = 0;
    check->moveIdxBytes = 0;
    check->moveCx = 0;
    check->moveCy = 0;
    check->moveXFmt = moveXFmt;
    check->moveXU = moveXU;
    check->moveXBytes = moveXBytes;
    check->moveYFmt = moveYFmt;
    check->moveYU = moveYU;
    check->moveYBytes = moveYBytes;
    if (moveCap > 0) {
        int n = 0, kx = 0, ky = 0, prev = 0, scan = 0, tux = 0, tuy = 0;
        if (ck_hx && moveXFmt != 0) memset(ck_hx, 0, sizeof(int) * ckHxSlots);
        if (ck_hy && moveYFmt != 0) memset(ck_hy, 0, sizeof(int) * ckHySlots);
        uint8_t *sp = check->moveIdxStream;
        float *tblx = (float *)check->moveObjX;
        float *tbly = (float *)check->moveObjY;
        uint8_t  *ix8x  = check->moveObjX ? (uint8_t  *)(check->moveObjX + sizeof(float) * (size_t)moveXU) : NULL;
        uint16_t *ix16x = check->moveObjX ? (uint16_t *)(check->moveObjX + sizeof(float) * (size_t)moveXU) : NULL;
        uint8_t  *ix8y  = check->moveObjY ? (uint8_t  *)(check->moveObjY + sizeof(float) * (size_t)moveYU) : NULL;
        uint16_t *ix16y = check->moveObjY ? (uint16_t *)(check->moveObjY + sizeof(float) * (size_t)moveYU) : NULL;
        for (int e = 0; e < moveCap; e++) {
            int i;
            if (mv_list) {
                i = mv_list[e];
            } else {
                do { i = scan++; } while (i < objects.count &&
                        !(ckFloatBits(objects.x[i]) ^ ckFloatBits(objects.originalX[i])) &&
                        !(ckFloatBits(objects.y[i]) ^ ckFloatBits(objects.originalY[i])));
            }
            unsigned int bx = ckFloatBits(objects.x[i]) ^ ckFloatBits(objects.originalX[i]);
            unsigned int by = ckFloatBits(objects.y[i]) ^ ckFloatBits(objects.originalY[i]);
            unsigned int gap = (unsigned int)(n ? (i - prev) : i);
            unsigned int flags = (bx ? 2u : 0u) | (by ? 1u : 0u);
            sp = ckVarintPut(sp, (gap << 2) | flags);
            prev = i;
            if (bx) {
                float v = objects.x[i];
                if (moveXFmt == 0) {
                    tblx[kx] = v;
                } else {
                    int ix = ckDedupPutH(ck_hx, ckHxSlots - 1u, tblx, &tux, v);
                    if (moveXFmt == 1) ix8x[kx] = (uint8_t)ix; else ix16x[kx] = (uint16_t)ix;
                }
                kx++;
            }
            if (by) {
                float v = objects.y[i];
                if (moveYFmt == 0) {
                    tbly[ky] = v;
                } else {
                    int iy = ckDedupPutH(ck_hy, ckHySlots - 1u, tbly, &tuy, v);
                    if (moveYFmt == 1) ix8y[ky] = (uint8_t)iy; else ix16y[ky] = (uint16_t)iy;
                }
                ky++;
            }
            n++;
        }
        check->moveObjCount = n;
        check->moveIdxBytes = (int)(sp - check->moveIdxStream);
        check->moveCx = kx;
        check->moveCy = ky;
    }
    free(mv_list);
    mv_list = NULL;
    free(ck_hx);
    ck_hx = NULL;
    free(ck_hy);
    ck_hy = NULL;

    check->firedMoveCount = 0;
    if (firedCap > 0) {
        int n = 0;
        for (int i = 0; i < objects.count; i++) {
            if (n >= firedCap) break;
            if ((objects.id[i] == MOVE_TRIGGER || objects.id[i] == ROTATE_TRIGGER) && GET_ACTIVATED(i))
                check->firedMoveIndex[n++] = i;
        }
        check->firedMoveCount = n;
    }

    check->keyCount = 0;
    if (keyCap > 0) {
        int n = 0;
        for (int i = 0; i < objects.count; i++) {
            if (n >= keyCap) break;
            if (objects.id[i] == KEY_OBJ && (objects.flags[i] & static_cast<int>(ObjectFlags::FlagToggled))
                && (objects.activated[i] & 3))
                check->keyIndex[n++] = i;
        }
        check->keyCount = n;
    }

    check->toggleGroupCount = 0;
    if (toggleCap > 0) {
        int n = 0;
        for (int g = 1; g < MAX_GROUPS && n < toggleCap; g++) {
            GroupNode *p = getGroup(g);
            if (p && p->toggled) check->toggleGroupID[n++] = g;
        }
        check->toggleGroupCount = n;
    }

    check->alphaGroupCount = 0;
    if (alphaGroupCap > 0) {
        int n = 0;
        for (int g = 1; g < MAX_GROUPS && n < alphaGroupCap; g++) {
            GroupNode *p = getGroup(g);
            if (p && p->alpha != 1.0f) {
                check->alphaGroupID[n] = g;
                check->alphaGroupAlpha[n] = p->alpha;
                n++;
            }
        }
        check->alphaGroupCount = n;
    }

    check->pulseSnapCount = 0;
    if (pulseCap > 0) {
        int n = 0;
        for (int i = 0; i < pulseTriggerCount && n < pulseCap; i++) {
            PulseTriggerBuffer *pb = &pulseTriggerBuffer[i];
            if (!pb->active) continue;
            check->pulseSnap[n].sourceObj = pb->sourceObj;
            check->pulseSnap[n].timeRun = pb->timeRun;
            n++;
        }
        check->pulseSnapCount = n;
    }

    setCheckpointTimer(AUTO_CHECKPOINT_TIME);
}

void restoreCheckpoint() {
    CheckpointData *check = &checkpoints[checkpointPointer];
    discardGroupCombinationVisualMoves();

    state.cameraX = check->cameraX;
    state.cameraY = check->cameraY;

    state.player = check->p1;
    state.player2 = check->p2;
    state.player.platformVelocity = state.player.lastPlatformVelocity = 0.f;
    state.player2.platformVelocity = state.player2.lastPlatformVelocity = 0.f;

    state.player.bufferingState = (state.input.holdJump ? static_cast<int>(BufferingState::BufferReady) : static_cast<int>(BufferingState::BufferNone));
    state.player2.bufferingState = (state.input.holdJump ? static_cast<int>(BufferingState::BufferReady) : static_cast<int>(BufferingState::BufferNone));

    state.player.bufferUfo = true;
    state.player2.bufferUfo = true;

    state.cameraIntendedY = check->cameraIntendedY;

    state.groundY = check->groundY;
    state.ceilingY = check->ceilingY;
    state.groundYGfx = check->groundYGfx;

    state.mirroring = check->mirroring;
    state.mirrorMult = check->mirrorMult;
    state.mirrorTimer = check->mirrorTimer;
    state.originalMirrorFactor = check->originalMirrorFactor;
    state.intendedMirrorFactor = check->intendedMirrorFactor;
    state.mirrorSpeedFactor = check->mirrorSpeedFactor;
    state.mirrorFactor = check->mirrorFactor;
    
    state.dual = check->dual;
    state.dualPortalY = check->dualPortalY;
    
    state.showPlayer = check->showPlayer;

    state.speed = check->speed;

    levelInfo.wallY = check->wallY;

    currentFadingEffect = check->currentFadingEffect;
    p1Trail = check->p1Trail;
    
    if (settingsState.practiceMusicSync) seekMP3(check->songOffset);
    
    for (int i = 0; i < check->channelSnapshotCount; i++) {
        int idx = channelsUsed[i];
        channels[idx] = check->channelSnapshot[i];
        colTriggerBuffer[idx] = check->triggerSnapshot[i];
    }

    if (check->moveTriggers && check->moveTriggersCount > 0) {
        MoveTriggerBuffer *buf = (MoveTriggerBuffer *)realloc(moveTriggerBuffer, sizeof(MoveTriggerBuffer) * check->moveTriggersCount);
        if (buf) {
            moveTriggerBuffer = buf;
            moveTriggerCapacity = check->moveTriggersCount;
            moveTriggerCount = check->moveTriggersCount;
            memcpy(moveTriggerBuffer, check->moveTriggers, sizeof(MoveTriggerBuffer) * check->moveTriggersCount);
        }
    }
    if (check->alphaTriggers && check->alphaTriggersCount > 0) {
        AlphaTriggerBuffer *buf = (AlphaTriggerBuffer *)realloc(alphaTriggerBuffer, sizeof(AlphaTriggerBuffer) * check->alphaTriggersCount);
        if (buf) {
            alphaTriggerBuffer = buf;
            alphaTriggerCapacity = check->alphaTriggersCount;
            alphaTriggerCount = check->alphaTriggersCount;
            memcpy(alphaTriggerBuffer, check->alphaTriggers, sizeof(AlphaTriggerBuffer) * check->alphaTriggersCount);
        }
    }
    if (check->spawnTriggers && check->spawnTriggersCount > 0) {
        SpawnTriggerBuffer *buf = (SpawnTriggerBuffer *)realloc(spawnTriggerBuffer, sizeof(SpawnTriggerBuffer) * check->spawnTriggersCount);
        if (buf) {
            spawnTriggerBuffer = buf;
            spawnTriggerCapacity = check->spawnTriggersCount;
            spawnTriggerCount = check->spawnTriggersCount;
            memcpy(spawnTriggerBuffer, check->spawnTriggers, sizeof(SpawnTriggerBuffer) * check->spawnTriggersCount);
        }
    }

    if (check->followTriggers && check->followTriggersCount > 0) {
        FollowPlayerYTriggerBuffer *buf = (FollowPlayerYTriggerBuffer *)realloc(followPlayerYTriggerBuffer, sizeof(FollowPlayerYTriggerBuffer) * check->followTriggersCount);
        if (buf) {
            followPlayerYTriggerBuffer = buf;
            followPlayerYTriggerCapacity = check->followTriggersCount;
            followPlayerYTriggerCount = check->followTriggersCount;
            memcpy(followPlayerYTriggerBuffer, check->followTriggers, sizeof(FollowPlayerYTriggerBuffer) * check->followTriggersCount);
        }
    }

    followTriggerCount = 0;
    if (check->followTriggerBuffers && check->followTriggerBuffersCount > 0) {
        FollowTriggerBuffer *buf = (FollowTriggerBuffer *)realloc(followTriggerBuffer, sizeof(FollowTriggerBuffer) * check->followTriggerBuffersCount);
        if (buf) {
            followTriggerBuffer = buf;
            followTriggerCapacity = check->followTriggerBuffersCount;
            followTriggerCount = check->followTriggerBuffersCount;
            memcpy(followTriggerBuffer, check->followTriggerBuffers, sizeof(FollowTriggerBuffer) * check->followTriggerBuffersCount);
        }
    }

    for (int i = 0; i < rotateTriggerCount; i++) {
        free(rotateTriggerBuffer[i].targets);
        rotateTriggerBuffer[i].targets = NULL;
    }
    rotateTriggerCount = 0;
    if (check->rotateTriggers && check->rotateTriggersCount > 0) {
        RotateTriggerBuffer *buf = (RotateTriggerBuffer *)realloc(rotateTriggerBuffer, sizeof(RotateTriggerBuffer) * check->rotateTriggersCount);
        if (buf) {
            rotateTriggerBuffer = buf;
            rotateTriggerCapacity = check->rotateTriggersCount;
            rotateTriggerCount = check->rotateTriggersCount;
            memcpy(rotateTriggerBuffer, check->rotateTriggers, sizeof(RotateTriggerBuffer) * check->rotateTriggersCount);
            for (int i = 0; i < rotateTriggerCount; i++) {
                RotateTriggerBuffer *rb = &rotateTriggerBuffer[i];
                int count = rb->targetCount;
                int offset = check->rotateTargetOffsets[i];
                rb->targets = NULL;
                if (count > 0 && offset >= 0 && offset + count <= check->rotateTargetsCount) {
                    rb->targets = (RotateTargetState *)malloc(sizeof(RotateTargetState) * count);
                    if (rb->targets) {
                        memcpy(rb->targets, check->rotateTargets + offset, sizeof(RotateTargetState) * count);
                    } else {
                        rb->targetCount = 0;
                        rb->active = false;
                    }
                } else {
                    rb->targetCount = 0;
                }
            }
        }
    }

    for (int i = 0; i < objects.count; i++) {
        PickupItem *pickup = getPickupItem(i);
        if (pickup) {
            objects.flags[i] &= ~static_cast<int>(ObjectFlags::FlagToggled);
            objects.activated[i] = 0;
        }
    }
    for (int i = 0; i < check->collectedPickupsCount; i++) {
        int obj = check->collectedPickups[i];
        if (obj >= 0 && obj < objects.count && isCheckpointPickup(obj)) {
            objects.flags[obj] |= static_cast<int>(ObjectFlags::FlagToggled);
            objects.activated[obj] = 3;
        }
    }

    if (check->countItemsCount > 0) {
        ItemCount *buf = (ItemCount *)realloc(itemCounterBuffer, sizeof(ItemCount) * check->countItemsCount);
        if (buf) {
            itemCounterBuffer = buf;
            itemCounterCapacity = check->countItemsCount;
            itemCounterCount = check->countItemsCount;
            memcpy(itemCounterBuffer, check->countItems, sizeof(ItemCount) * check->countItemsCount);
        }
    } else {
        itemCounterCount = 0;
    }
    if (check->countTriggersCount > 0) {
        CountTriggerBuffer *buf = (CountTriggerBuffer *)realloc(countTriggerBuffer, sizeof(CountTriggerBuffer) * check->countTriggersCount);
        if (buf) {
            countTriggerBuffer = buf;
            countTriggerCapacity = check->countTriggersCount;
            countTriggerCount = check->countTriggersCount;
            memcpy(countTriggerBuffer, check->countTriggers, sizeof(CountTriggerBuffer) * check->countTriggersCount);
        }
    } else {
        countTriggerCount = 0;
    }
    if (check->touchTriggersCount > 0) {
        TouchTriggerBuffer *buf = (TouchTriggerBuffer *)realloc(touchTriggerBuffer, sizeof(TouchTriggerBuffer) * check->touchTriggersCount);
        if (buf) {
            touchTriggerBuffer = buf;
            touchTriggerCapacity = check->touchTriggersCount;
            touchTriggerCount = check->touchTriggersCount;
            memcpy(touchTriggerBuffer, check->touchTriggers, sizeof(TouchTriggerBuffer) * check->touchTriggersCount);
        }
    } else {
        touchTriggerCount = 0;
    }

    for (int i = 0; i < countTriggerCount; i++)
        countTriggerBuffer[i].restoredFromCheckpoint = countTriggerBuffer[i].active;

    for (int i = 0; i < moveTriggerCount; i++)
        moveTriggerBuffer[i].restoredFromCheckpoint = moveTriggerBuffer[i].active;
    for (int i = 0; i < alphaTriggerCount; i++)
        alphaTriggerBuffer[i].restoredFromCheckpoint = alphaTriggerBuffer[i].active;
    for (int i = 0; i < spawnTriggerCount; i++)
        spawnTriggerBuffer[i].restoredFromCheckpoint = spawnTriggerBuffer[i].active;
    for (int i = 0; i < followPlayerYTriggerCount; i++)
        followPlayerYTriggerBuffer[i].restoredFromCheckpoint = followPlayerYTriggerBuffer[i].active;
    for (int i = 0; i < followTriggerCount; i++)
        followTriggerBuffer[i].restoredFromCheckpoint = followTriggerBuffer[i].active;
    for (int i = 0; i < rotateTriggerCount; i++)
        rotateTriggerBuffer[i].restoredFromCheckpoint = rotateTriggerBuffer[i].active;

    {
        int kx = 0, ky = 0, oi = 0;
        const uint8_t *sp = check->moveIdxStream;
        const uint8_t *spEnd = sp + check->moveIdxBytes;
        for (int i = 0; i < check->moveObjCount; i++) {
            if (sp >= spEnd) break;
            unsigned int v = ckVarintGet(&sp);
            unsigned int gap = v >> 2;
            unsigned int flags = v & 3u;
            oi = (i == 0) ? (int)gap : (oi + (int)gap);
            if (flags & 2u) {
                float vx = ckAxisGet(check->moveObjX, check->moveXFmt, check->moveXU, kx++);
                objects.x[oi] = vx;
            }
            if (flags & 1u) {
                float vy = ckAxisGet(check->moveObjY, check->moveYFmt, check->moveYU, ky++);
                objects.y[oi] = vy; objects.lastY[oi] = vy;
            }
            updateObjectSection(oi);
        }
    }
    for (int i = 0; i < check->rotateObjCount && check->rotateObjSnapshot; i++) {
        RotateObjectSnapshot *rs = &check->rotateObjSnapshot[i];
        objects.lastY[rs->obj] = objects.y[rs->obj];
        objects.rotation[rs->obj] = rs->rotation;
        objects.visualRotation[rs->obj] = rs->visualRotation;
    }
    for (int i = 0; i < check->alphaGroupCount; i++) {
        GroupNode *p = getGroup(check->alphaGroupID[i]);
        if (p) p->alpha = check->alphaGroupAlpha[i];
    }
    for (int i = 0; i < check->alphaGroupCount; i++) {
        GroupNode *p = getGroup(check->alphaGroupID[i]);
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.alphaTriggerOpacity[object] = getGroupOpacities(object);
        }
    }

    for (int i = 0; i < check->toggleGroupCount; i++) {
        GroupNode *p = getGroup(check->toggleGroupID[i]);
        if (!p) continue;
        p->toggled = true;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.flags[object] |= static_cast<int>(ObjectFlags::FlagToggled);
        }
    }

    for (int i = 0; i < check->firedMoveCount; i++) {
        SET_ACTIVATED(check->firedMoveIndex[i], true);
    }

    for (int i = 0; i < check->keyCount; i++) {
        SET_ACTIVATED(check->keyIndex[i], true);
        objects.flags[check->keyIndex[i]] |= static_cast<int>(ObjectFlags::FlagToggled);
    }

    pulseTriggerCount = 0;
    for (int i = 0; i < objects.count; i++) {
        objects.numMainPulses[i] = 0;
        objects.numDetailPulses[i] = 0;
        objects.mainBeingPulsed[i] = false;
        objects.detailBeingPulsed[i] = false;
    }
    for (int i = 0; i < static_cast<int>(ColorChannelIDs::ColChannelNum); i++) {
        channels[i].numPulses = 0;
    }

    for (int i = 0; i < check->pulseSnapCount; i++) {
        PulseSnapshot *ps = &check->pulseSnap[i];
        restorePulseTrigger(ps->sourceObj, ps->timeRun);
    }

    updateAttemptTextPos();

    setCheckpointTimer(AUTO_CHECKPOINT_TIME);
    restoredCheckpoint = true;
}

void deleteLastCheckpoint() {
    if (checkpointCount > 0) {
        freeCheckpointSnapshot(&checkpoints[checkpointPointer]);
        checkpointCount--;

        // Wrap around pointer
        if (checkpointPointer-- == 0) {
            checkpointPointer = MAX_CHECKPOINTS - 1;
        }
    }
}

void clearPracticeMode() {
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        freeCheckpointSnapshot(&checkpoints[i]);
    }
    checkpointCount = 0;
    checkpointPointer = 0;
    state.practiceMode = false;
}
void startPracticeMode() {
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        freeCheckpointSnapshot(&checkpoints[i]);
    }
    checkpointCount = 0;
    checkpointPointer = 0;
    pseudoCheckpointExists = false;
    state.practiceMode = true;
    
    if (!settingsState.practiceMusicSync) {
        stopMP3();
        playPracticeSong();
    }
}

void exitPracticeMode() {
    state.practiceMode = false;
    initVariables();
    reloadLevel(); 
    clearPracticeMode();

    if (settingsState.practiceMusicSync) {
        seekMP3(levelInfo.songOffset);
    } else {
        stopMP3();
        playLevelSong(levelInfo.songOffset);
    }
}

void runPendingCheckpoint() {
    if (!state.pendingAutoCheckpoint) {
        isPendingPseudoCheckpoint = false;
        return;
    }

    state.pendingAutoCheckpoint = false;

    bool isPseudo = isPendingPseudoCheckpoint;
    isPendingPseudoCheckpoint = false;

    if (state.dead) return;

    newCheckpoint();
    if (isPseudo) pseudoCheckpointExists = true;
}

void handleAutoCheckpoints(float delta) {
    // Exit if not in practice mode with autocheckpoints enabled
    if (!(state.practiceMode && settingsState.autoCheckpoints) || state.endWallAnimPlaying) return;
    
    if (checkpointTimer <= 0) {
        switch (state.player.gamemode) {
            case static_cast<int>(IconGamemode::GamemodePlayer):
            case static_cast<int>(IconGamemode::GamemodeBall):
            case static_cast<int>(IconGamemode::GamemodeRobot):
            case static_cast<int>(IconGamemode::GamemodeSpider):
                if (state.player.landedFromJump) {
                    state.pendingAutoCheckpoint = true;
                }
                break;

            case static_cast<int>(IconGamemode::GamemodeShip):
            case static_cast<int>(IconGamemode::GamemodeUfo):
            case static_cast<int>(IconGamemode::GamemodeWave):
                state.pendingAutoCheckpoint = true;
                isPendingPseudoCheckpoint = true;
                break;

            default:
                break;
        }
    } else {
        checkpointTimer -= delta;
    }
}

void handlePracticeMode() {
    if (!state.practiceMode) return;

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    if (((kDown & KEY_L) && !((kHeld & KEY_B) && settingsState.enableDebugBindings)) || (kDown & KEY_ZL)) {
        if (settingsState.autoCheckpoints  && playerGamemodeIsFlying(&state.player) && pseudoCheckpointExists) {
            pseudoCheckpointExists = false;
            deleteLastCheckpoint();
        }
        newCheckpoint();
    }

    if (((kDown & KEY_R) && !((kHeld & KEY_B) && settingsState.enableDebugBindings)) || (kDown & KEY_ZR)) {
        deleteLastCheckpoint();
    }
}

static void drawCheckpoint(float x, float y) {
    C2D_Sprite spr = { 0 };
    C2D_SpriteFromSheet(&spr, spriteSheet2, CHECKPOINT_GFX_ID);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetPos(&spr, getMirrorX(x, state.mirrorFactor), y);

    C2D_DrawSprite(&spr);
}

int getCheckpointCount() {
    int count = checkpointCount;
    if (settingsState.autoCheckpoints && playerGamemodeIsFlying(&state.player) && pseudoCheckpointExists) {
        count--;
    }
    return MAX(0, count);
}

void drawCheckpoints() {
    if (!state.practiceMode) return;

    int start = 0;

    if (settingsState.autoCheckpoints && playerGamemodeIsFlying(&state.player) && pseudoCheckpointExists)
        start++;

    for (int checkpoint = start; checkpoint < checkpointCount; checkpoint++) {
        // Obtain buffer index
        s32 index = WRAP((s32) (checkpointPointer - checkpoint), 0, MAX_CHECKPOINTS);
        CheckpointData *currCheckpoint = &checkpoints[index];

        float calcX = (currCheckpoint->p1.x - state.cameraX);
        float calcY = SCREEN_HEIGHT - ((currCheckpoint->p1.y - state.cameraY));  

        if (calcX < -60 || calcX >= (SCREEN_WIDTH / SCALE) + 60) continue;
        if (calcY < -60 || calcY >= (SCREEN_HEIGHT / SCALE) + 60) continue;

        drawCheckpoint(calcX, calcY);
    }
}

}
