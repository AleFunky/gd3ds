#pragma once
#include <3ds.h>

namespace gd3ds
{

struct ProfilerUpdateData {
    touchPosition touchPos;
    u32 kDown;
    int steps;
    float processingTime;
};

struct ProfilerSnapshot {
    // General
    float frameMs;
    float cpuMs;
    float gpuMs;

    // Timers
    float physicsMs;
    float triggersMs;
    float collisionMs;
    float particlesMs;
    float renderingMs;

    // Physics
    float collMs;
    float playMs;
    float handlerMs;
    float stepTriggerMs;
    int steps;
    int collisions;
    int collisionChecks;

    // Rendering
    float creatingMs;
    float sortingMs;
    float tintMs;
    float drawingMs;
    int drawCount;

    // Touch
    int inputX;
    int inputY;

    // Player
    int tick;
    float plyPosX;
    float plyPosY;
    float plyVelX;
    float plyVelY;

    // Camera
    float camPosX;
    float camPosY;
    float camItdY;

    // Heap
    u32 linearFree;
    u32 heapUsed;
    u32 heapFree;
    u32 heapTotal;

    u32 levelArenaUsed;
    u32 levelArenaFree;
    u32 levelArenaTotal;
};

extern ProfilerSnapshot snapshot;

void profilerUpdate(ProfilerUpdateData data);
void profilerDraw();

}
