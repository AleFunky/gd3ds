#pragma once

typedef struct {
    int   texture_idx;
    float px, py;
    float rotation;
    float scale_x, scale_y;
    int   z_order;
} PlayerSpritePart;

typedef struct {
    const PlayerSpritePart *parts;
    int   part_count;
    float delay;
} PlayerFrame;

typedef struct {
    const PlayerFrame *frames;
    int   frame_count;
} PlayerAnimation;
