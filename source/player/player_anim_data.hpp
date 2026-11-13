#pragma once

typedef struct {
    int   textureIdx;
    float px, py;
    float rotation;
    float scaleX, scaleY;
    int   zOrder;
} PlayerSpritePart;

typedef struct {
    const PlayerSpritePart *parts;
    int   partCount;
    float delay;
} PlayerFrame;

typedef struct {
    const PlayerFrame *frames;
    int   frameCount;
} PlayerAnimation;
