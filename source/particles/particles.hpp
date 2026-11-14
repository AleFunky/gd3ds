#pragma once

#include <3ds.h>
#include "particle_definitions.hpp"

namespace gd3ds
{

// Struct of arrays my beloved
struct ParticleData {
    int count;
    int capacity;

    bool couldLoad;

    float* timeToLive;
    float* totalTimeToLive;

    float* posx;
    float* posy;

    float* relx;
    float* rely;

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
};

struct ParticleSystem {
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

    // How far the particles fly towards you before they die (0 keeps them flat)
    float depth;

    // Flips the drift, so they start out at you and sink back in as they die
    bool depthInwards;

    bool affectedByMirror;
    bool stationary;
    bool relativeStationary;
    bool gravityFlipped;
    bool posVarRotates;
    bool dontApplyCamX;
    bool dontApplyCamY;
    
    bool active;
    bool emitting;
};

void updateParticleSystem(ParticleSystem* ps, float dt);
void initParticleData(ParticleData* d, int capacity);
void initParticleSystem(ParticleSystem* ps, const ParticleDefinition* cfg);
void spawnMultipleParticles(ParticleSystem* ps, int emitCount);
void drawParticleSystem(ParticleSystem* ps, float xOffset, float yOffset, float opacity);
void freeParticleData(ParticleData* d);

}