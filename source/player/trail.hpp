// Adapted from OpenGD

#pragma once

#include <citro2d.h>
#include "level_loading.hpp"
#include "types.hpp"

#include "../icons.hpp"

#define MAX_TRAIL_POINTS 128
#define TRAIL_CLEAR_DISTANCE 30.f
#define STATIONARY_TRAIL_CHUNK_SIZE 64.f

namespace gd3ds
{

struct Tex2F {
    float u, v;
};

struct MotionTrail {
    bool appendNewPoints;
    bool startingPositionInitialized;
    bool waveTrail;

    unsigned int nuPoints;
    unsigned int previousNuPoints;
    unsigned int actualNuPoints;
    unsigned int maxPoints;
    unsigned int offscreenCount;

    float opacity;
    float fadeDelta;
    float minSeg;

    float width;

    float stroke;

    float uvOffset;

    Point positionR;
    Color color;

    C2D_Image image;

    float pointState[MAX_TRAIL_POINTS];
    Point pointVertexes[MAX_TRAIL_POINTS];
    Point vertices[MAX_TRAIL_POINTS * 2];
    u8 opacities[MAX_TRAIL_POINTS * 2];
    Tex2F texCoords[MAX_TRAIL_POINTS * 2];
    
    Point centerVertices[MAX_TRAIL_POINTS * 2];

    Point lastStopPosition;

    int player;

    bool wasStopped;
    bool blending;
    bool alwaysOn;
    bool stationary;
};

struct MotionTrailConfig {
    float fade;
    float width;
    bool alwaysOn;
    bool colored;
    bool stationary;
    bool blending;
};

extern const MotionTrailConfig trailProperties[TRAIL_COUNT];

void motionTrailInit(MotionTrail* trail, int player, float fade, bool alwaysOn, float stroke, bool waveTrail, bool blending, bool stationary, Color color, C2D_Image tex);
void motionTrailUpdateWaveTrail(MotionTrail *trail, float delta);
void motionTrailUpdate(MotionTrail* trail, float delta);
void motionTrailResumeStroke(MotionTrail* trail);
void motionTrailClear(MotionTrail *trail);
void motionTrailStopStroke(MotionTrail* trail);
void motionTrailDraw(MotionTrail* trail);
void motionTrailDrawWaveTrail(MotionTrail *trail);

void motionTrailCopyTrail(MotionTrail *dst, MotionTrail *src);

void motionTrailAddWavePoint(MotionTrail* trail);

}