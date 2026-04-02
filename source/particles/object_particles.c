#include "object_particles.h"
#include "state.h"
#include "main.h"
#include "player/collision.h"
#include "math_helpers.h"

ObjectParticles object_particle[MAX_OBJECT_PS];

ParticleSystem brick_destroy_particles;

void init_op_system() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        object_particle[i].id = -1;
        object_particle[i].occupied = false;
    }
}

bool is_ps_already_loaded(int id) {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied && object_particle[i].id == id) {
            return true;
        }
    }
    return false;
}

int load_object_particles(int id, const ParticleDefinition *cfg, bool stationary) {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (!object_particle[i].occupied) {
            object_particle[i].occupied = true;
            object_particle[i].id = id;
            object_particle[i].ps.stationary = stationary; 
            initParticleSystem(&object_particle[i].ps, cfg);
            
            object_particle[i].ps.emitting = true;
            return i;
        }
    }
    return -1;
}

static void remove_offscreen_object_particles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            float x = object_particle[i].ps.emitterX;
            float y = object_particle[i].ps.emitterY;

            if (!object_particle[i].ps.stationary) {
                x = ((x - state.camera_x));
                y = SCREEN_HEIGHT - ((y - state.camera_y));  
            }

            if (x < -60 || x >= (SCREEN_WIDTH / SCALE) + 60 || y < -60 || y >= (SCREEN_HEIGHT / SCALE) + 60) {
                object_particle[i].occupied = false;
                freeParticleData(&object_particle[i].ps.data);
            }
        }
    }
}

void update_object_particles() {
    remove_offscreen_object_particles();
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            updateParticleSystem(&object_particle[i].ps, DT);
        }
    }
}


void draw_object_particles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            int obj = object_particle[i].id;
            int fade_x = 0;
            int fade_y = 0;

            float calc_x = ((objects.x[obj] - state.camera_x));

            float fade_scale = 1.f;
            get_fade_vars(obj, calc_x, &fade_x, &fade_y, &fade_scale);

            float opacity = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE) / 255.f;

            drawParticleSystem(&object_particle[i].ps, fade_x, fade_y, opacity);
        }
    }
}

void set_particle_color(ParticleDefinition *cfg, float red, float green, float blue) {
    cfg->startColorRed = red;
    cfg->startColorGreen = green;
    cfg->startColorBlue = blue;

    cfg->finishColorRed = red;
    cfg->finishColorGreen = green;
    cfg->finishColorBlue = blue;
}

void spawn_object_particles(int obj) {
    int index;
    switch (objects.id[obj]) {
        case YELLOW_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 0);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 1);
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case PINK_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0, 1);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case YELLOW_PAD:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &bump_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 0);

                    object_particle[index].ps.cfg.angle = 180.f - (adjust_angle_y(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_PAD:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &bump_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 1);

                    object_particle[index].ps.cfg.angle = 180.f - (adjust_angle_y(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case PINK_PAD:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &bump_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0, 1);

                    object_particle[index].ps.cfg.angle = 180.f - (adjust_angle_y(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case YELLOW_GRAVITY_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 0);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_GRAVITY_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case CUBE_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 0.25f);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SHIP_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BALL_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.25f, 0.25f);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case UFO_PORTAL:
        case ORANGE_MIRROR_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.75f, 0);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case WAVE_PORTAL:
        case BLUE_MIRROR_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BIG_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_08, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 0.25f);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case MINI_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_08, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.5f, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DUAL_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_08, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.75f, 0);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DIVORCE_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_08, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 0.69f, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SLOW_SPEED_PORTAL:
            if (!is_ps_already_loaded(obj))
            {
                index = load_object_particles(obj, &boost_01_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 0);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case NORMAL_SPEED_PORTAL:
            if (!is_ps_already_loaded(obj))
            {
                index = load_object_particles(obj, &boost_02_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 0.75f, 1);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case FAST_SPEED_PORTAL:
            if (!is_ps_already_loaded(obj))
            {
                index = load_object_particles(obj, &boost_03_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 0);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case FASTER_SPEED_PORTAL:
            if (!is_ps_already_loaded(obj))
            {
                index = load_object_particles(obj, &boost_04_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 0.9f, 0.25f, 1);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
    }
}