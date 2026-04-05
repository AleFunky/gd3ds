#pragma once

#include <3ds.h>
#include "particle_definitions.h"

// Struct of arrays my beloved
typedef struct {
    int count;
    int capacity;

    float* timeToLive;
    float* totalTimeToLive;

    float* posx;
    float* posy;

    float* dirX;
    float* dirY;

    float *gravityX;
    float *gravityY;

    float* radialAccel;
    float* tangentialAccel;

    float* colorR;
    float* colorG;
    float* colorB;
    float* colorA;

    float* deltaColorR;
    float* deltaColorG;
    float* deltaColorB;
    float* deltaColorA;

    float* size;
    float* deltaSize;

    float* rotation;
    float* deltaRotation;

    // Mode B (Radial / Orbit by Mindcap and more)
    float* angle;
    float* degreesPerSecond;
    float* radius;
    float* deltaRadius;
} ParticleData;

typedef struct {
    ParticleData data;
    ParticleDefinition cfg;

    float emissionRate;
    float emitCounter;

    float elapsed;
    float duration;

    float gravityX;
    float gravityY;

    float emitterX;
    float emitterY;

    float scale;

    bool stationary;
    bool relativeStationary;
    bool gravityFlipped;
    bool posVarRotates;
    
    bool active;
    bool emitting;
} ParticleSystem;

void updateParticleSystem(ParticleSystem* ps, float dt);
void initParticleData(ParticleData* d, int capacity);
void initParticleSystem(ParticleSystem* ps, const ParticleDefinition* cfg);
void spawnMultipleParticles(ParticleSystem* ps, int emitCount);
void drawParticleSystem(ParticleSystem* ps, float x_offset, float y_offset, float opacity);
void freeParticleData(ParticleData* d);