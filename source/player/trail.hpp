// Adapted from OpenGD

#ifndef MOTION_TRAIL_H
#define MOTION_TRAIL_H

#include <citro2d.h>
#include "objects.hpp"
#include "triggers.hpp"
#include "main.hpp"

#include "icons.hpp"

#define MAX_TRAIL_POINTS 128
#define TRAIL_CLEAR_DISTANCE 30.f
#define STATIONARY_TRAIL_CHUNK_SIZE 64.f

typedef struct {
    float u, v;
} Tex2F;

typedef struct {
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

    Vec2D positionR;
    Color color;

    C2D_Image image;

    float pointState[MAX_TRAIL_POINTS];
    Vec2D pointVertexes[MAX_TRAIL_POINTS];
    Vec2D vertices[MAX_TRAIL_POINTS * 2];
    u8 opacities[MAX_TRAIL_POINTS * 2];
    Tex2F texCoords[MAX_TRAIL_POINTS * 2];
    
    Vec2D centerVertices[MAX_TRAIL_POINTS * 2];

    Vec2D lastStopPosition;

    int player;

    bool wasStopped;
    bool blending;
    bool alwaysOn;
    bool stationary;
} MotionTrail;

typedef struct {
    float fade;
    float width;
    bool alwaysOn;
    bool colored;
    bool stationary;
    bool blending;
} MotionTrailConfig;

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

#endif