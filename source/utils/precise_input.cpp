#include "precise_input.hpp"

#define INPUT_QUEUE_SIZE 16
#define RING_BUFFER_ENTRIES 8
#define PAD_RING_ENTRY_WORDS 4

#define TOUCH_RING_ENTRY_WORDS 2
#define MAX_SAMPLE_INTERVAL_TICKS ((u32)(CPU_TICKS_PER_MSEC * 100))

#define PAD_SECTION_OFFSET 0x0
#define TOUCH_SECTION_OFFSET 0xA8
#define SECTION_TICKS_OFFSET 0x0
#define SECTION_INDEX_OFFSET 0x10
#define PAD_RING_OFFSET 0x28
#define TOUCH_RING_OFFSET 0x20
#define WORD_INDEX(byte_offset) ((byte_offset) / sizeof(u32))

namespace gd3ds
{

bool piEnabled = false;

static u32 frameStart;
static u32 frameEnd;
static u32 frameSubsteps;

struct PreciseSource {
    u32 tickWord;
    u32 idxWord;

    u32 ringBufferOffset;

    bool (*sampleJump)(const struct PreciseSource *src, u32 slot);
    u32 jumpKeys;
    bool (*touchFilter)(u16 px, u16 py);

    PreciseInputEvent queue[INPUT_QUEUE_SIZE];
    u32 queueCount;
    u32 queueHead;

    u32 lastIdx;
    u32 lastPollTick;
    bool lastSampleJump;

    bool holdState;
    bool suppressHoldUntilRelease;
    bool fakePress;
    bool pressedEdge;
};

struct RingSnapshot {
    u32 now;
    u32 latestTick;
    u32 prevTick;
    u32 currentIdx;
};

static bool padSampleJump(const PreciseSource *src, u32 slot);
static bool touchSampleJump(const PreciseSource *src, u32 slot);

enum class InputSource { SourcePad, SourceTouch, SourceCount };

// https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
#define PAD_SOURCE { \
    .tickWord = WORD_INDEX(PAD_SECTION_OFFSET + SECTION_TICKS_OFFSET), \
    .idxWord = WORD_INDEX(PAD_SECTION_OFFSET + SECTION_INDEX_OFFSET), \
    .ringBufferOffset = PAD_SECTION_OFFSET + PAD_RING_OFFSET, \
    .sampleJump = padSampleJump, \
}

// https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0xA8
#define TOUCH_SOURCE { \
    .tickWord = WORD_INDEX(TOUCH_SECTION_OFFSET + SECTION_TICKS_OFFSET), \
    .idxWord = WORD_INDEX(TOUCH_SECTION_OFFSET + SECTION_INDEX_OFFSET), \
    .ringBufferOffset = TOUCH_SECTION_OFFSET + TOUCH_RING_OFFSET, \
    .sampleJump = touchSampleJump, \
}

static PreciseSource sources[static_cast<int>(PreciseInputPlayer::PiPlayerCount)][static_cast<int>(InputSource::SourceCount)] = {
    { [static_cast<int>(InputSource::SourcePad)] = PAD_SOURCE, [static_cast<int>(InputSource::SourceTouch)] = TOUCH_SOURCE },
    { [static_cast<int>(InputSource::SourcePad)] = PAD_SOURCE, [static_cast<int>(InputSource::SourceTouch)] = TOUCH_SOURCE },
};

static vu32 *getRingBuffer(const PreciseSource* src);
static u32 sampleInterval(u32 latestTick, u32 prevTick);
static u32 reconstructTick(u32 newestTime, u32 samplesAgo, u32 interval);
static bool pushEvent(PreciseSource *src, u32 tick, bool down);
static PreciseInputEvent peekEvent(PreciseSource *src);
static PreciseInputEvent popEvent(PreciseSource *src);
static RingSnapshot snapshotRing(const PreciseSource *src, u32 now);
static void pollSource(PreciseSource *src, const RingSnapshot *snapshot);
static void applySource(PreciseSource *src, u32 substep);
static void resyncFromRing(PreciseSource *src);
static u32 substepCutoff(u32 substep);

void piSetTouchFilter(PreciseInputPlayer player, bool (*filter)(u16 px, u16 py)){
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return;
    }
    sources[static_cast<int>(player)][static_cast<int>(InputSource::SourceTouch)].touchFilter = filter;
}

void piSetJumpKeys(PreciseInputPlayer player, u32 mask) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return;
    }
    sources[static_cast<int>(player)][static_cast<int>(InputSource::SourcePad)].jumpKeys = mask;
}

void piReset(void) {
    for (u32 player = 0; player < static_cast<int>(PreciseInputPlayer::PiPlayerCount); player++) {
        for (u32 kind = 0; kind < static_cast<int>(InputSource::SourceCount); kind++) {
            PreciseSource *src = &sources[static_cast<int>(player)][kind];
            resyncFromRing(src);
            src->suppressHoldUntilRelease = false;
            src->fakePress = false;
            src->pressedEdge = false;
        }
    }
}

void piSuppressUntilRelease(void) {
    for (u32 player = 0; player < static_cast<int>(PreciseInputPlayer::PiPlayerCount); player++) {
        for (u32 kind = 0; kind < static_cast<int>(InputSource::SourceCount); kind++) {
            sources[static_cast<int>(player)][kind].suppressHoldUntilRelease = true;
        }
    }
}

// poll to find the times that the clicks occured and schdule them for later application
void piPoll(void) {
    u32 now = (u32)(svcGetSystemTick());
    for (u32 kind = 0; kind < static_cast<int>(InputSource::SourceCount); kind++) {
        RingSnapshot snapshot = snapshotRing(&sources[static_cast<int>(PreciseInputPlayer::PiPlayer1)][kind], now);
        for (u32 player = 0; player < static_cast<int>(PreciseInputPlayer::PiPlayerCount); player++) {
            pollSource(&sources[static_cast<int>(player)][kind], &snapshot);
        }
    }
}

void piBeginFrame(u32 frameStartTick, u32 frameEndTick, u32 substeps) {
    frameStart = frameStartTick;
    frameEnd = frameEndTick;
    frameSubsteps = (substeps == 0) ? 1 : substeps;
}

void piApplySubstep(u32 substep) {
    for (u32 player = 0; player < static_cast<int>(PreciseInputPlayer::PiPlayerCount); player++) {
        for (u32 kind = 0; kind < static_cast<int>(InputSource::SourceCount); kind++) {
            applySource(&sources[static_cast<int>(player)][kind], substep);
        }
    }
}

bool piHold(PreciseInputPlayer player) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return false;
    }
    PreciseSource *pad = &sources[static_cast<int>(player)][static_cast<int>(InputSource::SourcePad)];
    PreciseSource *touch = &sources[static_cast<int>(player)][static_cast<int>(InputSource::SourceTouch)];
    bool padHold = pad->holdState && !pad->suppressHoldUntilRelease;
    bool touchHold = touch->holdState && !touch->suppressHoldUntilRelease;
    return padHold || touchHold;
}

bool piPressed(PreciseInputPlayer player) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return false;
    }
    return sources[static_cast<int>(player)][static_cast<int>(InputSource::SourcePad)].pressedEdge || sources[static_cast<int>(player)][static_cast<int>(InputSource::SourceTouch)].pressedEdge;
}

u32 piPadEventCount(PreciseInputPlayer player) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return 0;
    }
    return sources[static_cast<int>(player)][static_cast<int>(InputSource::SourcePad)].queueCount;
}

u32 piTouchEventCount(PreciseInputPlayer player) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return 0;
    }
    return sources[static_cast<int>(player)][static_cast<int>(InputSource::SourceTouch)].queueCount;
}

PreciseInputEvent piPadEventGet(PreciseInputPlayer player, u32 index) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return (PreciseInputEvent){ 0 };
    }
    PreciseSource *pad = &sources[static_cast<int>(player)][static_cast<int>(InputSource::SourcePad)];
    return pad->queue[(pad->queueHead + index) % INPUT_QUEUE_SIZE];
}

PreciseInputEvent piTouchEventGet(PreciseInputPlayer player, u32 index) {
    if (static_cast<int>(player) >= static_cast<int>(PreciseInputPlayer::PiPlayerCount)) {
        return (PreciseInputEvent){ 0 };
    }
    PreciseSource *touch = &sources[static_cast<int>(player)][static_cast<int>(InputSource::SourceTouch)];
    return touch->queue[(touch->queueHead + index) % INPUT_QUEUE_SIZE];
}

static bool padSampleJump(const PreciseSource *src, u32 slot) {
    vu32 *pointerToRingBuffer = getRingBuffer(src);
    return (pointerToRingBuffer[PAD_RING_ENTRY_WORDS * slot] & src->jumpKeys) != 0;
}

static bool touchSampleJump(const PreciseSource *src, u32 slot) {
    vu32 *pointerToRingBuffer = getRingBuffer(src);
    u32 position = pointerToRingBuffer[TOUCH_RING_ENTRY_WORDS * slot];
    u32 valid = pointerToRingBuffer[(TOUCH_RING_ENTRY_WORDS * slot) + 1];

    if (!valid) {
        return false;
    }

    if (src->touchFilter) {
        u16 px = (u16)(position & 0xFFFF);
        u16 py = (u16)(position >> 16);
        return src->touchFilter(px, py);
    }

    return true;
}

static vu32 *getRingBuffer(const PreciseSource* src) {
    return (vu32*)((u8*)hidSharedMem + src->ringBufferOffset);
}

// calculates the duration between HID samples
static u32 sampleInterval(u32 latestTick, u32 prevTick) {
    return (latestTick - prevTick) / RING_BUFFER_ENTRIES;
}

// calculates the estimated (+/- interval) time the click occured. assumes click/release has occured
static u32 reconstructTick(u32 newestTime, u32 samplesAgo, u32 interval) {
    // get the newest time and subtract how long ago the click happened
    return newestTime - (samplesAgo * interval);
}

static bool pushEvent(PreciseSource *src, u32 tick, bool down) {
    if (src->queueCount >= INPUT_QUEUE_SIZE) {
        return false;
    }
    PreciseInputEvent pie;
    pie.down = down;
    pie.tick = tick;
    src->queue[(src->queueHead + src->queueCount) % INPUT_QUEUE_SIZE] = pie;
    src->queueCount++;
    return true;
}

static PreciseInputEvent peekEvent(PreciseSource *src) {
    return src->queue[src->queueHead];
}

static PreciseInputEvent popEvent(PreciseSource *src) {
    PreciseInputEvent pie = src->queue[src->queueHead];
    src->queueHead = (src->queueHead + 1) % INPUT_QUEUE_SIZE;
    src->queueCount--;
    return pie;
}

static RingSnapshot snapshotRing(const PreciseSource *src, u32 now) {
    RingSnapshot snapshot;
    snapshot.now = now;

    // the hid sysmodule shares its button samples with every process (hidSharedMem),
    // so we can read its ring buffer directly to find when a button was clicked: https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
    snapshot.latestTick = (u32)(*(vu64*)&hidSharedMem[src->tickWord]);
    snapshot.prevTick = (u32)(*(vu64*)&hidSharedMem[src->tickWord + 2]);
    snapshot.currentIdx = hidSharedMem[src->idxWord];
    return snapshot;
}

static void pollSource(PreciseSource *src, const RingSnapshot *snapshot) {
    u32 now = snapshot->now;
    u32 latestTick = snapshot->latestTick;
    u32 prevTick = snapshot->prevTick;
    u32 currentIdx = snapshot->currentIdx;

    u32 interval = sampleInterval(latestTick, prevTick);

    // the lap ticks aren't initialized yet (zero or only one stamp so far), so the
    // interval is zero or its huge (garbage) and we can't reconstruct timestamps, we resync instead
    if (interval == 0 || interval > MAX_SAMPLE_INTERVAL_TICKS) {
        resyncFromRing(src);
        return;
    }

    u32 newestTime = latestTick + (currentIdx * interval);

    u32 idxAdvanced = (currentIdx - src->lastIdx) & (RING_BUFFER_ENTRIES - 1);

    // calculate the number of samples elapsed since last poll
    u32 elapsedTime = now - src->lastPollTick;
    u32 samplesElapsed = (elapsedTime + (interval / 2)) / interval;

    // data was overwritten, but insted of resyncing immediatly we go through the survivors
    // for any recent potential inputs
    if(samplesElapsed > RING_BUFFER_ENTRIES){
        idxAdvanced = RING_BUFFER_ENTRIES;
    }

    // if at 30fps, then idx_advanced == 0 will look the same as moving ahead 8 slots
    // (making a whole lap around the ring buffer), we must differenciate between idx
    // not advancing and doing a whole lap by looking at the number of samples elapsed
    // since the last cpu tick
    if(!idxAdvanced && samplesElapsed >= (RING_BUFFER_ENTRIES / 2)){
        idxAdvanced = RING_BUFFER_ENTRIES;
    }

    for(u32 i = idxAdvanced; i-- > 0;){
        u32 slot = (currentIdx - i) & (RING_BUFFER_ENTRIES - 1);
        bool jump = src->sampleJump(src, slot);
        if(jump != src->lastSampleJump){
            u32 inputTick = reconstructTick(newestTime, i, interval);
            if(!pushEvent(src, inputTick, jump)){
                resyncFromRing(src);
                return;
            }
        }
        src->lastSampleJump = jump;
    }
    src->lastIdx = currentIdx;
    src->lastPollTick = now;
}

static void applySource(PreciseSource *src, u32 substep) {
    src->pressedEdge = src->fakePress;
    src->fakePress = false;
    u32 cutoff = substepCutoff(substep);
    bool finalSubstep = (substep + 1 >= frameSubsteps);
    bool pressedThisSubstep = false;

    while(src->queueCount > 0){
        PreciseInputEvent pie = peekEvent(src);

        // stop at the first event that isnt due yet except on the final substep, which must flush everything.
        if(!finalSubstep && (s32)(pie.tick - cutoff) > 0){
            break;
        }
        if(!finalSubstep && pressedThisSubstep){
            break;
        }

        popEvent(src);

        src->suppressHoldUntilRelease = false;

        if(pie.down){
            src->holdState = true;
            src->pressedEdge = true;
            pressedThisSubstep = true;
        }else{
            src->holdState = false;
        }
    }
}

static void resyncFromRing(PreciseSource *src) {
    bool wasHolding = src->holdState;

    src->queueCount = 0;
    src->queueHead = 0;

    src->lastIdx = hidSharedMem[src->idxWord];

    src->holdState = src->sampleJump(src, src->lastIdx);   // is a jump key down NOW?
    src->lastSampleJump = src->holdState;

    if(!src->holdState){
        src->suppressHoldUntilRelease = false;
    }

    // the press edge was overwritten by the lap, but the state change survived.
    // was can create the edge from the evidencewe have. This makes the ufo work a low frame rates
    // because ufo's require an edge to avoid continuous flapping while holding the button.
    if(!wasHolding && src->holdState && !src->suppressHoldUntilRelease){
        src->fakePress = true;
    }

    src->lastPollTick = (u32)svcGetSystemTick();
}

// calculates the tick where substeps slice of the frame window ends (events are due at or before it)
static u32 substepCutoff(u32 substep) {
    u32 span = frameEnd - frameStart;
    u32 slicesDone = substep + 1;

    // on a lag spike the window can reach 0.5s with slices_done up to ~120. that
    // product overflows if the multiplication happens in u32, so we widen to u64 first
    u64 scaledSpan = (u64)span * slicesDone;
    u32 offsetIntoFrame = (u32)(scaledSpan / frameSubsteps);
    return frameStart + offsetIntoFrame;
}

}
