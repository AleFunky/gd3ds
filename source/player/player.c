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

#include "utils/gfx.h"

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


int frame_skipped = 0;

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

const float slopeHeights[SPEED_COUNT] = {
    322.345224,
    399.889818,
    497.224926,
    600.643296
};

float player_get_vel(Player *player, float vel) {
    return vel * (player->upside_down ? -1 : 1);
}

void set_p_velocity(Player *player, float vel) {
    player->vel_y = vel * ((player->mini) ? 0.8 : 1);
}

void update_rotation_direction(Player *player) {
    player->rotation_direction = (player->upside_down ? -1 : 1);
}

void cube_gamemode(Player *player) {
    int mult = player->rotation_direction;
    
    trail->positionR = (Vec2){player->x, player->y};  
    trail->startingPositionInitialized = true;

    player->gravity = cube_accelerations[state.speed];

    if (player->vel_y < -810) player->vel_y = -810;
    if (player->vel_y > 1080) player->vel_y = 1080;

    if (player->y > 2794.f) state.dead = true;

    if (player->inverse_rotation) {
        player->rotation -= (415.3848f / 2) * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    } else {
        player->rotation += 415.3848f * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    }

    drag_particles[state.current_player].emitterX = getLeft(player);
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
    drag_particles[state.current_player].emitting = player->time_since_ground < 0.1f;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);
    
    
    if (player->on_ground) {
        MotionTrail_StopStroke(trail);
        update_rotation_direction(player);
        if (player->slope_data.slope_id < 0) player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }

    SlopeData slope_data = player->slope_data;

    // If not currently on slope, look at the last frame
    if (player->slope_data.slope_id < 0 && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }

    if ((slope_data.slope_id >= 0 || player->on_ground) && (state.input.holdJump)) {
        if (slope_data.slope_id >= 0) {
            // Slope jump
            int orient = grav_slope_orient(slope_data.slope_id, player);
            if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
                float time = clampf(10 * (player->timeElapsed - slope_data.elapsed), 0.4f, 1.0f);
                set_p_velocity(player, 0.25f * time * slopeHeights[state.speed] + cube_jump_heights[state.speed]);
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed]);
            }
        } else {
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed]);
        }
        player->inverse_rotation = false;
    
        player->on_ground = false;
        player->buffering_state = BUFFER_END;
    
        if (!(state.input.pressedJump)) {
            // This simulates the holding jump
            player->vel_y -= player->gravity * STEPS_DT;
        }
    }
}

void rotate_fly(Player *player, float mult) {
    float diff_x = (player->x - state.old_player.x);
    float diff_y = (player->y - state.old_player.y);
    float angle_rad = atan2f(-diff_y, diff_x);
 
    if (player->snap_rotation) {
        player->rotation = RadToDeg(angle_rad);
        player->lerp_rotation = player->rotation;
    } else if (STEPS_DT * 72 <= diff_x * diff_x + diff_y * diff_y) {
		player->rotation = RadToDeg(slerp_fancy(DegToRad(player->rotation), angle_rad, (STEPS_DT * 60) * mult));
	}
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
    
    trail->positionR = (Vec2){x, y};  
    trail->startingPositionInitialized = true;
    float calc_x = player->x - state.camera_x;
    float calc_y = SCREEN_HEIGHT - (fabsf(gravBottom(player)) - state.camera_y);

    ship_fire_particles[state.current_player].emitterX = x;
    ship_fire_particles[state.current_player].emitterY = y;
    ship_fire_particles[state.current_player].emitting = state.input.holdJump;

    ship_fire_particles[state.current_player].gravityFlipped = player->upside_down;
    ship_fire_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    ship_secondary_particles[state.current_player].emitterX = x;
    ship_secondary_particles[state.current_player].emitterY = y;
    ship_secondary_particles[state.current_player].emitting = true;

    ship_secondary_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles_2[state.current_player].emitterX = calc_x;
    drag_particles_2[state.current_player].emitterY = calc_y;
    drag_particles_2[state.current_player].emitting = player->on_ground;

    drag_particles_2[state.current_player].gravityFlipped = !player->upside_down;
    drag_particles_2[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    if (state.dual) {
        // Make both dual players symetric
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= -101.541492f)
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= -101.541492f)
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= grav(player, 101.541492f))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= grav(player, 101.541492f))
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
    trail->positionR = (Vec2){player->x, player->y};  
    trail->startingPositionInitialized = true;

    int mult = (player->upside_down ? -1 : 1);

    player->gravity = -1676.46672f;  
    
    if (player->on_ground || player->on_ceiling) {
        MotionTrail_StopStroke(trail);
        player->ball_rotation_speed = 2.3;
    }

    
    drag_particles[state.current_player].emitterX = player->x;
    drag_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -4 : 4);
    drag_particles[state.current_player].emitting = player->on_ground || player->on_ceiling;

    drag_particles[state.current_player].gravityFlipped = player->upside_down;
    drag_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    // Jump
    if ((state.input.holdJump) && (player->on_ground || player->on_ceiling) && player->buffering_state == BUFFER_READY) {        
        float delta_y = player->vel_y;

        player->upside_down ^= 1;

        set_p_velocity(player, ballJumpHeights[state.speed]);

        player->vel_y -= (delta_y < 0) ? 0 : delta_y;
        player->buffering_state = BUFFER_END;
        
        player->ball_rotation_speed = -1.f;

        player->on_ground = false;
    }
    
    player->rotation += player->ball_rotation_speed * mult * (player_speeds[state.speed] / player_speeds[SPEED_NORMAL]) / (player->mini ? 0.8 : 1);

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

    trail->positionR = (Vec2){x, y};  
    trail->startingPositionInitialized = true;

    secondary_particles[state.current_player].emitterX = player->x;
    secondary_particles[state.current_player].emitterY = fabsf(gravBottom(player)) + (player->upside_down ? -6 : 6);
    secondary_particles[state.current_player].emitting = true;

    secondary_particles[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    drag_particles_2[state.current_player].emitterX = calc_x;
    drag_particles_2[state.current_player].emitterY = calc_y;
    drag_particles_2[state.current_player].emitting = player->on_ground;

    drag_particles_2[state.current_player].gravityFlipped = !player->upside_down;
    drag_particles_2[state.current_player].scale = (player->mini ? 0.6f : 1.0f);

    int mult = (player->upside_down ? -1 : 1);
    bool buffering_check = ((state.old_player.gamemode == GAMEMODE_PLAYER || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_DART) && (state.input.holdJump));
    if (player->buffering_state == BUFFER_READY && (state.input.pressedJump || buffering_check)) {
        player->vel_y = fmaxf(player->vel_y, player->mini ? 358.992 : 371.034);
        player->buffering_state = BUFFER_END;
        player->ufo_last_y = player->y;
        player->burst_particle_timer = BURST_PARTICLES_DURATION;
    } else {
        if (!state.dual) {
            if (player->vel_y > grav(player, 103.485494)) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        } else {   
            if (player->vel_y > -103.485494) {
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

    if (player->on_ground) {
        player->ufo_last_y = player->y;
    }

    if (player->slope_data.slope_id < 0) {
        float y_diff = (player->y - player->ufo_last_y) * mult;

        if (y_diff >= 0) {
            player->rotation = map_range(y_diff, 0.f, 60.f, 0.f, 10.f) * mult;
        } else {
            player->rotation = -map_range(-y_diff, 0.f, 300.f, 0.f, 25.f) * mult;
        }
    }

    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->vel_y < min) {
        player->vel_y = min;
    } else if (player->vel_y > max) {
        player->vel_y = max;
    }
}

void wave_gamemode(Player *player) {
    trail->positionR = (Vec2){player->x, player->y};  
    trail->startingPositionInitialized = true;
    
    wave_trail->positionR = (Vec2){player->x, player->y};  
    wave_trail->startingPositionInitialized = true;
    if (player->cutscene_timer == 0) wave_trail->opacity = 1.f;

    if (player->buffering_state == BUFFER_READY) player->buffering_state = BUFFER_END;

    bool input = (state.input.holdJump);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
    if (player->vel_y != state.old_player.vel_y || player->on_ground != state.old_player.on_ground || player->on_ceiling != state.old_player.on_ceiling) {
        MotionTrail_AddWavePoint(wave_trail);
    }
}

void run_player(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;
    trail->stroke = 10.f * scale;
    
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

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            cube_gamemode(player);
            break;
        case GAMEMODE_SHIP:
            MotionTrail_ResumeStroke(trail);
            ship_gamemode(player);
            break;
        case GAMEMODE_PLAYER_BALL:
            ball_gamemode(player);
            break;
        case GAMEMODE_BIRD:
            MotionTrail_ResumeStroke(trail);
            ufo_gamemode(player);
            break;
        case GAMEMODE_DART:
            MotionTrail_ResumeStroke(trail);
            wave_gamemode(player);
            break;
    }
    
    player->time_since_ground += STEPS_DT;

    if (player->gamemode != GAMEMODE_DART || player->cutscene_timer > 0) {
        if (wave_trail->opacity > 0) wave_trail->opacity -= 0.02f;
        
        if (wave_trail->opacity <= 0) {
            wave_trail->opacity = 0;
            wave_trail->nuPoints = 0;
        }
    }

    if (player->cutscene_timer > 0) return;

    player->rotation = normalize_angle(player->rotation);
    
    if (player->snap_rotation) {
        player->lerp_rotation = player->rotation;
    } else {
        if (player->gamemode == GAMEMODE_BIRD) {
            if (player->slope_data.slope_id >= 0) {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.05f, STEPS_DT);
            } else {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.1f, STEPS_DT);
            }
        } else {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.2f, STEPS_DT);
        }
    }
    
    player->vel_x = player_speeds[state.speed];
    player->vel_y += player->gravity * STEPS_DT;
    player->y += player_get_vel(player, player->vel_y) * STEPS_DT;
    
    player->x += player->vel_x * STEPS_DT;

    player->left_ground = false;

    if (player->ceiling_inv_time > 0) {
        player->ceiling_inv_time -= STEPS_DT;
    } else {
        player->ceiling_inv_time = 0;
    }

    bool slopeCheck = player->slope_data.slope_id >= 0 && (grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope_id, player) == ORIENT_UD_DOWN);

    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) <= 0) player->vel_y = 0;
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
        player->snap_data.player_frame = 0;
    }

    // Ceiling
    if (getGroundTop(player) > state.ceiling_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && !player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) >= 0) player->vel_y = 0;
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
    } 
    
    if (player->slope_slide_coyote_time) {
        player->slope_slide_coyote_time--;
        if (!player->slope_slide_coyote_time) {
            player->coyote_slope.slope_id = -1;
            player->coyote_slope.elapsed = 0;
            player->coyote_slope.snapDown = false;
        }
    }

    if (player->slope_data.slope_id >= 0) {
        slope_calc(player->slope_data.slope_id, player);
    }

    if (player->gamemode == GAMEMODE_SHIP) rotate_fly(player, 0.15f);
    if (player->gamemode == GAMEMODE_DART) rotate_fly(player, player->mini ? 0.4f : 0.25f);

    player->snap_rotation = false;
}

float collision_time = 0;
float player_time = 0;
float handle_player_time = 0;

void handle_player(Player *player) {
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

    player->gravObj_id = -1;
    
    player->timeElapsed += STEPS_DT;

    player->frame++;

    u64 start = svcGetSystemTick();
    collide_with_objects(player);
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    collision_time += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    if (state.dead) return;

    start = svcGetSystemTick();
    run_player(player);
    end = svcGetSystemTick();
    ticks = end - start;
    player_time += ticks / CPU_TICKS_PER_MSEC;
    
    if (state.noclip) state.dead = false;
    
    player->delta_y = player->y - state.old_player.y;

    
    u64 end_player = svcGetSystemTick();
    ticks = end_player - start_player;

    if (state.hitbox_display == 2) add_new_hitbox(player);
    handle_player_time += ticks / CPU_TICKS_PER_MSEC;
}



void spawn_p1_trail(Player *player) {
    P1Trail *trail_data = &player->p1_trail_data[player->p1_trail_pos];
    
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
    trail_data->rot = player->lerp_rotation;
    trail_data->opacity = 0.7f;
    trail_data->life = P1_TRAIL_DURATION;

    trail_data->active = true;

    player->p1_trail_pos++;
    if (player->p1_trail_pos >= P1_TRAIL_LENGTH) {
        player->p1_trail_pos = 0;
    }
}

void update_p1_trail(Player *player) {
    // Spawn new p1 icon
    if (p1_trail && (frame_counter % 3) == 0) {
        spawn_p1_trail(player);
    }

    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trail_data = &player->p1_trail_data[i];

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

void draw_p1_trail(Player *player) {
    for (size_t i = 0; i < P1_TRAIL_LENGTH; i++) {
        P1Trail *trail_data = &player->p1_trail_data[i];

        if (trail_data->active) {
            float calc_x = ((trail_data->x - state.camera_x));
            float calc_y = SCREEN_HEIGHT - ((trail_data->y - state.camera_y));
            bool flip_x = (state.mirror_mult < 0);

            u32 color = (state.current_player == 1) ? C2D_Color32(p2_color.r, p2_color.g, p2_color.b, trail_data->opacity * 255) : C2D_Color32(p1_color.r, p1_color.g, p1_color.b, trail_data->opacity * 255);

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
    change_blending(true);
    draw_p1_trail(player);
    MotionTrail_Draw(trail);
    MotionTrail_DrawWaveTrail(wave_trail);

    change_blending(false);

    float calc_x = ((player->x - state.camera_x));
    float calc_y = SCREEN_HEIGHT - ((player->y - state.camera_y));

    u32 primary_color = C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255);
    u32 secondary_color = C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255);

    if (state.current_player == 1) {
        u32 tmp = primary_color;
        primary_color = secondary_color;
        secondary_color = tmp;
    }


    float scale = (player->mini) ? 0.6f : 1.f;

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
    float p_rot = player->lerp_rotation * state.mirror_mult;

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, player_glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_SHIP:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, player_glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            spawn_icon_at(GAMEMODE_SHIP, selected_ship, player_glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_PLAYER_BALL:
            spawn_icon_at(GAMEMODE_PLAYER_BALL, selected_ball, player_glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_BIRD:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, player_glow_enabled, p_x, p_y, p_rot, flip_x, player->upside_down, scale * 0.5f, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            spawn_icon_at(GAMEMODE_BIRD, selected_ufo, player_glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, player->upside_down, scale, 
                primary_color,
                secondary_color,
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;    
        case GAMEMODE_DART:
            spawn_icon_at(GAMEMODE_DART, selected_wave, player_glow_enabled, calc_x_mirror, calc_y, p_rot, flip_x, false, scale, 
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
        tri[ti].x = calc_x_on_screen(rect[i].x);
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
            calc_x_on_screen(rect[i].x), calc_y_on_screen(rect[i].y),
            calc_x_on_screen(rect[j].x), calc_y_on_screen(rect[j].y),
            1.5f, center.x, center.y, color
        );
    }
}

void draw_hitbox(int obj) {
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
    
    if (obj == state.player.slope_data.slope_id || obj == state.player2.slope_data.slope_id) color = C2D_Color32(0x00, 0xff, 0x00, 0xff);

    Vec2D rect[4];
    if (hitbox->type == COLLISION_SLOPE) {
        w = objects.width[obj];
        h = objects.height[obj];
        get_corners(x, y, w, h, 0, rect);

        draw_triangle_from_rect(rect, 3 - objects.orientation[obj], color);
    } else if (hitbox->type == COLLISION_CIRCLE) {
        //float calc_radius = hitbox->width;

        //custom_circunference(calc_x_on_screen(x), calc_y_on_screen(y), calc_radius, color, 2.f);
    } else if (w != 0 && h != 0) {
        //if (hitbox_type == HITBOX_SPECIAL && !objects.touch_triggered[obj]) return;
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

    // Circle hitbox
    //float calc_radius = (player->width / 2) * SCALE;
    //custom_circunference(calc_x_on_screen(player->x), calc_y_on_screen(player->y), calc_radius, C2D_Color32(0xff, 0x00, 0x00, 0xff), 2.f);

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