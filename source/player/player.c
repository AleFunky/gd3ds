#include "player.h"
#include "state.h"
#include "icons.h"
#include "graphics.h"

#include "menus/icon_kit.h"
#include "collision.h"


const float player_speeds[SPEED_COUNT] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566
};

const float cube_jump_heights[SPEED_COUNT] = {
    573.48,
    603.72,
    616.68,
    606.42,
};

float player_get_vel(Player *player, float vel) {
    return vel * (player->upside_down ? -1 : 1);
}

void set_p_velocity(Player *player, float vel) {
    player->vel_y = vel * ((player->mini) ? 0.8 : 1);
}

void cube_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    
    //trail.positionR = (Vec2){player->x, player->y};  
    //trail.startingPositionInitialized = true;

    player->gravity = -2794.1082;
    
    if (player->vel_y < -810) player->vel_y = -810;
    if (player->vel_y > 1080) player->vel_y = 1080;

    if (player->y > 2794.f) state.dead = true;

    if (player->inverse_rotation) {
        player->rotation -= (415.3848f / 2) * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    } else {
        player->rotation += 415.3848f * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    }
    
    
    if (player->on_ground) {
        //MotionTrail_StopStroke(&trail);
        //if (!player->slope_data.slope) player->rotation = round(player->rotation / 90.0f) * 90.0f;
        player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }
    /*
    // Player on ground or just left the ground
    if ((player->time_since_ground < 0.05f) && (frame_counter & 0b11) == 0) {
        particle_templates[CUBE_DRAG].angle = (player->upside_down ? -90 : 90);
        particle_templates[CUBE_DRAG].gravity_y = (player->upside_down ? 300 : -300);
        spawn_particle(CUBE_DRAG, getLeft(player) + 4, (player->upside_down ? getTop(player) - 2 : getBottom(player) + 2), NULL);
    }

    SlopeData slope_data = player->slope_data;

    // If not currently on slope, look at the last frame
    if (!player->slope_data.slope && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }*/
    //if ((slope_data.slope || player->on_ground) && (state.input.holdJump)) {

    if (player->on_ground && (state.input.holdJump)) {
        /*if (slope_data.slope) {
            // Slope jump
            int orient = grav_slope_orient(slope_data.slope, player);
            if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
                float time = clampf(10 * (player->timeElapsed - slope_data.elapsed), 0.4f, 1.0f);
                set_p_velocity(player, 0.25f * time * slopeHeights[state.speed] + cube_jump_heights[state.speed]);
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed]);
            }
        } else {*/
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed]);
        //}
        player->inverse_rotation = false;
    
        player->on_ground = false;
        player->buffering_state = BUFFER_END;
    
        if (!(state.input.pressedJump)) {
            // This simulates the holding jump
            player->vel_y -= player->gravity * STEPS_DT;
        }
    }
}

void run_player(Player *player) {
    //float scale = (player->mini) ? 0.6f : 1.f;
    //trail.stroke = 10.f * scale;
    
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

            /*if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }*/
            break;
        /*case GAMEMODE_SHIP:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            ship_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_BALL:
            ball_gamemode(player);
            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_UFO:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            ufo_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_WAVE:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            wave_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;*/
    }
    
    player->time_since_ground += STEPS_DT;

    /*if (player->gamemode != GAMEMODE_DART || player->cutscene_timer > 0) {
        if (wave_trail.opacity > 0) wave_trail.opacity -= 0.02f;
        
        if (wave_trail.opacity <= 0) {
            wave_trail.opacity = 0;
            wave_trail.nuPoints = 0;
        }
    }*/

    if (player->cutscene_timer > 0) return;

    player->rotation = normalize_angle(player->rotation);
    
    if (player->snap_rotation) {
        player->lerp_rotation = player->rotation;
    } else {
        /*if (player->gamemode == GAMEMODE_BIRD) {
            if (player->slope_data.slope) {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.05f, STEPS_DT);
            } else {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.1f, STEPS_DT);
            }
        } else {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.2f, STEPS_DT);
        }*/
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

    
    bool slopeCheck = false; //player->slope_data.slope && (grav_slope_orient(player->slope_data.slope, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope, player) == ORIENT_UD_DOWN);/

    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            //clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) <= 0) player->vel_y = 0;
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
    }

    // Ceiling
    if (getGroundTop(player) > state.ceiling_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && !player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            //clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) >= 0) player->vel_y = 0;
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
    } 
    /*
    if (player->slope_slide_coyote_time) {
        player->slope_slide_coyote_time--;
        if (!player->slope_slide_coyote_time) {
            player->coyote_slope.slope = NULL;
            player->coyote_slope.elapsed = 0;
            player->coyote_slope.snapDown = FALSE;
        }
    }

    if (player->slope_data.slope) {
        slope_calc(player->slope_data.slope, player);
    }
    
    if (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_WAVE) update_ship_rotation(player);
*/
    player->snap_rotation = false;
}

void handle_player(Player *player) {
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

    collide_with_objects(player);
    
    run_player(player);
    
    player->delta_y = player->y - state.old_player.y;

}

void draw_player(Player *player) {
    float calc_x = ((player->x - state.camera_x));
    float calc_y = GSP_SCREEN_WIDTH - ((player->y - state.camera_y));

    /*MotionTrail_Update(&trail, dt);
    MotionTrail_UpdateWaveTrail(&wave_trail, dt);

    MotionTrail_Draw(&trail);
    MotionTrail_DrawWaveTrail(&wave_trail);*/

    float scale = (player->mini) ? 0.6f : 1.f;

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, player_glow_enabled, calc_x, calc_y, player->rotation, false, false, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
    }
}