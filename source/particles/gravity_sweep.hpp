#pragma once
#include <citro2d.h>
#include "3ds/types.h"

namespace gd3ds
{

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

    void updateSweepTimer();
    void startSweep();
    void renderSweep();
    bool done();
};

void startGravitySweep(bool down);
void updateGravitySweeps();
void drawGravitySweeps();

}
