#pragma once
#include <citro2d.h>
#include "3ds/types.h"

namespace gd3ds
{

struct GravityLine {
    float x;
    float y;
    float scaleX;
    float scaleY;
    float opacity;
};

class GravitySweep {
public:
    GravitySweep(bool down);

    void updateSweepTimer(float delta);
    void startSweep();
    void renderSweep();
    bool done();
private:
    GravityLine m_lines[22];
    float m_time;
    bool m_down;
};

void startGravitySweep(bool down);
void updateGravitySweeps(float delta);
void drawGravitySweeps();
void resetGravitySweeps();
}
