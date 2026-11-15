#pragma once

// WIP
namespace gd3ds::constants
{

    inline constexpr int ScreenWidth = 400;
    inline constexpr int ScreenBotWidth = 320;
    inline constexpr int ScreenHeight = 240;
    inline constexpr float SwapSizeOffsetX = ((ScreenWidth - ScreenBotWidth) / 2.f);

    inline constexpr float MaxAudioDB = 9;

    namespace player {
        inline constexpr float RotationSpeed = 180 / (26.0f / 60.0f);
    }

    namespace ui {

    }
}