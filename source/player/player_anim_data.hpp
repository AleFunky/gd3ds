#pragma once

namespace gd3ds
{

struct PlayerSpritePart {
    int   textureIdx;
    float px, py;
    float rotation;
    float scaleX, scaleY;
    int   zOrder;
};

struct PlayerFrame {
    const PlayerSpritePart *parts;
    int   partCount;
    float delay;
};

struct PlayerAnimation {
    const PlayerFrame *frames;
    int   frameCount;
};

}