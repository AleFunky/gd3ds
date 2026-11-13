#pragma once

#include <3ds.h>

#define PARTICLE_DEF_COUNT (30)

namespace gd3ds
{

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

typedef struct {
    const char *name;
    const ParticleDefinition *def;
} ParticleDefenitionName;

extern const ParticleDefenitionName particleDefNames[];

extern const ParticleDefinition dragEffect;
extern const ParticleDefinition ringEffect;
extern const ParticleDefinition portalEffect01;
extern const ParticleDefinition bumpEffect;
extern const ParticleDefinition portalEffect08;
extern const ParticleDefinition landEffect;
extern const ParticleDefinition glassDestroy01;
extern const ParticleDefinition glitterEffect;
extern const ParticleDefinition levelComplete01;
extern const ParticleDefinition shipDragEffect;
extern const ParticleDefinition speedEffectFast;
extern const ParticleDefinition speedEffectNormal;
extern const ParticleDefinition speedEffectSlow;
extern const ParticleDefinition speedEffectVfast;
extern const ParticleDefinition speedEffectFastest;
extern const ParticleDefinition firework;
extern const ParticleDefinition explodeEffect;
extern const ParticleDefinition coinEffect;
extern const ParticleDefinition coinPickupEffect;
extern const ParticleDefinition burstEffect;
extern const ParticleDefinition boost01Effect;
extern const ParticleDefinition boost02Effect;
extern const ParticleDefinition boost03Effect;
extern const ParticleDefinition boost04Effect;
extern const ParticleDefinition endEffectPortal;
extern const ParticleDefinition shipEffect;
extern const ParticleDefinition shipFireEffect;
extern const ParticleDefinition touchExplosionEffect;
extern const ParticleDefinition touchDragEffect;
extern const ParticleDefinition starEffect;
extern const ParticleDefinition burstEffect2;

}