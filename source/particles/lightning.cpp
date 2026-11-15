#include "lightning.hpp"
#include <citro2d.h>
#include <random>
#include "math_helpers.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"

namespace gd3ds
{

constexpr u32 kLightningSizeMiniCol = C2D_Color32f(1.0f, 31 / 255.f, 1.0f, 1.0f);
constexpr u32 kLightningSizeNormalCol = C2D_Color32f(0.0f, 1.0f, 50.f / 255.f, 1.0f);

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

    float xRand = ((lightningRand(seed) % 101) / 100.0f - 0.5f) * segmentLength;
    float yRand = ((lightningRand(seed) % 101) / 100.0f - 0.5f) * segmentLength;

    unsigned int i = 0;
    for(; i < points.size() - 1; i++){
        nextPoints.push_back(points[i]);

        float midX = (points[i].x + points[i + 1].x) / 2.0f;
        float midY = (points[i].y + points[i + 1].y) / 2.0f;
        midX += xRand;
        midY += yRand;

        LightningPoint point = (LightningPoint){
            .x = midX,
            .y = midY
        };

        nextPoints.push_back(point);
    }
    nextPoints.push_back(points.back());

    segmentLength /= 2;
    if(segmentLength > 8){
        return generateLightingRecursive(nextPoints, segmentLength, type, seed);
    } else{
        //generate lightning
        std::vector<LightningSegment> segments;
        unsigned int segmentCount = nextPoints.size() - 1;
        segments.reserve(segmentCount);

        for(unsigned int i = 0; i < segmentCount; i++){
            segments.push_back((LightningSegment){
                nextPoints[i].x, 
                nextPoints[i].y, 
                nextPoints[i + 1].x,
                nextPoints[i + 1].y
            });
        }

        return segments;
    }
}

void Lightning::updateLightning(float delta){
    m_time += delta;
}

void Lightning::renderLightning(){
    for(unsigned int i = 0; i < m_segments.size(); i++){
        LightningSegment s = m_segments[i];

        //set alpha
        float alpha = 1.0f - (m_time * 2.0f);

        alpha = clampf(alpha, 0.0f, 1.0f);

        u32 color = m_color & (((u32)(alpha * (u32)255) << 24) | 0x00FFFFFF);

        float topX = s.startX - state.cameraX;
        float btmX = s.endX - state.cameraX;
        float topY = SCREEN_HEIGHT - (s.startY - state.cameraY);
        float btmY = SCREEN_HEIGHT - (s.endY  - state.cameraY);

        C2D_DrawLine(getMirrorX(topX, state.mirrorFactor), topY, color, getMirrorX(btmX, state.mirrorFactor), btmY, color, 2, 0.0f);
    }
}

bool Lightning::done(){
    return m_time >= 0.5f;
}

Lightning::Lightning(float startX, float startY, float endX, float endY, LightningType type){
    switch(type){
        case LightningType::SizeMini:
            m_color = kLightningSizeMiniCol;
            break;
        case LightningType::SizeNormal:
            m_color = kLightningSizeNormalCol;
            break;
        case LightningType::Teleport:
            m_color = p1Color.toColor32();
            break;
        default:
            break;
    }

    m_time = 0.0f;

    std::vector<LightningPoint> segmentPoints = {
        (LightningPoint){
            startX, startY
        },
        (LightningPoint){
            endX, endY
        },
    };

    m_segments = generateLightingRecursive(segmentPoints, 160, type, rand());
}

std::vector<Lightning> lightningBolts;

void updateAllLightning(float delta){
    for(auto& bolt : lightningBolts){
        bolt.updateLightning(delta);
    }
}

void drawAllLightning(){
    for(auto& bolt : lightningBolts){
        bolt.renderLightning();
    }
}

void addLightning(float start_x, float start_y, float end_x, float end_y, LightningType type){
    for(auto& bolt : lightningBolts){
        if(bolt.done()) {
            bolt = Lightning(start_x, start_y, end_x, end_y, type);
            return;
        }
    }
    lightningBolts.push_back(Lightning(start_x, start_y, end_x, end_y, type));
}

void resetLightnings(){
    lightningBolts.clear();
    lightningBolts.shrink_to_fit();
}

}