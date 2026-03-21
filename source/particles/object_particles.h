#pragma once

#include "particles.h"

#define MAX_OBJECT_PS 48

typedef struct {
    ParticleSystem ps;
    int id;
    bool occupied;
    bool isStationary;
} ObjectParticles;

extern ObjectParticles object_particle[MAX_OBJECT_PS];

void init_op_system();
bool is_ps_already_loaded(int id);
int load_object_particles(int id, const ParticleDefinition *cfg, bool stationary);
void draw_object_particles();
void spawn_object_particles(int obj);
void update_object_particles();