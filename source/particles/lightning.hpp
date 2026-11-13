#pragma once
#include "3ds/types.h"
#include <deque>
#include <vector>

namespace gd3ds
{

enum class LightningType{
    SIZE_MINI,
    SIZE_NORMAL,
    TELEPORT
};

struct LightningPoint {
    float x;
    float y;

    LightningPoint(float x, float y);
};

struct LightningSegment {
    float startX;
    float startY;
    float endX;
    float endY;
    u32 color;

    LightningSegment(float startX, float startY, float endX, float endY, u32 color);
};

class Lightning {
private:
    std::vector<LightningSegment> segments;
    float pulseTime;

    std::vector<LightningSegment> generateLightingRecursive(
        const std::vector<LightningPoint>& points, 
        int segmentLength, 
        LightningType type,
        unsigned long long seed
    );
public:
    Lightning(float startX, float startY, float endX, float endY, LightningType type);
    void renderLightning();
    bool done();
};

extern std::deque<Lightning> lightningBolts;
void drawAllLightning();

}