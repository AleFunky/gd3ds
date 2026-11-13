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

    void startSweep();
    void renderSweep();
    bool done();
};

void startGravitySweep(bool down);
void drawGravitySweeps();