#pragma once
#include "3ds/types.h"
#include <vector>

namespace gd3ds
{

enum class LightningType {
    SizeMini,
    SizeNormal,
    Teleport
};

struct LightningPoint {
    float x;
    float y;
};

struct LightningSegment {
    float startX;
    float startY;
    float endX;
    float endY;
};

class Lightning {
public:
    Lightning(float startX, float startY, float endX, float endY, LightningType type);
    void updateLightning(float delta);
    void renderLightning();
    bool done();
private:
    std::vector<LightningSegment> m_segments;
    float m_time;
    u32 m_color;

    std::vector<LightningSegment> generateLightingRecursive(
        const std::vector<LightningPoint>& points, 
        int segmentLength, 
        LightningType type,
        unsigned long long seed
    );
};

void updateAllLightning(float delta);
void drawAllLightning();
void addLightning(float start_x, float start_y, float end_x, float end_y, LightningType type);
void resetLightnings();
}
