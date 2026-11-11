#pragma once
#include "3ds/types.h"
#include <deque>
#include <vector>

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
    float start_x;
    float start_y;
    float end_x;
    float end_y;
    u32 color;

    LightningSegment(float start_x, float start_y, float end_x, float end_y, u32 color);
};

class Lightning {
private:
    std::vector<LightningSegment> segments;
    float pulseTime;

    std::vector<LightningSegment> generate_lighting_recursive(
        const std::vector<LightningPoint>& points, 
        int segment_length, 
        LightningType type,
        unsigned long long seed
    );
public:
    Lightning(float start_x, float start_y, float end_x, float end_y, LightningType type);
    void render_lightning();
    bool done();
};

extern std::deque<Lightning> lightning_bolts;
void draw_all_lightning();