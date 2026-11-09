#include "player.hpp"
#include "level_loading.hpp"
#include "particles/circles.hpp"
#include "player/trail.hpp"
#include "practice.hpp"
#include "profiling.hpp"
#include "state.hpp"
#include "icons.hpp"
#include "graphics.hpp"

#include "particles/particles.hpp"

#include "slope.hpp"

#include "menus/icon_kit.hpp"
#include "collision.hpp"
#include "robot_anim_data.hpp"
#include "spider_anim_data.hpp"
#include "math_helpers.hpp"

#include "main.hpp"

#include "mp3_player.hpp"
#include "menus/settings_hub/settings.hpp"
#include "utils/gfx.hpp"

#include "easing.hpp"
#include <stdint.h>

#define SPIDER_DASH_SCALE_DURATION 0.45f
#define SPIDER_DASH_SCALE_START 0.4f
#define SPIDER_DASH_ELASTIC_PERIOD 0.35f

inline float gravFloor(Player *player) { return player->upside_down ? -state.ceiling_y : state.ground_y; }

void anim_player_to_wall(Player *player);
float expected_slope_y(int obj, Player *player);

KeyInput curr_input;
KeyInput curr_old_input;

MotionTrail *trail;
MotionTrail trail_p1;
MotionTrail trail_p2;

MotionTrail *wave_trail;
MotionTrail wave_trail_p1;
MotionTrail wave_trail_p2;

ParticleSystem drag_particles[2];
ParticleSystem drag_particles_2[2];
ParticleSystem ship_fire_particles[2];
ParticleSystem ship_secondary_particles[2];
ParticleSystem secondary_particles[2];
ParticleSystem burst_particles[2];
ParticleSystem robot_fire_particles[2];
ParticleSystem land_particles[2];
ParticleSystem explosion_particles[2];
ParticleSystem glitter_particles;
ParticleSystem slow_speed_particles;
ParticleSystem normal_speed_particles;
ParticleSystem fast_speed_particles;
ParticleSystem faster_speed_particles;
ParticleSystem fastest_speed_particles;
ParticleSystem coin_pickup_particles;

PlayerAction player_actions[2][MAX_ACTIONS];
int num_actions[2] = {0};

const float velocity_thresholds[SPEED_COUNT] = {
	101.541492,
	103.485494592,
	103.377492,
	103.809492,
	103.809492
};

extern const float player_speeds[SPEED_COUNT] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566,
	576.000192
};

const float cube_jump_heights[SPEED_COUNT] = {
    573.481728,
    603.7217172,
    616.681728,
    606.421728,
    606.421728,
};

const float cube_accelerations[SPEED_COUNT] = {
    -2747.52,
    -2794.1082,
    -2786.4,
    -2799.36,
    -2799.36,
};

const float robot_accelerations[SPEED_COUNT] = {
    -2467.4583,
    -2514.6975,
    -2512.0731,
    -2522.5707,
    -2522.5707,
};

const float player_speed_mults[SPEED_COUNT] = {
	0.7f,
	0.9f,
	1.1f,
	1.3f,
	1.6f
};

const float cube_rotation_speed[2] = {
    415.3848f,
    540.f
};

bool player_gamemode_is_flying(Player *player) {
    return player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_UFO || player->gamemode == GAMEMODE_WAVE;
}

void set_animation(Player *player, int anim_id) {
    if (player->anim_id != anim_id) {
        player->blend_from_anim_id = player->anim_id;
        player->blend_from_frame = player->anim_frame;
        player->blend_from_timer = player->anim_timer;

        player->anim_id = anim_id;
        player->anim_frame = 0;
        player->anim_timer = 0;
        player->blend_timer = 0;
    }
}

float get_blend_duration(Player *player) {
    if (player->gamemode == GAMEMODE_ROBOT) {
        return ROBOT_BLEND_DURATION;
    } else if (player->gamemode == GAMEMODE_SPIDER) {
        return SPIDER_BLEND_DURATION;
    }
    return 0.1f;
}

void set_blend_timer(Player *player, float timer) {
    player->blend_timer = timer;
}

void player_non_flying_landing(Player *player) {
    if (!player_gamemode_is_flying(player)) {
        player->landed_from_jump |= player->jumped;
        player->jumped = false;
    }
}

float player_get_vel(Player *player, float vel) {
    return vel * (player->upside_down ? -1 : 1);
}

void set_p_velocity(Player *player, float vel, bool override) {
    player->velocity_override = override;
    player->vel_y = vel * ((player->mini) ? 0.8 : 1);
}

void update_rotation_direction(Player *player) {
    player->rotation_direction = (player->upside_down ? -1 : 1);
}

void stop_dashing(Player *player) {
    if (!player->dashing) return;
    add_dash_exit(player);
    player->dashing = false;
    player->dash_fire_timer = 0.f;
}

void player_flash(Player *player, float duration, float delay) {
    player->flash_active = true;
    player->flash_timer = 0.f;
    player->flash_duration = duration;
    player->flash_delay = delay;
}

float convert_to_closest_rotation(float rotation, float angle) {
    float rot = (int)rotation % 360;

    if (rot < 0)
        rot += 360.0f;

    float snapped = roundf((rot - angle) / 90.0f) * 90.0f + angle;

    snapped = fmodf(snapped, 360.0f);

    if (snapped < 0)
        snapped += 360.0f;

    return snapped;
}

void cube_gamemode(Player *player) {
    int mult = player->rotation_direction;
    
    trail->positionR = (Vec2D){player->x, player->y};  
    trail->startingPositionInitialized = true;

    player->gravity = cube_accelerations[state.speed];

    player->min_vel = -810;
    player->max_vel = 1080;

    // Limit player speed (there is a upwards cap interestingly)
    if (player->vel_y < player->min_vel) player->vel_y = player->min_vel;
    if (player->vel_y > player->max_vel) player->vel_y = player->max_vel;

    // If player fell above the level, die
    if (player->y > 2794.f) kill_player(DEATH_FELL_OFF_LEVEL);

    if (player->snap_rotation) {
        player->cube_target_rotation = player->rotation;
    }

    // Do cube rotation
    if (player->slope_data.slope_id < 0 && !player->on_ground && !player->dashing) {
        if (player->inverse_rotation) {
            player->cube_target_rotation -= (cube_rotation_speed[player->mini] / 2) * STEPS_DT * mult;
        } else {
            player->cube_target_rotation += cube_rotation_speed[player->mini] * STEPS_DT * mult;
        }
    }

    bool jump = false;

    drag_particles[state.current_player].emitterX = getLeft(player);
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -2 : 2);
    drag_particles[state.current_player].emitting = player->time_since_ground < DRAG_PARTICLES_FLOOR_DURATION;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles[state.current_player].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    if (curr_input.holdJump && !player->dashing) {
        jump = true;
    } else if (player->on_ground) {
        player->consecutive_jumps = 0;
    }

    // J block
    if (player->holding_jump_cancelled && player->buffering_state == BUFFER_END) {
        jump = false;
    }

    if (player->on_ground) {
        MotionTrail_StopStroke(trail);
        update_rotation_direction(player);
    }

    bool should_coyote = (state.dual || player->upside_down) && curr_input.holdJump && player->coyote_frames < 10;

    SlopeData slope_data = player->slope_data;

    // If not currently on slope, look at the last frame
    if (player->slope_data.slope_id < 0 && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }

    if (((slope_data.slope_id >= 0 && grav_slope_orient(slope_data.slope_id, player) < ORIENT_UD_DOWN) || player->on_ground || should_coyote) && jump) {
        // If on slope, the player jumps depending on time on slope
        if (slope_data.slope_id >= 0) {
            // Slope jump
            int orient = grav_slope_orient(slope_data.slope_id, player);
            if (orient == ORIENT_NORMAL_UP) {
                float time = clampf(10 * (player->timeElapsed - slope_data.elapsed), 0.4f, 1.0f);
                float vel = 0.9f * MIN(1.12f / slope_angle(slope_data.slope_id, player), 1.54f) * (objects.height[slope_data.slope_id] * player_speeds[state.speed] / objects.width[slope_data.slope_id]);
                set_p_velocity(player, 0.25f * time * vel + cube_jump_heights[state.speed], false);
                player->slope_slide_coyote_time = 0;
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed], curr_old_input.holdJump);
            }
        } else {
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed], curr_old_input.holdJump);
        }
        player->inverse_rotation = false;
        player->buffering_state = BUFFER_END;
    
        player->on_ground = false;
        player->consecutive_jumps++;
        player->jumped = true;
        
        state.current_data.jumps++;
    
        if (!(curr_input.pressedJump)) {
            // This prevents drag particles on succesive jumps
            player->time_since_ground = DRAG_PARTICLES_FLOOR_DURATION;
        }
    }

    // If not in a slope, snap to nearest 90 degree angle
    if (player->on_ground && player->slope_data.slope_id < 0) {
        player->cube_target_rotation = convert_to_closest_rotation(player->rotation, 0);
    }

    if (player->slope_data.slope_id >= 0) {
        float base = RadToDeg(slope_snap_angle(player->slope_data.slope_id, player));
        player->cube_target_rotation = convert_to_closest_rotation(player->rotation, base);
    }
    if (player->slope_slide_coyote_time) {
        float base = RadToDeg(slope_snap_angle(player->coyote_slope.slope_id, player));
        player->cube_target_rotation = convert_to_closest_rotation(player->rotation, base);
    }
}

void rotate_fly(Player *player, float mult) {
    float diff_x = (player->x - state.old_player.x);
    float diff_y = (player->y - state.old_player.y);
    float angle_rad = atan2f(-diff_y, diff_x);
 
    if (player->snap_rotation) {
        player->rotation = RadToDeg(angle_rad);
    } else if (STEPS_DT * 72 <= diff_x * diff_x + diff_y * diff_y) {
        // This is how gd does rotation
        if (player->gamemode == GAMEMODE_UFO) {
            if (player->slope_data.slope_id >= 0) {
                angle_rad = slope_snap_angle(player->slope_data.slope_id, player);
            } else if (player->on_ground) {
                angle_rad = 0;
            } else if (!player->upside_down) {
                angle_rad = MAX(angle_rad * -0.4f, -0.1f);
            } else {
                angle_rad = MIN(angle_rad * -0.4f, 0.1f);
            }
        }

        player->rotation = RadToDeg(slerp_fancy(DegToRad(player->rotation), angle_rad, (STEPS_DT * 60) * mult));
	}
}

// Ball rotation multiplier
float get_ball_rotation_speed(Player *player) {
    float speed = (player->mini ? 0.16f : 0.2f);
    switch (state.speed) {
        case 0:
            speed *= 1.2405638f;
            break;
        case 2:
            speed *= 0.80424345f;
            break;
        case 3:
            speed *= 0.6657693f;
            break;
        default:
            break;
    }

    return 120 / speed;
}

void ship_gamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;

    float rad = C3D_AngleFromDegrees(player->rotation);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_y_mult = (player->upside_down ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = -sin_r;
    float m11 = cos_r;

    const float local_x = -14;
    const float local_y = -8 * flip_y_mult;

    float rot_x = local_x * m00 + local_y * m01;
    float rot_y = local_x * m10 + local_y * m11;

    float x = player->x + rot_x * scale;
    float y = player->y + rot_y * scale;
    
    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;
    float calc_x = player->x - state.camera_x;
    float calc_y = SCREEN_HEIGHT - (fabsf(gravBottom(player)) - state.camera_y);

    ship_fire_particles[state.current_player].emitterX = x;
    ship_fire_particles[state.current_player].emitterY = y;
    ship_fire_particles[state.current_player].emitting = curr_input.holdJump;

    ship_fire_particles[state.current_player].gravityFlipped = player->upside_down;
    ship_fire_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    ship_fire_particles[state.current_player].cfg.sourcePositionVariancey = (player->mini ? 4.f : 3.f);

    ship_secondary_particles[state.current_player].emitterX = x;
    ship_secondary_particles[state.current_player].emitterY = y;
    ship_secondary_particles[state.current_player].emitting = true;

    ship_secondary_particles[state.current_player].gravityFlipped = player->upside_down;
    ship_secondary_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    ship_secondary_particles[state.current_player].cfg.sourcePositionVariancey = (player->mini ? 4.5f : 2.f);

    drag_particles_2[state.current_player].emitterX = calc_x;
    drag_particles_2[state.current_player].emitterY = calc_y;
    drag_particles_2[state.current_player].emitting = player->on_ground;

    drag_particles_2[state.current_player].gravityFlipped = !player->upside_down;
    drag_particles_2[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    player->min_vel = player->mini ? -406.566f : -345.6f;
    player->max_vel = player->mini ? 508.248f : 432.0f;

    if (player->velocity_uncapped) {
        if ((player->vel_y >= 0.f && player->vel_y < player->max_vel) ||
            (player->vel_y <= 0.f && player->vel_y > player->min_vel)) {
            player->velocity_uncapped = false;
        }
    }

    if (player->velocity_uncapped) {
        float threshold = state.dual ? -velocity_thresholds[state.speed] : grav(player, velocity_thresholds[state.speed]);
        bool legacy_falling = player->vel_y <= threshold;
        float ship_mod = (player->vel_y < 0.f) ? -1.f : 0.8f;
        if (!curr_input.holdJump && !legacy_falling) ship_mod = 1.2f;
        float boost = (curr_input.holdJump && legacy_falling) ? 0.5f : 0.4f;
        player->gravity = -ship_mod * boost * (player->mini ? 3287.186216f : 2794.108284f);
        if (curr_input.holdJump) player->buffering_state = BUFFER_END;
    } else if (state.dual) {
        // Make both dual players symmetric by using inverted ship gravity
        if (curr_input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= -velocity_thresholds[state.speed])
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= -velocity_thresholds[state.speed])
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (curr_input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= grav(player, velocity_thresholds[state.speed]))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= grav(player, velocity_thresholds[state.speed]))
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    }

    if (!player->velocity_uncapped) {
        if (player->gravity < 0 && player->vel_y < player->min_vel) {
            player->vel_y = player->min_vel;
        } else if (player->gravity > 0 && player->vel_y > player->max_vel) {
            player->vel_y = player->max_vel;
        }
    }
}


static float ballJumpHeights[SPEED_COUNT] = {
    -172.044007,
    -181.11601,
    -185.00401,
    -181.92601,
    -181.92601
};

void ball_gamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};  
    trail->startingPositionInitialized = true;

    int mult = (player->upside_down ? -1 : 1);

    // If player left slope, set gravity
    if (!state.old_player.velocity_override || state.old_player.slope_data.slope_id >= 0)
        player->gravity = -1676.46672f;  
    
    if (player->on_ground || player->on_ceiling) {
        MotionTrail_StopStroke(trail);
        player->ball_rotation_speed = 1.f;
    }
    
    drag_particles[state.current_player].emitterX = player->x;
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -2 : 2);
    drag_particles[state.current_player].emitting = player->on_ground || player->on_ceiling;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles[state.current_player].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    bool should_coyote = (state.dual || player->upside_down) && player->coyote_frames < 16;
    // If on ground (block or slope) and its buffering, do a jump
    if ((player->slope_data.slope_id >= 0 || player->on_ground || player->on_ceiling || should_coyote) && player->buffering_state == BUFFER_READY && !player->dashing) {        
        player->upside_down ^= 1;

        set_p_velocity(player, ballJumpHeights[state.speed], state.old_player.buffering_state == BUFFER_READY);

        if (player->slope_data.slope_id >= 0 && grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_NORMAL_UP)
            player->vel_y -= player->gravity * STEPS_DT;

        player->buffering_state = BUFFER_END;
        
        state.current_data.jumps++;
        
        player->ball_rotation_speed = -BALL_SLOW_ROTATION;

        player->on_ground = false;
        player->jumped = true;
    }
    
    if (!player->dashing) player->rotation += player->ball_rotation_speed * get_ball_rotation_speed(player) * mult * STEPS_DT;

    player->min_vel = -810;
    player->max_vel = 810;

    if (player->vel_y < player->min_vel) {
        player->vel_y = player->min_vel;
    } else if (player->vel_y > player->max_vel) {
        player->vel_y = player->max_vel;
    }
}

void ufo_gamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;

    float rad = C3D_AngleFromDegrees(player->rotation);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_y_mult = (player->upside_down ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = -sin_r;
    float m11 = cos_r;

    const float local_x = 0;
    const float local_y = -8 * flip_y_mult;

    float rot_x = local_x * m00 + local_y * m01;
    float rot_y = local_x * m10 + local_y * m11;

    float x = player->x + rot_x * scale;
    float y = player->y + rot_y * scale;
    
    float calc_x = player->x - state.camera_x;
    float calc_y = SCREEN_HEIGHT - (fabsf(gravBottom(player)) - state.camera_y);

    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;

    secondary_particles[state.current_player].emitterX = player->x;
    secondary_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
    secondary_particles[state.current_player].emitting = true;

    secondary_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles_2[state.current_player].emitterX = calc_x;
    drag_particles_2[state.current_player].emitterY = calc_y;
    drag_particles_2[state.current_player].emitting = player->on_ground;

    drag_particles_2[state.current_player].gravityFlipped = !player->upside_down;
    drag_particles_2[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    bool buffering_check = ((state.old_player.gamemode == GAMEMODE_PLAYER || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_WAVE || player->buffer_ufo) && (curr_input.holdJump));
    // If buffering, jump
    if (player->buffering_state == BUFFER_READY && (curr_input.pressedJump || buffering_check) && !player->dashing) {
        player->vel_y = fmaxf(player->vel_y, player->mini ? 358.992 : 371.034);
        player->buffering_state = BUFFER_END;
        player->velocity_override = true;
        player->burst_particle_timer = BURST_PARTICLES_DURATION;
        player->velocity_uncapped = false;
    } else {
        // Same as ship
        if (!state.dual) {
            if (player->vel_y > grav(player, velocity_thresholds[state.speed])) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        } else {   
            if (player->vel_y > -velocity_thresholds[state.speed]) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        }
    }

    if (player->burst_particle_timer > 0) {
        player->burst_particle_timer -= STEPS_DT;
    }

    burst_particles[state.current_player].emitterX = player->x;
    burst_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
    burst_particles[state.current_player].emitting = player->burst_particle_timer > 0;

    burst_particles[state.current_player].gravityFlipped = player->upside_down;
    burst_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    player->min_vel = player->mini ? -406.566f : -345.6f;
    player->max_vel = player->mini ? 508.248f : 432.0f;

    if (!player->velocity_uncapped) {
        if (player->vel_y < player->min_vel) {
            player->vel_y = player->min_vel;
        } else if (player->vel_y > player->max_vel) {
            player->vel_y = player->max_vel;
        }
    }
}

void wave_gamemode(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;
    float rad = C3D_AngleFromDegrees(player->rotation);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = -sin_r;
    float m11 = cos_r;

    const float local_x = -8;
    const float local_y = 0;

    float rot_x = local_x * m00 + local_y * m01;
    float rot_y = local_x * m10 + local_y * m11;

    float x = player->x + rot_x * scale;
    float y = player->y + rot_y * scale;
    
    trail->positionR = (Vec2D){x, y};  
    trail->startingPositionInitialized = true;
 
    if (game_state == STATE_GAME && player->cutscene_timer == 0 && !state.mirroring) wave_trail->opacity = 1.f;

    if (player->buffering_state == BUFFER_READY) player->buffering_state = BUFFER_END;

    bool input = (curr_input.holdJump);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
}

static float robot_fire_in_scale(float timer) {
    if (timer < 0.05f) return 0.1f + (1.44f - 0.1f) * (timer / 0.05f);
    if (timer < 0.1f) return 1.44f + (0.72f - 1.44f) * ((timer - 0.05f) / 0.05f);
    if (timer < 0.15f) return 0.72f + (1.8f - 0.72f) * ((timer - 0.1f) / 0.05f);
    if (timer < 0.2f) return 1.8f + (0.9f - 1.8f) * ((timer - 0.15f) / 0.05f);
    if (timer < 0.25f) return 0.9f + (1.8f - 0.9f) * ((timer - 0.2f) / 0.05f);
    return 1.8f;
}

static void update_robot_fire(Player *player) {
    bool show = player->gamemode == GAMEMODE_ROBOT
             && !player->on_ground
             && player->gravity == 0.f
             && player->robot_air_time > 0.f
             && player->robot_air_time < ROBOT_FIRE_MAX_AIR_TIME;

    if (show) {
        if (!player->robot_fire_visible || player->robot_fire_out) {
            player->robot_fire_visible = true;
            player->robot_fire_out = false;
            player->robot_fire_timer = 0.f;
            player->robot_fire_scale = 0.1f;
        } else {
            player->robot_fire_timer += STEPS_DT;
            player->robot_fire_scale = robot_fire_in_scale(player->robot_fire_timer);
        }
    } else if (player->robot_fire_visible) {
        if (!player->robot_fire_out) {
            player->robot_fire_out = true;
            player->robot_fire_timer = 0.f;
            player->robot_fire_exit_scale = player->robot_fire_scale;
        } else {
            player->robot_fire_timer += STEPS_DT;
            float progress = CLAMP(player->robot_fire_timer / ROBOT_FIRE_OUT_TIME, 0.f, 1.f);
            player->robot_fire_scale = player->robot_fire_exit_scale + (ROBOT_FIRE_MIN_SCALE - player->robot_fire_exit_scale) * progress;
            if (player->robot_fire_timer >= ROBOT_FIRE_OUT_TIME) player->robot_fire_visible = false;
        }
    }
}

void robot_gamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};
    trail->startingPositionInitialized = true;

    player->min_vel = -810;
    player->max_vel = 1080;

    if (player->vel_y < player->min_vel) player->vel_y = player->min_vel;
    if (player->vel_y > player->max_vel) player->vel_y = player->max_vel;

    if (player->y > 2794.f) kill_player(DEATH_FELL_OFF_LEVEL);

    if (player->on_ground) {
        set_animation(player, ROBOT_ANIM_RUN);
        MotionTrail_StopStroke(trail);
        if (player->slope_data.slope_id < 0 && !player->dashing) player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }
    
    drag_particles[state.current_player].emitterX = player->x;
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -2 : 2);
    drag_particles[state.current_player].emitting = player->on_ground || player->on_ceiling;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles[state.current_player].cfg.sourcePositionVariancey = (player->mini ? 4.f : 2.f);

    SlopeData slope_data = player->slope_data;

    bool should_coyote = (state.dual || player->upside_down) && curr_input.holdJump && player->coyote_frames < 11;

    if (player->slope_data.slope_id < 0 && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }

    if (player->snap_rotation) {
        player->rotation = 0;
    }

    if ((slope_data.slope_id >= 0 || player->on_ground || should_coyote) && (curr_input.holdJump && player->buffering_state == BUFFER_READY) && !player->dashing) {
        set_p_velocity(player, cube_jump_heights[state.speed] / 2, false);
        player->inverse_rotation = false;
        player->on_ground = false;
        set_animation(player, ROBOT_ANIM_JUMP_START);
        player->buffering_state = BUFFER_END;
        player->robot_air_time = 0.f;
        player->gravity = 0;
        player->jumped = true;
    }

    if (player->robot_air_time >= 1.5f || (!curr_input.holdJump)) {
        player->gravity = robot_accelerations[state.speed];
        if (player->anim_id == ROBOT_ANIM_JUMP) {
            set_animation(player, ROBOT_ANIM_FALL_START);
        }
    } else {
        player->robot_air_time += 5.4f * STEPS_DT;
    }

    float scale = (player->mini) ? 0.6f : 1.f;
    robot_fire_particles[state.current_player].emitterX = player->x - 4.f * scale;
    robot_fire_particles[state.current_player].emitterY = player->y - grav(player, 10.f) * scale;
    robot_fire_particles[state.current_player].emitting = player->gravity == 0;

    robot_fire_particles[state.current_player].scale = scale;

    if (!player->on_ground && player->vel_y < 0
        && player->anim_id != ROBOT_ANIM_FALL_START
        && player->anim_id != ROBOT_ANIM_FALL) {
        set_animation(player, ROBOT_ANIM_FALL_START);
    }

    const PlayerAnimation *anim = &robot_animations[player->anim_id];
    float anim_speed = (state.speed == SPEED_SLOW) ? 0.9f
                     : (state.speed == SPEED_FAST) ? 1.15f
                     : (state.speed == SPEED_FASTER) ? 1.3f
                     : (state.speed == SPEED_FASTEST) ? 1.5f : 1.0f;
    if (player->mini) anim_speed /= 0.85f;
    
    anim_speed *= 35.0f / 24.0f;
    player->anim_timer += STEPS_DT * anim_speed;

    if (player->anim_timer >= anim->frames[player->anim_frame].delay) {
        player->anim_timer -= anim->frames[player->anim_frame].delay;
        int next = player->anim_frame + 1;
        if (next >= anim->frame_count) {
            if (player->anim_id == ROBOT_ANIM_JUMP_START) {
                set_animation(player, ROBOT_ANIM_JUMP);
            } else if (player->anim_id == ROBOT_ANIM_FALL_START) {
                set_animation(player, ROBOT_ANIM_FALL);
            }
            next = 0;
        }
        player->anim_frame = next;
    }
}

#define SPIDER_RAY_MAX 256

static int spider_solid_buffer[SPIDER_RAY_MAX];
static int spider_hazard_buffer[SPIDER_RAY_MAX];

static bool spider_overlap(float player_x, float player_y, float player_width, float player_height, float object_x, float object_y, float object_width, float object_height) {
    return fabsf(player_x - object_x) < (player_width + object_width) * 0.5f && fabsf(player_y - object_y) < (player_height + object_height) * 0.5f;
}

static bool spider_rect_hits(int obj, float min_x, float min_y, float max_x, float max_y) {
    const ObjectHitbox *hitbox = game_objects[objects.id[obj]].hitbox;

    float object_x = objects.x[obj] + hitbox->x * objects.scale_x[obj];
    float object_y = objects.y[obj] + hitbox->y * objects.scale_y[obj];
    float half_width = objects.width[obj] * 0.5f;
    float half_height = objects.height[obj] * 0.5f;

    return min_x < object_x + half_width && max_x > object_x - half_width && min_y < object_y + half_height && max_y > object_y - half_height;
}

static int spider_collect(float min_x, float min_y, float max_x, float max_y, int *out, int capacity, bool is_hazard) {
    int count = 0;

    int solid_min_x = (int)floorf(min_x / SECTION_SIZE);
    int solid_max_x = (int)floorf(max_x / SECTION_SIZE);
    int solid_min_y = (int)floorf(min_y / SECTION_SIZE);
    int solid_max_y = (int)floorf(max_y / SECTION_SIZE);

    for (int sy = solid_min_y; sy <= solid_max_y; sy++) {
        for (int sx = solid_min_x; sx <= solid_max_x; sx++) {
            Section *sec = get_section(sx, sy);

            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];
                int id = objects.id[obj];

                if (!is_valid_object(id)) continue;

                const ObjectHitbox *hitbox = game_objects[id].hitbox;

                if (!hitbox) continue;
                if (objects.flags[obj] & FLAG_TOGGLED) continue;
                if (is_hazard) {
                    if (hitbox->collision_type != HITBOX_HAZARD) continue;
                } else if (hitbox->collision_type != HITBOX_SOLID) continue;

                if (!spider_rect_hits(obj, min_x, min_y, max_x, max_y)) continue;

                if (count < capacity) out[count++] = obj;
            }
        }
    }

    return count;
}

static void spider_sort(int *list, int count, bool upside_down) {
    for (int i = 1; i < count; i++) {
        int obj = list[i];
        float key = upside_down ? -obj_getTop(obj) : obj_getBottom(obj);
        int j = i - 1;

        while (j >= 0) {
            float prev = upside_down ? -obj_getTop(list[j]) : obj_getBottom(list[j]);
            if (prev <= key) break;
            list[j + 1] = list[j];
            j--;
        }

        list[j + 1] = obj;
    }
}

void spider_test_jump(Player *player, bool is_retry) {
    if (state.dead) return;

    float player_width = player->width;
    float player_height = player->height;
    float player_left = player->x - player_width * 0.5f;
    float player_bottom = player->y - player_height * 0.5f;
    float player_top = player->y + player_height * 0.5f;

    float min_teleport = MAX(state.ground_y, player->y - 3000.0f);
    float max_teleport = MIN(state.ceiling_y, player->y + 3000.0f);

    float tolerance = 2.0f;

    float solid_min_y = !player->upside_down ? (player_top - tolerance) : min_teleport;
    float solid_max_y = !player->upside_down ? max_teleport : (player_bottom + tolerance);

    bool going_left = state.mirror_mult < 0;

    float solid_min_x = going_left ? (player_left - player_width * 0.5f - 1.0f) : (player_left + player_width * 0.5f);
    float solid_max_x = solid_min_x + (player_width * 0.5f + 1.0f);

    float hazard_min_x = player_left;
    float hazard_max_x = player_left + player_width;
    float extra_min_x = hazard_min_x;
    float extra_max_x = hazard_max_x + 4.0f;

    float hazard_width = (player->mini ? 0.6f : 1.0f) * 8.0f;
    float hazard_narrow_min_x = hazard_min_x + (solid_max_x - solid_min_x - hazard_width) * 0.5f;
    float hazard_narrow_max_x = hazard_narrow_min_x + hazard_width;

    int solid_count = spider_collect(solid_min_x, solid_min_y, solid_max_x, solid_max_y, spider_solid_buffer, SPIDER_RAY_MAX, false);
    int hazard_count = spider_collect(hazard_narrow_min_x, solid_min_y, hazard_narrow_max_x, solid_max_y, spider_hazard_buffer, SPIDER_RAY_MAX, true);

    bool only_damaging = (solid_count == 0 && hazard_count != 0);

    if (only_damaging || is_retry) {
        is_retry = true;
        solid_count = spider_collect(extra_min_x, solid_min_y, extra_max_x, solid_max_y, spider_solid_buffer, SPIDER_RAY_MAX, false);
    }

    spider_sort(spider_solid_buffer, solid_count, player->upside_down);
    spider_sort(spider_hazard_buffer, hazard_count, player->upside_down);

    float prev_land_y = 0.0f;
    float land_y = 0.0f;

    if (only_damaging) {
        int nearest = spider_hazard_buffer[0];
        land_y = objects.y[nearest];
    } else {
        int i = 0;

        for (i = 0; i < solid_count; i++) {
            int obj = spider_solid_buffer[i];
            bool checking_slope = false;

            if (!player->upside_down) {
                land_y = obj_getBottom(obj) - player_height * 0.5f;
            } else {
                land_y = obj_getTop(obj) + player_height * 0.5f;
            }

            if (game_objects[objects.id[obj]].hitbox->type == COLLISION_SLOPE) {
                checking_slope = true;
                land_y = expected_slope_y(obj, player);
            }

            if ((!player->upside_down && land_y < player->y - 10.0f) ||
                (player->upside_down && land_y > player->y + 10.0f)) continue;

            if (!going_left) {
                if (obj_getLeft(obj) <= player->x) break;
            } else {
                if (obj_getRight(obj) >= player->x) break;
            }

            bool hit = false;

            for (int j = 0; j < solid_count; j++) {
                if (j == i) continue;

                int inner = spider_solid_buffer[j];
                float object_height = objects.height[inner] - (checking_slope ? 0.0f : 4.0f);

                if (spider_overlap(player->x, land_y, player_width, player_height, objects.x[inner], objects.y[inner], objects.width[inner], object_height)) {
                    if (checking_slope) {
                        if (prev_land_y != 0.0f) land_y = !player->upside_down ? MIN(land_y, prev_land_y) : MAX(land_y, prev_land_y);
                    } else {
                        land_y = prev_land_y;
                    }

                    hit = true;
                    prev_land_y = land_y;
                    break;
                }
            }

            if (!hit) break;
        }

        if (i >= solid_count) land_y = 0.0f;
    }

    if (land_y == 0.0f) {
        land_y = player->upside_down ? (min_teleport + player_height * 0.5f) : (max_teleport - player_height * 0.5f);
    }

    if (!only_damaging && hazard_count != 0) {
        float hazard_y = objects.y[spider_hazard_buffer[0]];

        if ((!player->upside_down && hazard_y < land_y) ||
            (player->upside_down && hazard_y > land_y)) {
            if (!is_retry) {
                spider_test_jump(player, true);
                return;
            }

            land_y = hazard_y;
        }
    }

    float old_y = player->y;
    player->y = land_y;
    player->upside_down = !player->upside_down;
    player->vel_y = player->upside_down ? 1.0f : -1.0f;
    player->on_ground = false;
    player->on_ceiling = false;
    player->left_ground = true;
    player->inverse_rotation = false;
    player->snap_rotation = true;
    player->buffering_state = BUFFER_END;

    Color dash_color = get_white_if_black(state.current_player == 1 ? p1_color : p2_color);
    add_spider_dash_effect(player->x, old_y, land_y, player->upside_down, dash_color);

    player_flash(player, 0.3f, 0.05f);

    player->spider_scale_active = true;
    player->spider_scale_timer = 0.f;

    UseEffect *effect = add_use_effect(player->x, land_y, USE_EFFECT_OBJ_NOTHING, &spider_in_effect, get_use_effect_array_ptr(SCREEN_TOP));
    if (effect) {
        Color p2_not_white = get_white_if_black(p2_color);

        effect->def.colorR = p2_not_white.r / 255.f;
        effect->def.colorG = p2_not_white.g / 255.f;
        effect->def.colorB = p2_not_white.b / 255.f;
        effect->def.start_rad = spider_in_effect.start_rad * (player->mini ? 0.6f : 1);
    }
    effect = add_use_effect(player->x, old_y, USE_EFFECT_OBJ_NOTHING, &spider_out_effect, get_use_effect_array_ptr(SCREEN_TOP));
    if (effect) {
        Color p2_not_white = get_white_if_black(p2_color);

        effect->def.colorR = p2_not_white.r / 255.f;
        effect->def.colorG = p2_not_white.g / 255.f;
        effect->def.colorB = p2_not_white.b / 255.f;
        effect->def.start_rad = spider_out_effect.start_rad * (player->mini ? 0.6f : 1);
    }
    update_rotation_direction(player);
    clear_slope_data(player);
    
    MotionTrail_Clear(trail);
    trail->positionR = (Vec2D){player->x, player->y};
}

void spider_gamemode(Player *player) {
    trail->positionR = (Vec2D){player->x, player->y};
    trail->startingPositionInitialized = true;

    player->min_vel = -810;
    player->max_vel = 810;

    if (!player->upside_down && player->vel_y < player->min_vel) player->vel_y = player->min_vel;
    if (player->upside_down && player->vel_y > player->max_vel) player->vel_y = player->max_vel;

    player->gravity = -2794.1082f * 0.6f;

    if (player->y > 2794.f) kill_player(DEATH_FELL_OFF_LEVEL);

    if (player->on_ground) {
        set_animation(player, (state.speed <= SPEED_NORMAL) ? SPIDER_ANIM_WALK : SPIDER_ANIM_RUN);
        MotionTrail_StopStroke(trail);

        if (player->slope_data.slope_id < 0 && !player->dashing) player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }

    if (player->snap_rotation) {
        player->rotation = 0;
    }

    drag_particles[state.current_player].emitterX = player->x;
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -2 : 2);
    drag_particles[state.current_player].emitting = player->on_ground || player->on_ceiling;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles[state.current_player].cfg.sourcePositionVariancey = 0;

    bool came_from_black_orb = (player->last_orb_pressed >= 0 && objects.id[player->last_orb_pressed] == BLACK_ORB);

    if ((player->slope_data.slope_id >= 0 || player->on_ground) && (curr_input.holdJump && (player->buffering_state == BUFFER_READY || came_from_black_orb)) && !player->dashing) {
        spider_test_jump(player, false);
        player->buffering_state = BUFFER_END;
        player->jumped = true;
        player->last_orb_pressed = -1;
    }

    if (!player->on_ground && player->vel_y < 0 && player->anim_id != SPIDER_ANIM_FALL) {
        set_animation(player, SPIDER_ANIM_FALL);
    }

    const PlayerAnimation *anim = &spider_animations[player->anim_id];

    float anim_speed = (state.speed == SPEED_SLOW) ? 0.9f
                     : (state.speed == SPEED_FAST) ? 1.15f
                     : (state.speed == SPEED_FASTER) ? 1.3f
                     : (state.speed == SPEED_FASTEST) ? 1.5f : 1.0f;

    if (player->mini) anim_speed /= 0.85f;
    if (state.speed >= SPEED_FAST) anim_speed /= 1.1f;

    player->anim_timer += STEPS_DT * anim_speed;

    if (player->anim_timer >= anim->frames[player->anim_frame].delay) {
        player->anim_timer -= anim->frames[player->anim_frame].delay;

        int next = player->anim_frame + 1;

        if (next >= anim->frame_count) next = 0;

        player->anim_frame = next;
    }
}

void clamp_player_ground(Player *player) {
    bool slopeCheck = player->slope_data.slope_id >= 0 && (grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_UD_DOWN);

    // Check for ground collision
    if (getGroundBottom(player) < state.ground_y && !player->just_teleported) {
        if (player->ceiling_inv_time <= 0 && (player->gamemode == GAMEMODE_PLAYER || player->gamemode == GAMEMODE_ROBOT) && player->upside_down) {
            kill_player(DEATH_CEILING);
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_WAVE && grav(player, player->vel_y) <= 0) set_p_velocity(player, 0, player->gamemode == GAMEMODE_BALL);
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0);;
        player->snap_data.player_frame = 0;    
    }

    // Check for ceiling collision
    if (getGroundTop(player) > state.ceiling_y && !player->just_teleported) {
        if (player->ceiling_inv_time <= 0 && (player->gamemode == GAMEMODE_PLAYER || player->gamemode == GAMEMODE_ROBOT) && !player->upside_down) {
            kill_player(DEATH_CEILING);
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_WAVE && grav(player, player->vel_y) >= 0) set_p_velocity(player, 0, player->gamemode == GAMEMODE_BALL);
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0);    
        
    } 
}
void run_player(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;
    trail->stroke = trail->width * scale;
    wave_trail->stroke = 18.f * map_range(amplitude, 0.f, 1.f, 0.1f, 1.f) * scale;

    if (!player->left_ground) {
        // Ground
        if (getGroundBottom(player) <= state.ground_y) {
            if (player->upside_down) {
                player->on_ceiling = true;
                player->inverse_rotation = false;
            } else {
                player->on_ground = true;      
                player->inverse_rotation = false;
                player_non_flying_landing(player);
            }
            player->time_since_ground = 0; 
        } 

        // Ceiling
        if (getGroundTop(player) >= state.ceiling_y) {
            if (player->upside_down) {
                player->on_ground = true;
                player->inverse_rotation = false;
                player_non_flying_landing(player);
            } else {
                player->on_ceiling = true;     
                player->inverse_rotation = false;     
            } 
            player->time_since_ground = 0; 
        } 
    }
    
    if (state.current_player == 0) handle_auto_checkpoints(STEPS_DT);
    else if (state.just_placed_auto_checkpoint) update_checkpoint_with_p2();
    
    // Handle land particles
    if (player->gamemode != GAMEMODE_WAVE && !state.old_player.on_ground && player->on_ground) {
        land_particles[state.current_player].emitterX = player->x;
        land_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
        land_particles[state.current_player].gravityFlipped = player->upside_down;
        land_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);
        spawnMultipleParticles(&land_particles[state.current_player], 10);
    }

    // Coyote time (only applies to upside down gravity)
    if (gravBottom(&state.old_player) > gravFloor(&state.old_player) && player->upside_down == state.old_player.upside_down && !player->on_ground && player->vel_y <= 0) {
		if (state.old_player.on_ground && !curr_old_input.holdJump)
			player->coyote_frames = 0;
		player->coyote_frames++;
	} else {
		player->coyote_frames = INT32_MAX;
	}

    // Disable cap
    if (player->on_ground) player->velocity_uncapped = false;

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            cube_gamemode(player);
            break;
        case GAMEMODE_SHIP:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring && state.show_player) MotionTrail_ResumeStroke(trail);
            ship_gamemode(player);
            break;
        case GAMEMODE_BALL:
            ball_gamemode(player);
            break;
        case GAMEMODE_UFO:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring && state.show_player) MotionTrail_ResumeStroke(trail);
            ufo_gamemode(player);
            break;
        case GAMEMODE_WAVE:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring && state.show_player) {
                if (settingsState.noWaveTrailBehind) {
                    MotionTrail_StopStroke(trail);
                } else {
                    MotionTrail_ResumeStroke(trail);
                }
            }
            wave_gamemode(player);
            break;
        case GAMEMODE_ROBOT:
            robot_gamemode(player);
            break;
        case GAMEMODE_SPIDER:
            spider_gamemode(player);
            break;
    } 
    
    if (player->flash_active) {
        player->flash_timer += STEPS_DT;
        if (player->flash_timer >= player->flash_delay + player->flash_duration) player->flash_active = false;
    }

    if (player->spider_scale_active) {
        player->spider_scale_timer += STEPS_DT;
        if (player->spider_scale_timer >= SPIDER_DASH_SCALE_DURATION) player->spider_scale_active = false;
    }

    update_robot_fire(player);

    player->blend_timer += STEPS_DT;
    if (player->blend_timer > get_blend_duration(player)) {
        set_blend_timer(player, get_blend_duration(player));
    }
    
    player->time_since_ground += STEPS_DT;

    // Fade wave trail if not in wave anymore or end animation started or mirror portal animation is happening
    if (player->gamemode != GAMEMODE_WAVE || state.mirroring || player->cutscene_timer > 0) {
        if (wave_trail->opacity > 0) wave_trail->opacity -= 0.02f;
        
        if (wave_trail->opacity <= 0) {
            wave_trail->opacity = 0;
            wave_trail->nuPoints = 0;
        }
    }

    // Weird stuff indeed
    if (!player->velocity_override && !player->dashing) {
		float newVel = player->vel_y + player->gravity * STEPS_DT;

		// Player will fall off blocks a frame faster than expected
		if (!(player->on_ground || player->on_ceiling) && (state.old_player.on_ground || state.old_player.on_ceiling) && ((!curr_input.holdJump && (curr_old_input.pressedJump || curr_input.pressedJump)) || player->buffering_state == BUFFER_READY) && gravBottom(&state.old_player) > gravFloor(&state.old_player) && player->mini == state.old_player.mini) {
			player->y += grav(&state.old_player, state.old_player.gravity) * STEPS_DT * STEPS_DT;

			if (player->vel_y == 0)
				newVel += state.old_player.gravity * STEPS_DT;
		}

        player->vel_y = newVel;
	}

    if (player->cutscene_timer > 0) return;

    player->rotation = normalize_angle(player->rotation);

    player->left_ground = false;

    // If the player hits a gravity changing object, theres a small period of time where the cube doesn't die to ceilings
    if (player->ceiling_inv_time > 0) {
        player->ceiling_inv_time -= STEPS_DT;
    } else {
        player->ceiling_inv_time = 0;
    }

    clamp_player_ground(player);
    
    // Coyote time for slopes, so you can jump if you only touch the slope for a single frame (and fixes some visual bugs)
    if (player->slope_slide_coyote_time) {
        player->slope_slide_coyote_time--;

        if (!player->slope_slide_coyote_time) {
            player->coyote_slope.slope_id = -1;
            player->coyote_slope.elapsed = 0;
            player->coyote_slope.snapDown = false;
        }
    }

    // If player is on slope, do slope stuff
    if (player->slope_data.slope_id >= 0) {
        slope_calc(player->slope_data.slope_id, player);
    }

    // Handle wave trail point adding
    if (player->gamemode == GAMEMODE_WAVE && !state.mirroring) {    
        wave_trail->positionR = (Vec2D){player->x, player->y};  
        wave_trail->startingPositionInitialized = true;
        if (player->vel_y != state.old_player.vel_y || player->on_ground != state.old_player.on_ground || player->on_ceiling != state.old_player.on_ceiling) {
            MotionTrail_AddWavePoint(wave_trail);
        }
    }

    float lerp_speed = player_speed_mults[state.speed] * 0.175f;

    // Handle rotation for ship and wave
    if (!player->dashing) switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            if (player->on_ground || player->slope_data.slope_id >= 0) {
                lerp_speed *= 3.0f;
                player->rotation = RadToDeg(slerp_fancy(DegToRad(player->rotation), DegToRad(player->cube_target_rotation), MIN(STEPS_DT, STEPS_DT * lerp_speed) * 60));
            } else {
                player->rotation = player->cube_target_rotation;
            }
            break;
        case GAMEMODE_SHIP:
            rotate_fly(player, 0.15f);
            break;
        case GAMEMODE_UFO:
            rotate_fly(player, 0.07f);
            break;
        case GAMEMODE_WAVE:
            rotate_fly(player, player->mini ? 0.4f : 0.25f);
            break;
        case GAMEMODE_ROBOT:
        case GAMEMODE_SPIDER:
            float angle_rad = 0;
            if (player->slope_data.slope_id >= 0) {
                angle_rad = slope_snap_angle(player->slope_data.slope_id, player);
            }
            player->rotation = RadToDeg(slerp_fancy(DegToRad(player->rotation), angle_rad, MIN(STEPS_DT, STEPS_DT * lerp_speed) * 60));
            break;       
    }

    player->snap_rotation = false;

    player->buffer_ufo = false;

    if (player->on_ground) player->last_orb_pressed = -1;
}

void handle_player(Player *player) {
    u64 start_player = svcGetSystemTick();

    if (curr_input.holdJump) {    
        if (player->buffering_state == BUFFER_NONE) {
            player->buffering_state = BUFFER_READY;
        }
    } else {
        player->holding_jump_cancelled = false;
        player->buffering_state = BUFFER_NONE;
        stop_dashing(player);
    }
    
    player->on_ground = false;
    player->on_ceiling = false;
    player->landed_from_jump = false;
    
    player->allow_dart_slide = false;
    player->allow_head_hit = false;

    player->velocity_override = false;

    player->gravObj_id = -1;
    player->potentialSlope_id = -1;
    
    player->timeElapsed += STEPS_DT;

    player->vel_x = player_speeds[state.speed]; 
    player->x += player->vel_x * STEPS_DT;
    if (player->dashing) {
        player->vel_y = 0;
        player->y += (player->vel_x * STEPS_DT) * player->dash_slope;
        player->dash_fire_timer += STEPS_DT;
        if (player->gamemode == GAMEMODE_PLAYER || player->gamemode == GAMEMODE_BALL) {
            player->rotation += (player->upside_down ? -1.f : 1.f) * (288.f / 0.3f) * STEPS_DT;
        } else {
            player->rotation = -player->dash_angle;
        }
    } else {
        player->y += player_get_vel(player, player->vel_y) * STEPS_DT;
    }

    clamp_player_ground(player);

    player->just_teleported = false;

    player->frame++;

    // If new vel y is no the max (not set) set vel y
    if (player->new_vel_y != __FLT_MAX__) {
        player->vel_y = player->new_vel_y;
        player->new_vel_y = __FLT_MAX__;
    }

    // Run actions that were queued last frame
    for (int i = 0; i < num_actions[state.current_player]; i++) {
        player_actions[state.current_player][i].func(player);
    }
    num_actions[state.current_player] = 0;

    // The svc calls are for profiling
    u64 start = svcGetSystemTick();
    collide_with_objects(player);
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.collision_ms += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip || state.end_wall_anim_playing) state.dead = false;
    
    if (state.dead) {
        stop_dashing(player);
        return;
    }

    start = svcGetSystemTick();

    if (player->x >= level_info.wall_x - END_ANIMATION_X_START) {
        stop_dashing(player);
        state.end_wall_anim_playing = true;
        p1_trail = true;
        if (player->cutscene_timer == 0) {
            // Add a trail point for wave
            if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(wave_trail);

            player->cutscene_initial_player_x = player->x;
            player->cutscene_initial_player_y = player->y;
        }
        anim_player_to_wall(player);
        player->rotation += easeValue(EASE_IN, 0, 415.3848f, player->cutscene_timer, 0.5f, 2.f) * STEPS_DT;
        player->cutscene_timer += STEPS_DT;
        
        // End level
        if (player->x > level_info.wall_x) {
            level_info.completing = true;
        }
    } 

    run_player(player);

    if (state.dead) stop_dashing(player);

    end = svcGetSystemTick();
    ticks = end - start;
    snapshot.play_ms += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    // Run dual ball being repelled when touching
    do_ball_reflection();

    player->delta_y = player->y - state.old_player.y;
    
    u64 end_player = svcGetSystemTick();
    ticks = end_player - start_player;

    // Add player hitboxes to hitbox trail
    if (state.hitbox_display == 2) add_new_hitbox(player);
    snapshot.handler_ms += ticks / CPU_TICKS_PER_MSEC;
}

void anim_player_to_wall(Player *player) {
    float t = CLAMP(powf(player->cutscene_timer, 1.2f), 0, 1);

    // (1 - t) and powers
    float one_minus_t = 1.0f - t;
    float one_minus_t_squared = one_minus_t * one_minus_t;
    float one_minus_t_cubed = one_minus_t_squared * one_minus_t;

    float t_squared = t * t;
    float t_cubed = t_squared * t;

    // Start point
    float start_x = player->cutscene_initial_player_x;
    float start_y = player->cutscene_initial_player_y;

    // Final;destination
    float final_x = level_info.wall_x + 50.f;
    float final_y = level_info.wall_y - 20.f;

    // Mid point
    float mid_x = start_x + 40.f;
    float mid_y = level_info.wall_y + 150.f;

    // Cubic Bezier interpolation
    player->x = 
        one_minus_t_cubed * start_x + 
        one_minus_t_squared * 3 * t * start_x +
        one_minus_t * 3 * t_squared * mid_x + t_cubed * final_x;

    player->y = 
        one_minus_t_cubed * start_y + 
        one_minus_t_squared * 3 * t * start_y +
        one_minus_t * 3 * t_squared * mid_y + t_cubed * final_y;
}


void spawn_p1_trail(Player *player, int player_id) {
    P1Trail *trail_data = &state.p1_trail_data[player_id][state.p1_trail_pos[player_id]];
    
    float scale = (player->mini) ? 0.6f : 1.f;

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
        case GAMEMODE_BALL:
        case GAMEMODE_WAVE:
            trail_data->gamemode = player->gamemode;
            trail_data->scale = scale;
            trail_data->upside_down = false;
            break;
        case GAMEMODE_SHIP:
        case GAMEMODE_UFO:
            trail_data->gamemode = GAMEMODE_PLAYER;
            trail_data->scale = scale * 0.5f;
            trail_data->upside_down = player->upside_down;
            break;
        case GAMEMODE_ROBOT:
            return;
        case GAMEMODE_SPIDER:
            return;
    }

    float end_scale = trail_data->scale * P1_TRAIL_END_SCALE;

    trail_data->delta_scale = (end_scale - trail_data->scale) / P1_TRAIL_DURATION;
    
    trail_data->x = player->x;
    trail_data->y = player->y;
    trail_data->rot = player->rotation;
    trail_data->opacity = 0.7f;
    trail_data->life = P1_TRAIL_DURATION;

    trail_data->active = true;

    state.p1_trail_pos[player_id]++;
    if (state.p1_trail_pos[player_id] >= P1_TRAIL_LENGTH) {
        state.p1_trail_pos[player_id] = 0;
    }
}

#define P1_TRAIL_RATE 0.05f

void update_p1_trail(Player *player, int player_id) {
    // Spawn new p1 icon 
    if (!state.dead && p1_trail && p1_trail_timer <= 0) {
        p1_trail_timer = P1_TRAIL_RATE;
        // If current player is player 2, dual must be true
        if (player_id == 0 || state.dual) {
            spawn_p1_trail(player, player_id);
        }
    }

    p1_trail_timer -= delta;

    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trail_data = &state.p1_trail_data[player_id][i];

        if (trail_data->active) {
            trail_data->opacity -= (0.8f / P1_TRAIL_DURATION) * delta;
            trail_data->scale += trail_data->delta_scale * delta;

            trail_data->life -= delta;

            if (trail_data->life <= 0) {
                trail_data->active = false;
            }
        }
    }
}

void draw_p1_trail(Player *player, int player_id) {
    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trail_data = &state.p1_trail_data[player_id][i];

        if (trail_data->active) {
            float calc_x = ((trail_data->x - state.camera_x));
            float calc_y = SCREEN_HEIGHT - ((trail_data->y - state.camera_y));
            bool flip_x = (state.mirror_mult < 0);

            u32 color = (player_id == 1) ? C2D_Color32(p2_color.r, p2_color.g, p2_color.b, trail_data->opacity * 255) : C2D_Color32(p1_color.r, p1_color.g, p1_color.b, trail_data->opacity * 255);

            spawn_p1_layer_at(
                trail_data->gamemode, ((settingsState.defaultMiniIcon && player->mini && (trail_data->gamemode == 0 || trail_data->gamemode == 2)) ? 0 : *current_icons[trail_data->gamemode]), 
                get_mirror_x(calc_x, state.mirror_factor), calc_y, 
                trail_data->rot,  
                flip_x, trail_data->upside_down,
                trail_data->scale,
                color,
                (IconParameters) { 
                    .anim_id = player->anim_id,
                    .anim_frame = player->anim_frame,
                    .anim_timer = player->anim_timer,
                    .blend_from_anim_id = player->blend_from_anim_id,
                    .blend_from_frame = player->blend_from_frame,
                    .blend_from_timer = player->blend_from_timer,
                    .blend_fraction = player->blend_timer / get_blend_duration(player)
                 }
            );
        }
    }
}


void draw_player(Player *player) {
    // Don't draw player if dead
    if (state.dead) return;

    // Don't draw player if respawning and should hide
    if (state.respawn_effect_data.active && state.respawn_effect_data.hide_player) return;

    float calc_x = ((player->x - state.camera_x));
    float calc_y = SCREEN_HEIGHT - ((player->y - state.camera_y));

    Color p1_color = player->player_icons.p1_color;
    Color p2_color = player->player_icons.p2_color;
    Color glow_color = player->player_icons.glow_color;

    if (player->flash_active) {
        float flash_t = player->flash_timer - player->flash_delay;
        float flash_k = (flash_t <= 0.f) ? 0.f : CLAMP(flash_t / player->flash_duration, 0.f, 1.f);

        p1_color.r = (unsigned char)(255.f + ((float)p1_color.r - 255.f) * flash_k);
        p1_color.g = (unsigned char)(255.f + ((float)p1_color.g - 255.f) * flash_k);
        p1_color.b = (unsigned char)(255.f + ((float)p1_color.b - 255.f) * flash_k);

        p2_color.r = (unsigned char)(255.f + ((float)p2_color.r - 255.f) * flash_k);
        p2_color.g = (unsigned char)(255.f + ((float)p2_color.g - 255.f) * flash_k);
        p2_color.b = (unsigned char)(255.f + ((float)p2_color.b - 255.f) * flash_k);
    }

    u32 primary_color = C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255);
    u32 secondary_color = C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255);

    if (state.current_player == 1) {
        u32 tmp = primary_color;
        primary_color = secondary_color;
        secondary_color = tmp;
    }

    float scale = (player->mini) ? 0.6f : 1.f;

    if (player->gamemode == GAMEMODE_SPIDER && player->spider_scale_active) {
        float scale_t = CLAMP(player->spider_scale_timer / SPIDER_DASH_SCALE_DURATION, 0.f, 1.f);
        scale *= SPIDER_DASH_SCALE_START + (1.f - SPIDER_DASH_SCALE_START) * elasticEaseOut(scale_t, SPIDER_DASH_ELASTIC_PERIOD);
    }

    // All of this is just for calculating the cube position in ufo and ship
    float rad = C3D_AngleFromDegrees(player->rotation);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    bool flip_x = (state.mirror_mult < 0);
    int flip_y_mult = (player->upside_down ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    const float local_x = 0;
    const float local_y = ((player->gamemode == GAMEMODE_SHIP) ? 6 : 4) * flip_y_mult;

    float rot_x = local_x * m00 + local_y * m01;
    float rot_y = local_x * m10 + local_y * m11;

    float p_x = get_mirror_x(calc_x + rot_x * scale, state.mirror_factor);
    float p_y = calc_y + rot_y * scale;

    float calc_x_mirror = get_mirror_x(calc_x, state.mirror_factor);
    float p_rot = player->rotation;

    int selected_cube = player->player_icons.cube;
    int selected_ship = player->player_icons.ship;
    int selected_ball = player->player_icons.ball;
    int selected_ufo =  player->player_icons.ufo;
    int selected_wave = player->player_icons.wave;
    int selected_robot = player->player_icons.robot;
    int selected_spider = player->player_icons.spider;

    bool glow_enabled = (player->player_icons.glow || ((player->player_icons.p1_color.r | player->player_icons.p1_color.g | player->player_icons.p1_color.b) == 0));

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            spawn_icon_at(GAMEMODE_PLAYER, (settingsState.defaultMiniIcon && player->mini) ? 0 : selected_cube, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case GAMEMODE_SHIP:
            if (glow_enabled) {
                spawn_glow_layer_at(GAMEMODE_SHIP, selected_ship, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                    C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255), 
                    (IconParameters) { 0 }
                );
            }
            spawn_icon_at(GAMEMODE_PLAYER, (settingsState.defaultMiniIcon && player->mini) ? 0 : selected_cube, glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 0 }
            );
            spawn_icon_at(GAMEMODE_SHIP, selected_ship, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                0,
                (IconParameters) { 0 }
            );
            break;
        case GAMEMODE_BALL:
            spawn_icon_at(GAMEMODE_BALL, (settingsState.defaultMiniIcon && player->mini) ? 0 : selected_ball, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case GAMEMODE_UFO:
            if (glow_enabled) {
                spawn_glow_layer_at(GAMEMODE_UFO, selected_ufo, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale,
                    C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                    (IconParameters) { 0 }
            );
            }
            spawn_icon_at(GAMEMODE_PLAYER, (settingsState.defaultMiniIcon && player->mini) ? 0 : selected_cube, glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 0 }
            );
            spawn_icon_at(GAMEMODE_UFO, selected_ufo, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                0,
                (IconParameters) { 0 }
            );
            break;    
        case GAMEMODE_WAVE:
            spawn_icon_at(GAMEMODE_WAVE, selected_wave, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 0 }
            );
            break;
        case GAMEMODE_ROBOT: {
            spawn_icon_at(GAMEMODE_ROBOT, selected_robot, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale,
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) { 
                    .anim_id = player->anim_id,
                    .anim_frame = player->anim_frame,
                    .anim_timer = player->anim_timer,
                    .blend_from_anim_id = player->blend_from_anim_id,
                    .blend_from_frame = player->blend_from_frame,
                    .blend_from_timer = player->blend_from_timer,
                    .blend_fraction = player->blend_timer / get_blend_duration(player),
                    .robot_fire_scale = player->robot_fire_visible ? player->robot_fire_scale : 0.f
                 }
            );
            break;
        }
        case GAMEMODE_SPIDER: {
            spawn_icon_at(GAMEMODE_SPIDER, selected_spider, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale,
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255),
                (IconParameters) {
                    .anim_id = player->anim_id,
                    .anim_frame = player->anim_frame,
                    .anim_timer = player->anim_timer,
                    .blend_from_anim_id = player->blend_from_anim_id,
                    .blend_from_frame = player->blend_from_frame,
                    .blend_from_timer = player->blend_from_timer,
                    .blend_fraction = player->blend_timer / get_blend_duration(player)
                 }
            );
            break;
        }
    }
}

extern void get_corners(float cx, float cy, float w, float h, float angle, Vec2D out[4]);


void draw_triangle_from_rect(Vec2D rect[4], int skip_index, uint32_t color, uint32_t hypo_color, float depth) {
    Vec2D tri[3];
    u32 col[3];
    int ti = 0;

    int hypo_id = skip_index - 1;
    if (hypo_id < 0) hypo_id = 3;

    // Collect 3 points that are not the skipped one
    for (int i = 0; i < 4; ++i) {
        if (i == skip_index) continue;
        tri[ti].x = mirror_x_on_screen(rect[i].x);
        tri[ti].y = calc_y_on_screen(rect[i].y);
        
        if (i == hypo_id) col[ti++] = hypo_color;
        else col[ti++] = color;
    }

    draw_polygon_inward_mitered(tri, 3, 2.f, col, depth);
}

void draw_square(Vec2D rect[4], uint32_t color, float depth) {
    Vec2D center = {
        (rect[0].x + rect[1].x + rect[2].x + rect[3].x) / 4.f,
        (rect[0].y + rect[1].y + rect[2].y + rect[3].y) / 4.f
    };

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;

        draw_hitbox_line_inward(rect,
            mirror_x_on_screen(rect[i].x), calc_y_on_screen(rect[i].y),
            mirror_x_on_screen(rect[j].x), calc_y_on_screen(rect[j].y),
            1.5f, center.x, center.y, color, depth
        );
    }
}

static u32 get_id_color(int id, float *out_depth) {
    switch (id) {
        case J_MODIFIERBLOCK:
        case D_MODIFIERBLOCK:
        case S_MODIFIERBLOCK:
        case H_MODIFIERBLOCK:
            *out_depth = 0.75f;
            return C2D_Color32(0xff, 0xff, 0x00, 0xff);
    }
    return 0;
}

void draw_hitbox(int obj) {
    int id = objects.id[obj];
    if (!is_valid_object(id)) return;

    Vec2D rect[4];
    if (is_trigger_object(id) && objects.touch_triggered[obj]) {
        get_corners(objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], objects.rotation[obj], rect);
        draw_square(rect, C2D_Color32(0x00, 0xff, 0xff, 0xff), 3.f);
        return;
    }

    const ObjectHitbox *hitbox = game_objects[id].hitbox;

    if (!hitbox) return;

    float depth = 0.f;

    float angle = -objects.rotation[obj];
    float rot_rad = C3D_AngleFromDegrees(adjust_angle_y(angle, objects.flippedH[obj]));
    float cos_r = cosf(rot_rad), sin_r = sinf(rot_rad);

    float raw_off_x = hitbox->x * objects.scale_x[obj];
    float raw_off_y = hitbox->y * objects.scale_y[obj];

    float off_x = raw_off_x * cos_r - raw_off_y * sin_r;
    float off_y = raw_off_x * sin_r + raw_off_y * cos_r;

    float x = objects.x[obj] + off_x;
    float y = objects.y[obj] + off_y;
    float w = objects.width[obj];
    float h = objects.height[obj];

    float rot = objects.rotation[obj];

    if (hitbox->collision_type == HITBOX_SOLID) {
        rot = 0;
    }
    unsigned int color = C2D_Color32(0x00, 0xff, 0xff, 0xff);

    int hitbox_type = hitbox->collision_type;
    if (hitbox_type == HITBOX_HAZARD) {
        color = C2D_Color32(0xff, 0x00, 0x00, 0xff);
        depth = 0.5f;
    }
    if (hitbox_type == HITBOX_SOLID) {
        color = C2D_Color32(0x00, 0x00, 0xff, 0xff);
        depth = 0.f;
    }
    if (hitbox_type == HITBOX_SPECIAL) {
        color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
        depth = 1.f;
    }

    u32 id_color = get_id_color(id, &depth);
    if (id_color) color = id_color;

    // Current slope and x snapped obj gets its own colors
    if (obj == state.player.slope_data.slope_id || obj == state.player2.slope_data.slope_id) color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
    if (obj == state.player.snap_data.snapped_obj || obj == state.player2.snap_data.snapped_obj) color = C2D_Color32(0xff, 0xff, 0x00, 0xff);

    if (hitbox->type == COLLISION_SLOPE) {
        w = objects.width[obj];
        h = objects.height[obj];
        get_corners(x, y, w, h, 0, rect);

        depth = 0.f;

        if (is_spike_slope(obj)) {
            depth += 0.5f;
            draw_triangle_from_rect(rect, 3 - objects.orientation[obj], color, C2D_Color32(0xff, 0x00, 0x00, 0xff), depth);
        } else {
            draw_triangle_from_rect(rect, 3 - objects.orientation[obj], color, color, depth);
        }
    } else if (hitbox->type == COLLISION_CIRCLE) {
        float calc_radius = objects.width[obj];

        custom_circunference(mirror_x_on_screen(x), calc_y_on_screen(y), calc_radius, color, 2.f);
    } else if (w != 0 && h != 0) {
        get_corners(x, y, w, h, rot, rect);
        draw_square(rect, color, depth);
    }
}

void draw_player_hitbox(Player *player) {
    Vec2D rect[4];

    // Unrotated hitbox
    get_corners(player->x, player->y, player->width, player->height, 0, rect);

    draw_square(rect, C2D_Color32(0xff, 0x00, 0x00, 0xff), 0.f);
}

void draw_internal_hitbox(Player *player) {
    InternalHitbox internal = player->internal_hitbox;
    Vec2D rect[4];
    // Internal hitbox
    get_corners(player->x, player->y, internal.width, internal.height, 0, rect);

    draw_square(rect, C2D_Color32(0x00, 0x00, 0xff, 0xff), 0.f);
}

void draw_rotated_hitbox(Player *player) {
    Vec2D rect[4];
    // Rotated hitbox
    get_corners(player->x, player->y, player->width, player->height, player->rotation, rect);

    draw_square(rect, C2D_Color32(0x7f, 0x00, 0x00, 0xff), 0.f);
}


void add_new_hitbox(Player *player) {
    for (int i = HITBOX_TRAIL_SIZE - 2; i > 0; i--) {
        state.hitbox_trail_players[state.current_player][i] = state.hitbox_trail_players[state.current_player][i - 1];
    }
    PlayerHitboxTrail hitbox;
    hitbox.x = player->x;
    hitbox.y = player->y;
    hitbox.width = player->width;
    hitbox.height = player->height;
    hitbox.rotation = player->rotation;
    hitbox.internal_hitbox = player->internal_hitbox;

    state.hitbox_trail_players[state.current_player][0] = hitbox;

    state.last_hitbox_trail++;

    if (state.last_hitbox_trail >= HITBOX_TRAIL_SIZE) state.last_hitbox_trail = HITBOX_TRAIL_SIZE - 1;
}

void draw_hitbox_trail(int player) {
    // Draw normal hitbox
    for (int i = state.last_hitbox_trail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitbox_trail_players[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.rotation = hitbox.rotation;

        draw_player_hitbox(&player);
    }
    // Draw internal
    for (int i = state.last_hitbox_trail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitbox_trail_players[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.internal_hitbox = hitbox.internal_hitbox;
        player.rotation = hitbox.rotation;

        draw_internal_hitbox(&player);
    }
}

void push_player_action(void (*func)(Player *)) {
    int count = num_actions[state.current_player];
    if (count < MAX_ACTIONS) {
        player_actions[state.current_player][num_actions[state.current_player]++].func = func;
    }
}