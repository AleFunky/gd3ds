#pragma once

#include <3ds.h>

typedef enum {
    TEXTURE_SQUARE_PNG,
    TEXTURE_CIRCLE_PNG,
} TextureFileName;

// Billion properties
typedef struct {
    float angle;
    float angleVariance;
    int blendFuncDestination;
    int blendFuncSource;
    float duration;
    int emitterType;
    float finishColorAlpha;
    float finishColorBlue;
    float finishColorGreen;
    float finishColorRed;
    float finishColorVarianceAlpha;
    float finishColorVarianceBlue;
    float finishColorVarianceGreen;
    float finishColorVarianceRed;
    float finishParticleSize;
    float finishParticleSizeVariance;
    float gravityx;
    float gravityy;
    float maxParticles;
    float maxRadius;
    float maxRadiusVariance;
    float minRadius;
    float particleLifespan;
    float particleLifespanVariance;
    float radialAccelVariance;
    float radialAcceleration;
    float rotatePerSecond;
    float rotatePerSecondVariance;
    float rotationEnd;
    float rotationEndVariance;
    float rotationStart;
    float rotationStartVariance;
    float sourcePositionVariancex;
    float sourcePositionVariancey;
    float sourcePositionx;
    float sourcePositiony;
    float speed;
    float speedVariance;
    float startColorAlpha;
    float startColorBlue;
    float startColorGreen;
    float startColorRed;
    float startColorVarianceAlpha;
    float startColorVarianceBlue;
    float startColorVarianceGreen;
    float startColorVarianceRed;
    float startParticleSize;
    float startParticleSizeVariance;
    float tangentialAccelVariance;
    float tangentialAcceleration;
    TextureFileName textureFileName;
} ParticleDefinition;

extern const ParticleDefinition drag_effect;
extern const ParticleDefinition ring_effect;
extern const ParticleDefinition portal_effect_01;
extern const ParticleDefinition bump_effect;
extern const ParticleDefinition portal_effect_08;
extern const ParticleDefinition land_effect;
extern const ParticleDefinition glass_destroy_01;
extern const ParticleDefinition glitter_effect;
extern const ParticleDefinition level_complete_01;
extern const ParticleDefinition ship_drag_effect;
extern const ParticleDefinition speed_effect_fast;
extern const ParticleDefinition speed_effect_normal;
extern const ParticleDefinition speed_effect_slow;
extern const ParticleDefinition speed_effect_vfast;
extern const ParticleDefinition firework;
extern const ParticleDefinition explode_effect;
extern const ParticleDefinition coin_effect;
extern const ParticleDefinition coin_pickup_effect;
extern const ParticleDefinition burst_effect;
extern const ParticleDefinition boost_01_effect;
extern const ParticleDefinition boost_02_effect;
extern const ParticleDefinition boost_03_effect;
extern const ParticleDefinition boost_04_effect;
extern const ParticleDefinition end_effect_portal;
extern const ParticleDefinition ship_effect;
extern const ParticleDefinition ship_fire_effect;