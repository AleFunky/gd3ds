#include "object_particles.h"
#include <math.h>
#include "level_loading.h"
#include "triggers.h"
#include "particles/particles.h"
#include "state.h"
#include "main.h"
#include "player/collision.h"
#include "math_helpers.h"

ObjectParticles object_particle[MAX_OBJECT_PS];

ParticleSystem brick_destroy_particles;

// probably inaccurate, especially the alpha, but it's osmething lmao
static const ParticleDefinition key_particles = {
    .angleVariance = 180.0f,
    .duration = -1.0f,
    .maxParticles = 36.0f,
    .particleLifespan = 0.6f,
    .particleLifespanVariance = 0.15f,
    .sourcePositionVariancex = 15.0f,
    .sourcePositionVariancey = 10.0f,
    .speed = 5.0f,
    .speedVariance = 5.0f,
    .startColorAlpha = 1.0f,
    .startColorBlue = 1.0f,
    .startColorGreen = 1.0f,
    .startColorRed = 1.0f,
    .startParticleSize = 1.5f,
    .startParticleSizeVariance = 1.1f,
    .textureFileName = TEXTURE_SQUARE_PNG,
};

void init_op_system() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        object_particle[i].id = -1;
        object_particle[i].occupied = false;
        object_particle[i].color_applied = false;
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

// How far the particles an object gives off drift towards you before they die
static float get_object_particle_depth(int id) {
    switch (id) {
        case YELLOW_PAD:
        case BLUE_PAD:
        case PINK_PAD:
        case CUBE_PORTAL:
        case SHIP_PORTAL:
        case BALL_PORTAL:
        case UFO_PORTAL:
        case WAVE_PORTAL:
        case BLUE_GRAVITY_PORTAL:
        case YELLOW_GRAVITY_PORTAL:
        case ORANGE_MIRROR_PORTAL:
        case BLUE_MIRROR_PORTAL:
        case BIG_PORTAL:
        case MINI_PORTAL:
        case SLOW_SPEED_PORTAL:
        case NORMAL_SPEED_PORTAL:
        case FAST_SPEED_PORTAL:
        case FASTER_SPEED_PORTAL:
        case FASTEST_SPEED_PORTAL:
        case DUAL_PORTAL:
        case DIVORCE_PORTAL:
        case ROBOT_PORTAL:
        case SPIDER_PORTAL:
        case BLUE_TP_PORTAL:
        case ORANGE_TP_PORTAL:
            return 0.5f;
        // Orbs and coins hang in mid air, so theirs carry a bit further
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
        case GREEN_ORB:
        case RED_ORB:
        case BLACK_ORB:
        case DASH_ORB:
        case GRAVITY_DASH_ORB:
        case 1594:
        case SECRET_COIN:
            return 0.7f;
    }
    return 0.f;
}

int load_object_particles(int id, const ParticleDefinition *cfg, bool stationary) {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (!object_particle[i].occupied) {
            object_particle[i].occupied = true;
            object_particle[i].id = id;
            object_particle[i].rotation_base_set = false;
            object_particle[i].ps.stationary = stationary;
            initParticleSystem(&object_particle[i].ps, cfg);

            // After the init, it resets the depth back to flat
            object_particle[i].ps.depth = get_object_particle_depth(objects.id[id]);

            // The small portals and orbs spiral their particles in, so the drift follows
            object_particle[i].ps.depthInwards = (cfg == &portal_effect_01 || cfg == &ring_effect);

            object_particle[i].ps.emitting = true;
            return i;
        }
    }
    return -1;
}

void free_object_particles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            freeParticleData(&object_particle[i].ps.data);
        }
    }
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
                object_particle[i].color_applied = false;
                freeParticleData(&object_particle[i].ps.data);
            }
        }
    }
}

static void rotate_object_particles(ParticleSystem *particle_system, float delta_degrees, float emitter_x, float emitter_y) {
    ParticleData *data = &particle_system->data;
    if (!data->couldLoad || data->count <= 0) return;
    float rotation_radians = -delta_degrees * (float)(M_PI / 180.0);
    if (particle_system->cfg.emitterType != 0) {
        for (int particle_index = 0; particle_index < data->count; particle_index++)
            data->angle[particle_index] += rotation_radians;
        return;
    }
    float cos_angle = cosf(rotation_radians);
    float sin_angle = sinf(rotation_radians);
    for (int particle_index = 0; particle_index < data->count; particle_index++) {
        float offset_x = data->posx[particle_index] - particle_system->emitterX;
        float offset_y = data->posy[particle_index] - particle_system->emitterY;
        data->posx[particle_index] = emitter_x + offset_x * cos_angle - offset_y * sin_angle;
        data->posy[particle_index] = emitter_y + offset_x * sin_angle + offset_y * cos_angle;
        float relative_x = data->relx[particle_index];
        float relative_y = data->rely[particle_index];
        data->relx[particle_index] = relative_x * cos_angle - relative_y * sin_angle;
        data->rely[particle_index] = relative_x * sin_angle + relative_y * cos_angle;
        float velocity_x = data->dirX[particle_index];
        float velocity_y = data->dirY[particle_index];
        data->dirX[particle_index] = velocity_x * cos_angle - velocity_y * sin_angle;
        data->dirY[particle_index] = velocity_x * sin_angle + velocity_y * cos_angle;
    }
}

void update_object_particles(float dt) {
    remove_offscreen_object_particles();
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            
            int obj = object_particle[i].id;
            float emitter_x = objects.x[obj];
            float emitter_y = objects.y[obj];
            if (!object_particle[i].rotation_base_set) {
                object_particle[i].rotation_base = objects.rotation[obj];
                object_particle[i].angle_base = object_particle[i].ps.cfg.angle;
                object_particle[i].last_rotation = objects.rotation[obj];
                object_particle[i].rotation_base_set = true;
            } else if (objects.rotation[obj] != object_particle[i].last_rotation) {
                float rotation_delta = objects.rotation[obj] - object_particle[i].last_rotation;
                object_particle[i].last_rotation = objects.rotation[obj];
                rotate_object_particles(&object_particle[i].ps, rotation_delta, emitter_x, emitter_y);
                object_particle[i].ps.cfg.angle = object_particle[i].angle_base - (objects.rotation[obj] - object_particle[i].rotation_base);
            }
            object_particle[i].ps.emitterX = emitter_x;
            object_particle[i].ps.emitterY = emitter_y;
            bool toggled = (objects.flags[obj] & FLAG_TOGGLED) != 0;
            if (objects.id[obj] == KEY_OBJ) {
                object_particle[i].ps.emitting = !toggled;
                if (toggled) object_particle[i].ps.data.count = 0;
                object_particle[i].ps.scale = fmaxf(fabsf(objects.scale_x[obj]), fabsf(objects.scale_y[obj]));
                object_particle[i].ps.cfg.startColorRed = objects.main_color[obj].r / 255.f;
                object_particle[i].ps.cfg.startColorGreen = objects.main_color[obj].g / 255.f;
                object_particle[i].ps.cfg.startColorBlue = objects.main_color[obj].b / 255.f;
            } else if (toggled) object_particle[i].ps.emitting = false;

            updateParticleSystem(&object_particle[i].ps, dt);
        }
    }
}


void draw_object_particles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (object_particle[i].occupied) {
            int obj = object_particle[i].id;
            float fade_x = 0;
            float fade_y = 0;

            float calc_x = ((objects.x[obj] - state.camera_x));
            
            float fade_scale = 1.f;
            get_fade_vars(obj, calc_x, &fade_x, &fade_y, &fade_scale);

            float fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);

            // Calculate stationary fade positions
            fade_x += get_special_fading_vars(obj, fade_val);

            drawParticleSystem(&object_particle[i].ps, fade_x, fade_y, (fade_val / 255.f) * objects.alpha_trigger_opacity[obj]);
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
        case GREEN_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 0);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case RED_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.39216f, 0);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLACK_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 1);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DASH_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0, 1, 0);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case GRAVITY_DASH_ORB:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0, 0);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case 1594:
            index = -1;
            for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
                if (object_particle[i].occupied && object_particle[i].id == obj) { index = (int)i; break; }
            }

            if (index < 0) {
                index = load_object_particles(obj, &ring_effect, false);
                if (index >= 0) {
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }

            if (index >= 0) {
                if (!object_particle[index].color_applied) {
                    Color base = channels[get_col_channel_index(objects.detail_col_channel[obj])].non_pulse_color;
                    set_particle_color(&object_particle[index].ps.cfg, base.r / 255.f, base.g / 255.f, base.b / 255.f);
                    object_particle[index].color_applied = true;
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
                    object_particle[index].ps.posVarRotates = true;
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
                    object_particle[index].ps.posVarRotates = true;
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
                    object_particle[index].ps.posVarRotates = true;
                }
            }
            break;
        case RED_PAD:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &bump_effect, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 0.19608f, 0.19608f);

                    object_particle[index].ps.cfg.angle = 180.f - (adjust_angle_y(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                    object_particle[index].ps.posVarRotates = true;
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
        case ROBOT_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1, 1, 1);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SPIDER_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0.752941f, 0.133333f, 0.972549f);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_TP_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 0.22f, 0.78f, 1.0f);

                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));

                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case ORANGE_TP_PORTAL:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &portal_effect_01, false);
                if (index >= 0) {
                    set_particle_color(&object_particle[index].ps.cfg, 1.0f, 91.0f/255.0f, 0.0f);

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
        case FASTEST_SPEED_PORTAL:
            if (!is_ps_already_loaded(obj))
            {
                index = load_object_particles(obj, &boost_04_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 1.f, 0.f, 0);
                    object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SECRET_COIN:
            if (!is_ps_already_loaded(obj))
            {
                if (is_coin_collected(obj)) break;
                
                index = load_object_particles(obj, &coin_effect, false);
                if (index >= 0)
                {
                    set_particle_color(&object_particle[index].ps.cfg, 1, state.custom_level ? 1.f : 0.75f, state.custom_level ? 1.f : 0);
                    // object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj] - 15;
                }
            }
            break;
        case KEY_OBJ:
            if (!is_ps_already_loaded(obj)) {
                index = load_object_particles(obj, &key_particles, false);
                if (index >= 0) {
                    object_particle[index].ps.emitterX = objects.x[obj];
                    object_particle[index].ps.emitterY = objects.y[obj];
                    object_particle[index].ps.posVarRotates = true;
                }
            }
            break;
    }
}