#include "state.h"
#include <string.h>
#include "main.h"
#include "math_helpers.h"
#include "player/slope.h"
#include "mp3_player.h"
#include "player/collision.h"
#include "particles/circles.h"
#include "particles/particles.h"

GameState state;

void run_camera() {
    Player *player = &state.player;
    state.old_camera_x = state.camera_x;
    state.old_camera_y = state.camera_y;

    //float calc_x = (player->x - state.camera_x);

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (player->gamemode != GAMEMODE_PLAYER || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }
    state.ground_y_gfx = ease_out(state.ground_y_gfx, calc_height, 0.02f);

    state.ground_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
    state.background_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;

    /*
    if (level_info.wall_y == 0) {
        if (state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA >= level_info.wall_x - (4.5f * 30.f)) {
            level_info.wall_y = MAX(state.camera_y, -30) + (SCREEN_HEIGHT_AREA / 2);
        }
    }

    float camera_x_right = state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA;

    if (calc_x >= get_camera_x_scroll_pos()) {
        // Cap at camera_x
        if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
            if (state.camera_wall_timer == 0) {
                state.background_wall_initial_x = state.background_x;
                state.ground_wall_initial_x = state.ground_x;
            }
            state.background_x = easeValue(EASE_IN_OUT, state.background_wall_initial_x, state.background_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
            state.ground_x = easeValue(EASE_IN_OUT, state.ground_wall_initial_x, state.ground_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
        } else {
            state.camera_x += player->vel_x * STEPS_DT;
            state.ground_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
            state.background_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
        }
    }

    if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
        if (state.camera_wall_timer == 0) {
            state.camera_wall_initial_y = state.camera_y;
        }

        float final_camera_x_wall = level_info.wall_x - (SCREEN_WIDTH_AREA + WIDTH_ADJUST_AREA);
        float final_camera_y_wall = level_info.wall_y - (SCREEN_HEIGHT_AREA / 2);   

        state.camera_x = easeValue(EASE_IN_OUT, final_camera_x_wall - CAMERA_X_WALL_OFFSET, final_camera_x_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_y = easeValue(EASE_IN_OUT, state.camera_wall_initial_y, final_camera_y_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_wall_timer += STEPS_DT;
        if (completion_shake) {
            state.camera_x = final_camera_x_wall + 3.f * random_float(-1, 1);
            state.camera_y = final_camera_y_wall + 3.f * random_float(-1, 1);
        }
    } else */
     if (player->gamemode == GAMEMODE_PLAYER && !state.dual) {
        float distance = state.camera_y_middle - player->y;
        float distance_abs = fabsf(distance);

        int mult = (distance >= 0 ? 1 : -1);

        float difference = player->y - state.old_player.y;

        if (distance_abs > 60.f && (difference * -mult > 0 || player->on_ground)) {
            float lerp_ratio = 0.1f;
            if (player->on_ground) {
                // Slowly make player in bounds (60 units from player center)
                state.camera_y_lerp = player->y + 60.f * mult - ((SCREEN_HEIGHT_AREA / 2) - CAMERA_Y_OFFSET);
                lerp_ratio = 0.2f;
            } else {
                // Move camera
                state.camera_y_lerp += difference;
            }
            // Lerp so the camera doesn't go all the way when not moving
            state.intermediate_camera_y = ease_out(state.intermediate_camera_y, state.camera_y_lerp, lerp_ratio);
        } else {
            state.camera_y_lerp = state.intermediate_camera_y;
        }

        if (state.intermediate_camera_y < 0) state.intermediate_camera_y = 0;
        if (state.camera_y_lerp > MAX_LEVEL_HEIGHT) state.camera_y_lerp = MAX_LEVEL_HEIGHT;

        state.camera_y = ease_out(state.camera_y, state.intermediate_camera_y, 0.07f);
    } else {
        state.camera_y = ease_out(state.camera_y, state.camera_intended_y, 0.02f);
        state.camera_y_lerp = state.camera_y;
        state.intermediate_camera_y = state.camera_y;
    }
	state.camera_x = player->x - 125.0f/SCALE;
    

    state.camera_x_middle = state.camera_x + (SCREEN_WIDTH_AREA / 2);
    state.camera_y_middle = state.camera_y_lerp + ((SCREEN_HEIGHT_AREA / 2) - CAMERA_Y_OFFSET);
}

void set_hitbox_size(Player *player, int gamemode) {
    float scale = (player->mini) ? 0.6f : 1.f;
    if (gamemode != GAMEMODE_DART) {
        player->height = 30 * scale;
        player->width = 30 * scale;
        
        player->internal_hitbox.width = 9;
        player->internal_hitbox.height = 9;
    } else {
        player->height = 10 * scale;
        player->width = 10 * scale;

        player->internal_hitbox.width = 3;
        player->internal_hitbox.height = 3;
    }
}

void set_intended_ceiling() {
    float mid_point = (state.ground_y + state.ceiling_y) / 2;
    state.camera_intended_y = mid_point - ((SCREEN_HEIGHT_AREA / 2) - CAMERA_Y_OFFSET);
}

void set_gamemode(Player *player, int gamemode) {
    player->gamemode = gamemode;
    set_hitbox_size(player, gamemode);
}

void set_mini(Player *player, bool mini) {
    player->mini = mini;
    set_hitbox_size(player, player->gamemode);
}

void init_variables() {
    level_frame = 0;
    
    C2D_Image img = C2D_SpriteSheetGetImage(trailSheet, 0);

    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    MotionTrail_Init(&trail_p1, 0.3f, 3, 10.0f, false, get_white_if_black(p2_color), img);
    MotionTrail_Init(&trail_p2, 0.3f, 3, 10.0f, false, get_white_if_black(p1_color), img);
    MotionTrail_Init(&wave_trail_p1, 3.f, 3, 10.0f, true, get_white_if_black(p2_color), img);
    MotionTrail_Init(&wave_trail_p2, 3.f, 3, 10.0f, true, get_white_if_black(p1_color), img);
    MotionTrail_StopStroke(&trail_p1);
    MotionTrail_StopStroke(&trail_p2);

    state.camera_wall_timer = 0;
    state.camera_wall_initial_y = 0;

    state.mirror_factor = 0;
    state.mirror_speed_factor = 1.f;
    state.intended_mirror_factor = 0;
    state.intended_mirror_speed_factor = 1.f;


    current_fading_effect = FADE_NONE;
    memset(&state.player.p1_trail_data, 0, sizeof(P1Trail) * P1_TRAIL_LENGTH);
    memset(&state.player.snap_data, 0, sizeof(SnapData));
    state.player.p1_trail_pos = 0;
    p1_trail = false;
    state.death_timer = 0.f;

    level_info.completing = false;
    
    memset(&state.player, 0, sizeof(Player));
    //memset(&state.hitbox_trail_players, 0, sizeof(state.hitbox_trail_players));
    //state.last_hitbox_trail = 0;

    state.dual = false;
    state.dead = false;
    state.mirror_mult = 1;

    Player *player = &state.player;
    clear_slope_data(player);
    player->cutscene_timer = 0;
    player->width = 30;
    player->height = 30;
    state.speed = level_info.initial_speed;
    player->x = 0;
    player->y = player->height / 2;
    player->vel_x = player_speeds[state.speed];  
    player->vel_y = 0;
    player->frame = 0;
    state.ground_y = 0;
    state.ceiling_y = 999999;

    state.current_player = 0;

    set_gamemode(player, level_info.initial_gamemode);
    player->on_ground = true;
    player->on_ceiling = false;
    player->inverse_rotation = false;
    set_mini(player, level_info.initial_mini);
    player->upside_down = level_info.initial_upsidedown;
    player->timeElapsed = 0.f;

    player->internal_hitbox.height = 9;
    player->internal_hitbox.width = 9;

    player->cutscene_initial_player_x = 0;
    player->cutscene_initial_player_y = 0;

    switch (level_info.initial_gamemode) {
        case GAMEMODE_SHIP:
        case GAMEMODE_BIRD:
        case GAMEMODE_DART:
            state.ceiling_y = state.ground_y + 300;
            set_intended_ceiling();
            break;
        case GAMEMODE_PLAYER_BALL:
            state.ceiling_y = state.ground_y + 240;
            set_intended_ceiling();
            break;
        case GAMEMODE_PLAYER:
            state.camera_intended_y = 0;
    }
    
    if (level_info.initial_dual) {
        state.dual = true;
        state.dual_portal_y = 0.f;
        setup_dual();
    }
    run_camera();

    // Set camera vertical pos
    state.camera_y = state.camera_intended_y;
    state.camera_y_lerp = state.camera_y;
    state.intermediate_camera_y = state.camera_y;

    
    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (player->gamemode != GAMEMODE_PLAYER || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }

    state.ground_y_gfx = calc_height;   
}

void handle_death() {
    play_sfx(&explode_sound, 1);
    if (song_loaded) {
        pause_playback_mp3();
        seek_mp3(level_info.song_offset);
    }

    Player *player = (state.current_player == 1) ? &state.player2 : &state.player;

    UseEffect *effect = add_use_effect(player->x, player->y, -1, &death_effect, GFX_TOP);
    if (effect) {
        Color color_not_white = get_white_if_black((state.current_player == 1 ? p2_color : p1_color));

        effect->def.colorR = color_not_white.r / 255.f;
        effect->def.colorG = color_not_white.g / 255.f;
        effect->def.colorB = color_not_white.b / 255.f;

        effect->def.end_rad *= (player->mini ? 0.6 : 1.0f);
        effect->def.start_rad *= (player->mini ? 0.6 : 1.0f);
    }

    explosion_particles[state.current_player].emitterX = player->x;
    explosion_particles[state.current_player].emitterY = player->y;
    explosion_particles[state.current_player].scale = (player->mini ? 0.6 : 1.0f);
    spawnMultipleParticles(&explosion_particles[state.current_player], 90);
    
}