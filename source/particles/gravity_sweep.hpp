#pragma once
#include <citro2d.h>
#include "3ds/types.h"

extern C2D_SpriteSheet gravLineSheet;

struct GravityLine {
    float x;
    float y;
    float scaleX;
    float scaleY;
    float opacity;
};

class GravitySweep {
    GravityLine lines[22];
    float time;
    bool down;
public:
    GravitySweep(bool down);

    void start_sweep();
    void render_sweep();
    bool done();
};

void start_gravity_sweep(bool down);
void draw_gravity_sweeps();