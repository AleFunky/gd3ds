#pragma once

#include "particles.hpp"

#define MAX_OBJECT_PS 48

namespace gd3ds
{

struct ObjectParticles {
    ParticleSystem ps;
    int id;
    bool occupied;
    bool colorApplied;
    bool rotationBaseSet;
    float rotationBase;
    float angleBase;
    float lastRotation;
};

extern ObjectParticles objectParticle[MAX_OBJECT_PS];
extern ParticleSystem brickDestroyParticles;

void initOpSystem();
bool isPsAlreadyLoaded(int id);
int loadObjectParticles(int id, const ParticleDefinition *cfg, bool stationary);
void drawObjectParticles();
void spawnObjectParticles(int obj);
void updateObjectParticles(float dt);
void freeObjectParticles();

}