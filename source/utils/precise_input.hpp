#pragma once
#include <3ds.h>
#include <stdbool.h>

namespace gd3ds
{

enum class PreciseInputPlayer {
    PiPlayer1,
    PiPlayer2,
    PiPlayerCount
};

struct PreciseInputEvent {
    u32 tick;
    bool down;
};

extern bool piEnabled;

void piReset(void);
void piPoll(void);
void piBeginFrame(u32 frameStartTick, u32 frameEndTick, u32 substeps);
void piApplySubstep(u32 substep);

void piSetJumpKeys(PreciseInputPlayer player, u32 mask);
void piSetTouchFilter(PreciseInputPlayer player, bool (*filter)(u16 px, u16 py));
void piSuppressUntilRelease(void);
bool piHold(PreciseInputPlayer player);
bool piPressed(PreciseInputPlayer player);

u32 piPadEventCount(PreciseInputPlayer player);
PreciseInputEvent piPadEventGet(PreciseInputPlayer player, u32 index);
u32 piTouchEventCount(PreciseInputPlayer player);
PreciseInputEvent piTouchEventGet(PreciseInputPlayer player, u32 index);

}
