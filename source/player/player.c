#include "player.h"
#include "state.h"
#include "icons.h"
#include "graphics.h"

#include "particles/particles.h"

#include "slope.h"

#include "menus/icon_kit.h"
#include "collision.h"
#include "math_helpers.h"

#include "main.h"

#include "mp3_player.h"
#include "menus/settings.h"
#include "utils/gfx.h"

#include "easing.h"

inline float gravFloor(Player *player) { return player->upside_down ? -state.ceiling_y : state.ground_y; }

void anim_player_to_wall(Player *player);

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
ParticleSystem land_particles[2];
ParticleSystem explosion_particles[2];
ParticleSystem glitter_particles;
ParticleSystem slow_speed_particles;
ParticleSystem normal_speed_particles;
ParticleSystem fast_speed_particles;
ParticleSystem faster_speed_particles;
ParticleSystem coin_pickup_particles;

PlayerAction player_actions[2][MAX_ACTIONS];
int num_actions[2] = {0};

int frame_skipped = 0;


const float velocity_thresholds[SPEED_COUNT] = {
	101.541492,
	103.485494592,
	103.377492,
	103.809492
};

const float player_speeds[SPEED_COUNT] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566
};

const float cube_jump_heights[SPEED_COUNT] = {
    573.481728,
    603.7217172,
    616.681728,
    606.421728,
};

const float cube_accelerations[] = {
    -2747.52,
    -2794.1082,
    -2786.4,
    -2799.36,
};

const float player_speed_mults[SPEED_COUNT] = {
	0.7f,
	0.9f,
	1.1f,
	1.3f	
};

const float cube_rotation_speed[2] = {
    415.3848f,
    540.f
};

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

    // Limit player speed (there is a upwards cap interestingly)
    if (player->vel_y < -810) player->vel_y = -810;
    if (player->vel_y > 1080) player->vel_y = 1080;

    // If player fell above the level, die
    if (player->y > 2794.f) kill_player(DEATH_FELL_OFF_LEVEL);

    if (player->snap_rotation) {
        player->cube_target_rotation = player->rotation;
    }

    // Do cube rotation
    if (player->slope_data.slope_id < 0 && !player->on_ground) {
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

    if (state.input.holdJump) {
        jump = true;
    } else if (player->on_ground) {
        player->consecutive_jumps = 0;
    }

    if (player->on_ground) {
        MotionTrail_StopStroke(trail);
        update_rotation_direction(player);
    }

    bool should_coyote = (state.dual || player->upside_down) && state.input.holdJump && player->coyote_frames < 10;

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
                set_p_velocity(player, cube_jump_heights[state.speed], state.old_input.holdJump);
            }
        } else {
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed], state.old_input.holdJump);
        }
        player->inverse_rotation = false;
        player->buffering_state = BUFFER_END;
    
        player->on_ground = false;
        player->consecutive_jumps++;
        
        state.current_data.jumps++;
    
        if (!(state.input.pressedJump)) {
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
        if (player->gamemode == GAMEMODE_BIRD) {
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
    ship_fire_particles[state.current_player].emitting = state.input.holdJump;

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



    if (state.dual) {
        // Make both dual players symmetric by using inverted ship gravity
        if (state.input.holdJump) {
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
        if (state.input.holdJump) {
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
    
    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->gravity < 0 && player->vel_y < min) {
        player->vel_y = min;
    } else if (player->gravity > 0 && player->vel_y > max) {
        player->vel_y = max;
    }
}


static float ballJumpHeights[SPEED_COUNT] = {
    -172.044007,
    -181.11601,
    -185.00401,
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

    // If on ground (block or slope) and its buffering, do a jump
    if ((player->slope_data.slope_id >= 0 || player->on_ground || player->on_ceiling) && player->buffering_state == BUFFER_READY) {        
        player->upside_down ^= 1;

        set_p_velocity(player, ballJumpHeights[state.speed], state.old_player.buffering_state == BUFFER_READY);

        if (player->slope_data.slope_id >= 0 && grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_NORMAL_UP)
            player->vel_y -= player->gravity * STEPS_DT;

        player->buffering_state = BUFFER_END;
        
        state.current_data.jumps++;
        
        player->ball_rotation_speed = -BALL_SLOW_ROTATION;

        player->on_ground = false;
    }
    
    player->rotation += player->ball_rotation_speed * get_ball_rotation_speed(player) * mult * STEPS_DT;

    if (player->vel_y < -810) {
        player->vel_y = -810;
    } else if (player->vel_y > 810) {
        player->vel_y = 810;
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

    bool buffering_check = ((state.old_player.gamemode == GAMEMODE_PLAYER || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_DART) && (state.input.holdJump));
    // If buffering, jump
    if (player->buffering_state == BUFFER_READY && (state.input.pressedJump || buffering_check)) {
        player->vel_y = fmaxf(player->vel_y, player->mini ? 358.992 : 371.034);
        player->buffering_state = BUFFER_END;
        player->velocity_override = true;
        player->burst_particle_timer = BURST_PARTICLES_DURATION;
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

    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->vel_y < min) {
        player->vel_y = min;
    } else if (player->vel_y > max) {
        player->vel_y = max;
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

    bool input = (state.input.holdJump);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
}

void clamp_player_ground(Player *player) {
    bool slopeCheck = player->slope_data.slope_id >= 0 && (grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_UD_DOWN);

    // Check for ground collision
    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gravObj_id < 0 && player->gamemode == GAMEMODE_PLAYER && player->upside_down) {
            kill_player(DEATH_CEILING);
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) <= 0) set_p_velocity(player, 0, player->gamemode == GAMEMODE_PLAYER_BALL);
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
        player->snap_data.player_frame = 0;
    }

    // Check for ceiling collision
    if (getGroundTop(player) > state.ceiling_y) {
        if (player->ceiling_inv_time <= 0  && player->gravObj_id < 0 && player->gamemode == GAMEMODE_PLAYER && !player->upside_down) {
            kill_player(DEATH_CEILING);
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) >= 0) set_p_velocity(player, 0, player->gamemode == GAMEMODE_PLAYER_BALL);
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);
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
            }
            player->time_since_ground = 0; 
        } 

        // Ceiling
        if (getGroundTop(player) >= state.ceiling_y) {
            if (player->upside_down) {
                player->on_ground = true;
                player->inverse_rotation = false;
            } else {
                player->on_ceiling = true;     
                player->inverse_rotation = false;     
            } 
            player->time_since_ground = 0; 
        } 
    }
    
    // Handle land particles
    if (player->gamemode != GAMEMODE_DART && !state.old_player.on_ground && player->on_ground) {
        land_particles[state.current_player].emitterX = player->x;
        land_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
        land_particles[state.current_player].gravityFlipped = player->upside_down;
        land_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);
        spawnMultipleParticles(&land_particles[state.current_player], 10);
    }

    // Coyote time (only applies to upside down gravity)
    if (gravBottom(&state.old_player) > gravFloor(&state.old_player) && player->upside_down == state.old_player.upside_down && !player->on_ground && player->vel_y <= 0) {
		if (state.old_player.on_ground && !state.old_input.holdJump)
			player->coyote_frames = 0;
		player->coyote_frames++;
	} else {
		player->coyote_frames = INT32_MAX;
	}

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            cube_gamemode(player);
            break;
        case GAMEMODE_SHIP:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring) MotionTrail_ResumeStroke(trail);
            ship_gamemode(player);
            break;
        case GAMEMODE_PLAYER_BALL:
            ball_gamemode(player);
            break;
        case GAMEMODE_BIRD:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring) MotionTrail_ResumeStroke(trail);
            ufo_gamemode(player);
            break;
        case GAMEMODE_DART:
            glitter_particles.emitterX = state.camera_x_middle;
            glitter_particles.emitterY = state.camera_y_middle;
            glitter_particles.emitting = true;
            if (!state.mirroring) {
                if (noWaveTrailBehind) {
                    MotionTrail_StopStroke(trail);
                } else {
                    MotionTrail_ResumeStroke(trail);
                }
            }
            wave_gamemode(player);
            break;
    } 
    
    player->time_since_ground += STEPS_DT;

    // Fade wave trail if not in wave anymore or end animation started or mirror portal animation is happening
    if (player->gamemode != GAMEMODE_DART || state.mirroring || player->cutscene_timer > 0) {
        if (wave_trail->opacity > 0) wave_trail->opacity -= 0.02f;
        
        if (wave_trail->opacity <= 0) {
            wave_trail->opacity = 0;
            wave_trail->nuPoints = 0;
        }
    }

    // Weird stuff indeed
    if (!player->velocity_override) {
		float newVel = player->vel_y + player->gravity * STEPS_DT;

		// Player will fall off blocks a frame faster than expected
		if (!(player->on_ground || player->on_ceiling) && (state.old_player.on_ground || state.old_player.on_ceiling) && ((!state.input.holdJump && (state.old_input.pressedJump || state.input.pressedJump)) || player->buffering_state == BUFFER_READY) && gravBottom(&state.old_player) > gravFloor(&state.old_player) && player->mini == state.old_player.mini) {
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
    if (player->gamemode == GAMEMODE_DART && !state.mirroring) {    
        wave_trail->positionR = (Vec2D){player->x, player->y};  
        wave_trail->startingPositionInitialized = true;
        if (player->vel_y != state.old_player.vel_y || player->on_ground != state.old_player.on_ground || player->on_ceiling != state.old_player.on_ceiling) {
            MotionTrail_AddWavePoint(wave_trail);
        }
    }

    // Handle rotation for ship and wave
    if (player->gamemode == GAMEMODE_PLAYER) {
        float lerp_speed = player_speed_mults[state.speed] * 0.175f;

        if (player->on_ground || player->slope_data.slope_id >= 0) {
            lerp_speed *= 3.0f;
            player->rotation = RadToDeg(slerp_fancy(DegToRad(player->rotation), DegToRad(player->cube_target_rotation), MIN(STEPS_DT, STEPS_DT * lerp_speed) * 60));
        } else {
            player->rotation = player->cube_target_rotation;
        }
    }
    if (player->gamemode == GAMEMODE_SHIP) rotate_fly(player, 0.15f);
    if (player->gamemode == GAMEMODE_DART) rotate_fly(player, player->mini ? 0.4f : 0.25f);
    if (player->gamemode == GAMEMODE_BIRD) rotate_fly(player, 0.07f);

    player->snap_rotation = false;
}

float collision_time = 0;
float player_time = 0;
float handle_player_time = 0;

void handle_player(Player *player) {
    if (get_fade_status()) return;

    u64 start_player = svcGetSystemTick();
    if (state.input.holdJump) {
        if (player->buffering_state == BUFFER_NONE) {
            player->buffering_state = BUFFER_READY;
        }
    } else {
        player->buffering_state = BUFFER_NONE;
    }
    
    player->on_ground = false;
    player->on_ceiling = false;

    player->velocity_override = false;

    player->gravObj_id = -1;
    player->potentialSlope_id = -1;
    
    player->timeElapsed += STEPS_DT;

    player->vel_x = player_speeds[state.speed]; 
    player->x += player->vel_x * STEPS_DT;
    player->y += player_get_vel(player, player->vel_y) * STEPS_DT;

    clamp_player_ground(player);

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
    collision_time += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    if (state.dead) return;

    start = svcGetSystemTick();

    if (player->x >= level_info.wall_x - END_ANIMATION_X_START) {
        state.end_wall_anim_playing = true;
        p1_trail = true;
        if (player->cutscene_timer == 0) {
            // Add a trail point for wave
            if (player->gamemode == GAMEMODE_DART) MotionTrail_AddWavePoint(wave_trail);

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
    end = svcGetSystemTick();
    ticks = end - start;
    player_time += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    // Run dual ball being repelled when touching
    do_ball_reflection();

    player->delta_y = player->y - state.old_player.y;
    
    u64 end_player = svcGetSystemTick();
    ticks = end_player - start_player;

    // Add player hitboxes to hitbox trail
    if (state.hitbox_display == 2) add_new_hitbox(player);
    handle_player_time += ticks / CPU_TICKS_PER_MSEC;
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
        case GAMEMODE_PLAYER_BALL:
        case GAMEMODE_DART:
            trail_data->gamemode = player->gamemode;
            trail_data->scale = scale;
            trail_data->upside_down = false;
            break;
        case GAMEMODE_SHIP:
        case GAMEMODE_BIRD:
            trail_data->gamemode = GAMEMODE_PLAYER;
            trail_data->scale = scale * 0.5f;
            trail_data->upside_down = player->upside_down;
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

void update_p1_trail(Player *player, int player_id) {
    // Spawn new p1 icon every 3 frames (the division hurts)
    if (!state.dead && p1_trail && (frame_counter % 3) == 0) {
        // If current player is player 2, dual must be true
        if (player_id == 0 || state.dual) {
            spawn_p1_trail(player, player_id);
        }
    }

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
                trail_data->gamemode, *current_icons[trail_data->gamemode], 
                get_mirror_x(calc_x, state.mirror_factor), calc_y, 
                trail_data->rot,  
                flip_x, trail_data->upside_down,
                trail_data->scale,
                color
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

    u32 primary_color = C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255);
    u32 secondary_color = C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255);

    if (state.current_player == 1) {
        u32 tmp = primary_color;
        primary_color = secondary_color;
        secondary_color = tmp;
    }

    float scale = (player->mini) ? 0.6f : 1.f;

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
    float p_rot = player->rotation * state.mirror_mult;

    int selected_cube = player->player_icons.cube;
    int selected_ship = player->player_icons.ship;
    int selected_ball = player->player_icons.ball;
    int selected_ufo =  player->player_icons.ufo;
    int selected_wave = player->player_icons.wave;

    bool glow_enabled = (player->player_icons.glow || ((p1_color.r | p1_color.g | p1_color.b) == 0));

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_SHIP:
            if (glow_enabled) spawn_glow_layer_at(GAMEMODE_SHIP, selected_ship, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255));
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            spawn_icon_at(GAMEMODE_SHIP, selected_ship, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                0
            );
            break;
        case GAMEMODE_PLAYER_BALL:
            spawn_icon_at(GAMEMODE_PLAYER_BALL, selected_ball, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_BIRD:
            if (glow_enabled) spawn_glow_layer_at(GAMEMODE_BIRD, selected_ufo, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255));
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            spawn_icon_at(GAMEMODE_BIRD, selected_ufo, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                0
            );
            break;    
        case GAMEMODE_DART:
            spawn_icon_at(GAMEMODE_DART, selected_wave, glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
    }
}

extern void get_corners(float cx, float cy, float w, float h, float angle, Vec2D out[4]);


void draw_triangle_from_rect(Vec2D rect[4], int skip_index, uint32_t color) {
    Vec2D tri[3];
    int ti = 0;

    // Collect 3 points that are not the skipped one
    for (int i = 0; i < 4; ++i) {
        if (i == skip_index) continue;
        tri[ti].x = mirror_x_on_screen(rect[i].x);
        tri[ti++].y = calc_y_on_screen(rect[i].y);
    }

    draw_polygon_inward_mitered(tri, 3, 2.f, color);
}

void draw_square(Vec2D rect[4], uint32_t color) {
    Vec2D center = {
        (rect[0].x + rect[1].x + rect[2].x + rect[3].x) / 4.f,
        (rect[0].y + rect[1].y + rect[2].y + rect[3].y) / 4.f
    };

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;

        draw_hitbox_line_inward(rect,
            mirror_x_on_screen(rect[i].x), calc_y_on_screen(rect[i].y),
            mirror_x_on_screen(rect[j].x), calc_y_on_screen(rect[j].y),
            1.5f, center.x, center.y, color
        );
    }
}

void draw_hitbox(int obj) {
    if (!is_valid_object(objects.id[obj])) return;

    const ObjectHitbox *hitbox = game_objects[objects.id[obj]].hitbox;

    if (!hitbox) return;

    float angle = objects.rotation[obj];

    float x = objects.x[obj];
    float y = objects.y[obj];
    float w = hitbox->width;
    float h = hitbox->height;

    unsigned int color = C2D_Color32(0x00, 0xff, 0xff, 0xff);

    int hitbox_type = hitbox->collision_type;
    if (hitbox_type == HITBOX_HAZARD) color = C2D_Color32(0xff, 0x00, 0x00, 0xff);
    if (hitbox_type == HITBOX_SOLID) color = C2D_Color32(0x00, 0x00, 0xff, 0xff);
    if (hitbox_type == HITBOX_SPECIAL) color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
    
    // Current slope and x snapped obj gets its own colors
    if (obj == state.player.slope_data.slope_id || obj == state.player2.slope_data.slope_id) color = C2D_Color32(0x00, 0xff, 0x00, 0xff);
    if (obj == state.player.snap_data.snapped_obj || obj == state.player2.snap_data.snapped_obj) color = C2D_Color32(0xff, 0xff, 0x00, 0xff);

    Vec2D rect[4];
    if (hitbox->type == COLLISION_SLOPE) {
        w = objects.width[obj];
        h = objects.height[obj];
        get_corners(x, y, w, h, 0, rect);

        draw_triangle_from_rect(rect, 3 - objects.orientation[obj], color);
    } else if (hitbox->type == COLLISION_CIRCLE) {
        float calc_radius = hitbox->width;

        custom_circunference(mirror_x_on_screen(x), calc_y_on_screen(y), calc_radius, color, 2.f);
    } else if (w != 0 && h != 0) {
        get_corners(x, y, w, h, angle, rect);
        draw_square(rect, color);
    }
}

void draw_player_hitbox(Player *player) {
    InternalHitbox internal = player->internal_hitbox;
    Vec2D rect[4];
    // Rotated hitbox
    get_corners(player->x, player->y, player->width, player->height, player->rotation, rect);

    draw_square(rect, C2D_Color32(0x7f, 0x00, 0x00, 0xff));

    // Internal hitbox
    get_corners(player->x, player->y, internal.width, internal.height, 0, rect);

    draw_square(rect, C2D_Color32(0x00, 0x00, 0xff, 0xff));

    // Unrotated hitbox
    get_corners(player->x, player->y, player->width, player->height, 0, rect);

    draw_square(rect, C2D_Color32(0xff, 0x00, 0x00, 0xff));
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
    for (int i = state.last_hitbox_trail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitbox_trail_players[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.internal_hitbox = hitbox.internal_hitbox;
        player.rotation = hitbox.rotation;

        draw_player_hitbox(&player);
    }
}

void push_player_action(void (*func)(Player *)) {
    int count = num_actions[state.current_player];
    if (count < MAX_ACTIONS) {
        player_actions[state.current_player][num_actions[state.current_player]++].func = func;
    }
}