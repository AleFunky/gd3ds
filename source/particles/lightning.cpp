#include "lightning.hpp"
#include <citro2d.h>
#include <random>
#include "math_helpers.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"

constexpr u32 lightningSizeMiniCol = C2D_Color32f(1.0f, 31 / 255.f, 1.0f, 1.0f);
constexpr u32 lightningSizeNormalCol = C2D_Color32f(0.0f, 1.0f, 50.f / 255.f, 1.0f);

constexpr u32 lightningTeleInCol = C2D_Color32f(56 / 255.f, 200 / 255.f, 1.0f, 1.0f);
constexpr u32 lightningTeleOutCol = C2D_Color32f(1.0f, 1.0f, 0.0f, 1.0f);

LightningPoint::LightningPoint(float x, float y){
    this->x = x;
    this->y = y;
}

LightningSegment::LightningSegment(float startX, float startY, float endX, float endY, u32 color){
    this->startX = startX;
    this->startY = startY;
    this->endX = endX;
    this->endY = endY;
    this->color = color;
}

static unsigned int lightningRand(unsigned long long& seed){
    seed = seed * 1103515245 + 12345;
    return (unsigned)(seed / 65536) % 32768;
}

std::vector<LightningSegment> Lightning::generateLightingRecursive(
    const std::vector<LightningPoint>& points, 
    int segmentLength, 
    LightningType type,
    unsigned long long seed
){
    std::vector<LightningPoint> nextPoints;

    unsigned int i = 0;
    for(; i < points.size() - 1; i++){
        nextPoints.push_back(points[i]);

        float midX = (points[i].x + points[i + 1].x) / 2.0f;
        float midY = (points[i].y + points[i + 1].y) / 2.0f;
        midX += ((lightningRand(seed) % 101) / 100.0f - 0.5f) * segmentLength;
        midY += ((lightningRand(seed) % 101) / 100.0f - 0.5f) * segmentLength;

        LightningPoint point = LightningPoint(midX, midY);

        nextPoints.push_back(point);
    }
    nextPoints.push_back(points.back());

    if(segmentLength > 8){
        return generateLightingRecursive(nextPoints, segmentLength / 2, type, seed);
    } else{
        //generate lightning
        std::vector<LightningSegment> segments;
        unsigned int segmentCount = nextPoints.size() - 1;
        segments.reserve(segmentCount);

        for(unsigned int i = 0; i < segmentCount; i++){
            u32 color = 0;
            switch(type){
                case LightningType::SIZE_MINI:
                    color = lightningSizeMiniCol;
                    break;
                case LightningType::SIZE_NORMAL:
                    color = lightningSizeNormalCol;
                    break;
                case LightningType::TELEPORT:
                    color = p1Color.toColor32();
                    break;
                default:
                    break;
            }

            segments.push_back(LightningSegment(
                nextPoints[i].x, 
                nextPoints[i].y, 
                nextPoints[i + 1].x,
                nextPoints[i + 1].y,
                color
            ));
        }

        return segments;
    }
}

void Lightning::renderLightning(){
    for(unsigned int i = 0; i < this->segments.size(); i++){
        LightningSegment s = segments[i];

        //set alpha
        float alpha = 1.0f - (this->pulseTime * 2.0f);

        alpha = clampf(alpha, 0.0f, 1.0f);

        u32 color = (s.color & 0x00FFFFFF) |((u32)(alpha * 255.0f) << 24);

        float topX = s.startX - state.cameraX;
        float btmX = s.endX - state.cameraX;
        float topY = SCREEN_HEIGHT - (s.startY - state.cameraY);
        float btmY = SCREEN_HEIGHT - (s.endY  - state.cameraY);

        C2D_DrawLine(topX, topY, color, btmX, btmY, color, 2, 0.0f);
    }

    if(!gamePaused) this->pulseTime += delta;
}

bool Lightning::done(){
    return pulseTime >= 0.5f;
}

Lightning::Lightning(float startX, float startY, float endX, float endY, LightningType type){
    std::vector<LightningPoint> segmentPoints = {
        LightningPoint(startX, startY),
        LightningPoint(endX, endY)
    };

    this->segments = generateLightingRecursive(segmentPoints, 160, type, rand());
}

std::deque<Lightning> lightningBolts;

void drawAllLightning(){
    while (!lightningBolts.empty() && lightningBolts.front().done()) {
        lightningBolts.pop_front();
    }

    for(auto& bolt : lightningBolts){
        bolt.renderLightning();
    }
}