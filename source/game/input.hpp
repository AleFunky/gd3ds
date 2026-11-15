#pragma once

#include "../types.hpp"
#include "../state.hpp"
#include "../utils/precise_input.hpp"
#include "../menus/settings_hub/settings.hpp"
#include "macros.hpp"
#include "constants.hpp"

namespace gd3ds::input
{

inline bool touchJumpFilter(u16 px, u16 py) {
    bool onPauseButton = px > 300-32/2 && py < 20+32/2;
    bool onPracticeUI = state.practiceMode && !settingsState.hidePracticeButtons && (px > 92 && px < 222 && py > 175 && py < 222);
    return !(onPauseButton || onPracticeUI);
}

inline bool touchLeftHalf(u16 px) {
    return px < constants::ScreenBotWidth / 2;
}

inline bool touchJumpFilterP1(u16 px, u16 py) {
    return touchJumpFilter(px, py) && touchLeftHalf(px);
}

inline bool touchJumpFilterP2(u16 px, u16 py) {
    return touchJumpFilter(px, py) && !touchLeftHalf(px);
}

inline bool touchJumpFilterNone(UNUSED u16 px, UNUSED u16 py) {
    return false;
}

inline u32 jumpKeyMaskP1(void) {
    return (settingsState.yJump ? KEY_Y : KEY_A);
}

inline u32 jumpKeyMaskP2(void) {
    return KEY_UP;
}

inline u32 jumpKeyMask(void) {
    return jumpKeyMaskP1() | jumpKeyMaskP2();
}


inline void handleGameplayInput(touchPosition touchPos, u32 kDown, u32 kHeld) {
    bool inBounds = touchJumpFilter(touchPos.px, touchPos.py);
    bool leftSide = touchLeftHalf(touchPos.px);

    bool touchPressed = inBounds && (kDown & KEY_TOUCH);
    bool touchHeld = inBounds && (kHeld & KEY_TOUCH);

    // CBF not enabled
    if (!piEnabled) {
        if (levelInfo.twoPlayerMode) {
            bool buttonPressed = (kDown & jumpKeyMaskP1()) != 0;
            bool buttonHeld = (kHeld & jumpKeyMaskP1()) != 0;

            bool buttonPressedP2 = (kDown & jumpKeyMaskP2()) != 0;
            bool buttonHeldP2 = (kHeld & jumpKeyMaskP2()) != 0;

            state.oldInput = state.input;
            
            state.input.pressedJump = (buttonPressed || (touchPressed && leftSide)) == true;
            state.input.holdJump = (state.input.pressedJump || buttonHeld || (touchHeld && leftSide)) == true;

            state.oldInputP2 = state.inputP2;

            state.inputP2.pressedJump = (buttonPressedP2 || (touchPressed && !leftSide)) == true;
            state.inputP2.holdJump = (state.inputP2.pressedJump || buttonHeldP2 || (touchHeld && !leftSide)) == true;
        } else {
            bool buttonPressed = (kDown & jumpKeyMask()) != 0;
            bool buttonHeld = (kHeld & jumpKeyMask()) != 0;
            
            state.oldInput = state.input;
            
            state.input.pressedJump = (buttonPressed || touchPressed) == true;
            state.input.holdJump = (state.input.pressedJump || buttonHeld || touchHeld) == true;
            
            state.oldInputP2 = state.input;

            state.inputP2.pressedJump = state.input.pressedJump;
            state.inputP2.holdJump = state.input.holdJump;
        }
    }
}

inline void syncPreciseInput(bool suppressHeld) {
    if (levelInfo.twoPlayerMode) {
        piSetJumpKeys(PreciseInputPlayer::Player1, jumpKeyMaskP1());
        piSetTouchFilter(PreciseInputPlayer::Player1, touchJumpFilterP1);
        piSetJumpKeys(PreciseInputPlayer::Player2, jumpKeyMaskP2());
        piSetTouchFilter(PreciseInputPlayer::Player2, touchJumpFilterP2);
    } else {
        piSetJumpKeys(PreciseInputPlayer::Player1, jumpKeyMask());
        piSetTouchFilter(PreciseInputPlayer::Player1, touchJumpFilter);
        piSetJumpKeys(PreciseInputPlayer::Player2, 0);
        piSetTouchFilter(PreciseInputPlayer::Player2, touchJumpFilterNone);
    }
    piReset();
    if (suppressHeld) {
        piSuppressUntilRelease();
    }
}

}