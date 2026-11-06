#pragma once

#include "particles.h"

#define MAX_OBJECT_PS 48

typedef struct {
    ParticleSystem ps;
    int id;
    bool occupied;
    bool color_applied;
    bool rotation_base_set;
    float rotation_base;
    float angle_base;
    float last_rotation;
} ObjectParticles;

extern ObjectParticles object_particle[MAX_OBJECT_PS];
extern ParticleSystem brick_destroy_particles;

void init_op_system();
bool is_ps_already_loaded(int id);
int load_object_particles(int id, const ParticleDefinition *cfg, bool stationary);
void draw_object_particles();
void spawn_object_particles(int obj);
void update_object_particles(float dt);
void free_object_particles();