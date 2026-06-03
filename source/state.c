#include "state.h"
#include <string.h>
#include "main.h"
#include "math_helpers.h"
#include "player/slope.h"
#include "mp3_player.h"
#include "player/collision.h"
#include "particles/circles.h"
#include "particles/particles.h"
#include "menus/settings.h"
#include "menus/icon_kit.h"
#include "utils/json_config.h"
#include "level/main_levels.h"
#include "menus/level_select.h"
#include "save/saving.h"

GameState state;

void run_camera() {
    Player *player = &state.player;
    state.old_camera_x = state.camera_x;
    state.old_camera_y = state.camera_y;

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (player->gamemode != GAMEMODE_PLAYER || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }
    state.ground_y_gfx = ease_out(state.ground_y_gfx, calc_height, 0.02f);

    if (level_info.wall_y == 0) {
        if (state.camera_x + SCREEN_WIDTH_AREA >= level_info.wall_x - (4.5f * 30.f)) {
            level_info.wall_y = MAX(state.camera_y_middle, 60 + ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET));
        }
    }

    float camera_x_right = state.camera_x + SCREEN_WIDTH_AREA;

    // Cap at camera_x
    if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
        if (state.camera_wall_timer == 0) {
            state.background_wall_initial_x = state.background_x;
            state.ground_wall_initial_x = state.ground_x;
        }
        state.background_x = easeValue(EASE_IN_OUT, state.background_wall_initial_x, state.background_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
        state.ground_x = easeValue(EASE_IN_OUT, state.ground_wall_initial_x, state.ground_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
    }

    if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
        if (state.camera_wall_timer == 0) {
            state.camera_wall_initial_y = state.camera_y;
        }

        float final_camera_x_wall = level_info.wall_x - (SCREEN_WIDTH_AREA);
        float final_camera_y_wall = level_info.wall_y - ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);   

        state.camera_x = easeValue(EASE_IN_OUT, final_camera_x_wall - CAMERA_X_WALL_OFFSET, final_camera_x_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_y = easeValue(EASE_IN_OUT, state.camera_wall_initial_y, final_camera_y_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_wall_timer += STEPS_DT;
        if (state.completion_shake) {
            state.camera_x = final_camera_x_wall + 3.f * random_float(-1, 1);
            state.camera_y = final_camera_y_wall + 3.f * random_float(-1, 1);
        }
    } else { 
        float cam_y = state.camera_y;

        float target_y = cam_y;

        if (player->gamemode == GAMEMODE_PLAYER && !state.dual) {
            float player_y = player->y;

            const float half_view = (SCREEN_HEIGHT_AREA / 2);
            const float bgparallaxdrop = LEVEL_Y_OFFSET;

            float anchor_y = cam_y - bgparallaxdrop + half_view;

            if (player_y > anchor_y + 70.0f) {
                target_y = player_y - half_view - 70.0f + bgparallaxdrop;
            } else if (player_y < anchor_y - 40.0f) {
                target_y = player_y - half_view + 40.0f + bgparallaxdrop;
            }
        } else {
            target_y = state.camera_intended_y;
        }
            
        if (target_y < 0) {
            target_y = 0;
        }
        
        cam_y += (target_y - cam_y) / (10 / 0.25f);
        
        if (cam_y < 0) {
            cam_y = 0;
        }
        
        if (cam_y > MAX_LEVEL_HEIGHT - SCREEN_HEIGHT_AREA) {
            cam_y = MAX_LEVEL_HEIGHT - SCREEN_HEIGHT_AREA;
        }

        state.camera_y = cam_y;

        state.camera_x = player->x - 125.0f/SCALE;
        
        if (state.current_data.attempts == 1) {
            if (state.camera_x < 0) {
                state.camera_x = 0;
            }
        }

        if (state.current_data.attempts > 1 || player->x - 125.0f/SCALE > 0) {
            state.ground_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
            state.background_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
        }
    }
    
    state.camera_x_middle = state.camera_x + (SCREEN_WIDTH_AREA / 2);
    state.camera_y_middle = state.camera_y + (SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET;
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
    state.camera_intended_y = mid_point - ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);
}

void set_gamemode(Player *player, int gamemode) {
    player->gamemode = gamemode;
    set_hitbox_size(player, gamemode);
}

void set_mini(Player *player, bool mini) {
    player->mini = mini;
    set_hitbox_size(player, player->gamemode);
}

void update_attempt_text_pos() {
    // Set attempt text positions
    if (state.current_data.attempts == 1) {
        state.attempt_text_pos.x = SCREEN_WIDTH_AREA / 2;
    } else {
        state.attempt_text_pos.x = state.player.x + (5 * 30);
    }

    state.attempt_text_pos.y = state.camera_y + (5 * 30);
}

void clear_snap_data(Player *player) {
    player->snap_data.snapped_obj = -1;
    player->snap_data.player_frame = 0;
    player->snap_data.player_snap_diff = 0;
    player->snap_data.object_id = -1;
}

void init_player(Player *player) {
    memset(player, 0, sizeof(Player));
    
    set_gamemode(player, level_info.initial_gamemode);
    set_mini(player, level_info.initial_mini);

    player->player_icons.cube = selected_cube;
    player->player_icons.ship = selected_ship;
    player->player_icons.ball = selected_ball;
    player->player_icons.ufo  = selected_ufo;
    player->player_icons.wave = selected_wave;
    player->player_icons.glow = player_glow_enabled;

    player->player_icons.p1_color = p1_color;
    player->player_icons.p2_color = p2_color;
    player->player_icons.glow_color = glow_color;
    
    player->cutscene_timer = 0;
    player->x = 0;
    player->y = player->height / 2;
    player->vel_x = player_speeds[state.speed];  
    player->vel_y = 0;
    player->new_vel_y = __FLT_MAX__;
    player->frame = 0;

    player->on_ground = true;
    player->on_ceiling = false;
    player->inverse_rotation = false;
    player->upside_down = level_info.initial_upsidedown;
    player->timeElapsed = 0.f;

    player->internal_hitbox.height = 9;
    player->internal_hitbox.width = 9;

    player->cutscene_initial_player_x = 0;
    player->cutscene_initial_player_y = 0;

    clear_slope_data(player);
    clear_snap_data(player);
}

void init_state() {
    state.current_player = 0;

    state.camera_x = 0;

    state.camera_wall_timer = 0;
    state.camera_wall_initial_y = 0;

    state.mirroring = false;
    state.original_mirror_factor = 0.f;
    state.intended_mirror_factor = 0.f;
    state.mirror_factor = 0.f;
    state.mirror_speed_factor = 1.f;
    state.mirror_mult = 1;

    state.hitbox_enabled_when_dead = false;
    state.death_timer = 0.f;

    state.dual = false;
    state.dead = false;
    state.mirror_mult = 1;
    state.speed = level_info.initial_speed;
    state.ground_y = 0;
    state.ceiling_y = 999999;
    
    level_info.wall_y = 0;

    state.end_wall_anim_playing = false;
    
    state.p1_trail_pos[0] = 0;
    state.p1_trail_pos[1] = 0;

    state.current_data.attempts++;
    state.current_data.coin1 = false;
    state.current_data.coin2 = false;
    state.current_data.coin3 = false;
}

void init_level_bounds() {
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
        set_dual_bounds();
    }
    run_camera();

    // Set camera vertical pos
    state.camera_y = state.camera_intended_y;
    state.camera_y_middle = state.camera_y + ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (state.player.gamemode != GAMEMODE_PLAYER || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }
    
    state.ground_y_gfx = calc_height; 

    update_attempt_text_pos();
}

void first_load_init_variables() {
    memset(&state.current_data, 0, sizeof(StateLevelData));
    
    int rounded_last_obj_x = (int) (level_info.last_obj_x / 30) * 30 + 15;
    level_info.wall_x = rounded_last_obj_x;
    level_info.wall_y = 0;

    state.background_x = 0;
    state.ground_x = 0;
    
    state.camera_x = 0;
    state.camera_y = 0;
    current_fading_effect = FADE_NONE;

    init_variables();
    
    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);
    state.current_data.time_start = svcGetSystemTick() / (CPU_TICKS_PER_MSEC * 1000);
    state.current_data.max_normal = level_data_sel->normal_progress;
    state.current_data.max_practice = level_data_sel->practice_progress;
}

void init_wave_trails() {
    C2D_Image img = C2D_SpriteSheetGetImage(trailSheet, 0);
    Color used_p1 = (switchWaveTrailColor ? p1_color : p2_color);
    Color used_p2 = (switchWaveTrailColor ? p2_color : p1_color);
    
    MotionTrail_Init(&wave_trail_p1, 0, 3.f, true, 10.0f, true, (used_p1.r | used_p1.g | used_p1.b && !solidWaveTrail), false, used_p1, img);
    MotionTrail_Init(&wave_trail_p2, 1, 3.f, true, 10.0f, true, (used_p2.r | used_p2.g | used_p2.b && !solidWaveTrail), false, used_p2, img);
}

void init_trails(int trail) {
    const MotionTrailConfig *config = &trail_properties[trail];

    C2D_Image img = C2D_SpriteSheetGetImage(trailSheet, trail);

    Color used_p1 = (switchTrailColor ? p1_color : p2_color);
    Color used_p2 = (switchTrailColor ? p2_color : p1_color);

    if (!config->colored) {
        used_p1 = white;
        used_p2 = white;
    }

    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    MotionTrail_Init(&trail_p1, 0, config->fade, config->always_on, config->width, false, config->blending, config->stationary, used_p1, img);
    MotionTrail_Init(&trail_p2, 1, config->fade, config->always_on, config->width, false, config->blending, config->stationary, used_p2, img);

    MotionTrail_StopStroke(&trail_p1);
    MotionTrail_StopStroke(&trail_p2);
}

void init_variables() {
    level_frame = 0;
   
    init_trails(selected_trail);
    init_wave_trails();

    clear_use_effects(GFX_TOP);

    current_fading_effect = FADE_NONE;
    level_info.completing = false;

    init_player(&state.player);
    init_player(&state.player2);

    init_state();

    init_level_bounds();

    p1_trail = false;

    clear_bg_flash();
}

void handle_death(Player *player, bool pause_song) {
    play_sfx(&explode_sound, 1);
    if (song_loaded && pause_song && !state.practice_mode) {
        pause_playback_mp3();
        seek_mp3(level_info.song_offset);
    }

    // Spawn death particles
    UseEffect *effect = add_use_effect(player->x, player->y, USE_EFFECT_OBJ_NOTHING, &death_effect, GFX_TOP);
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
    
    if (hitboxesOnDeath) {
        state.hitbox_enabled_when_dead = true;
    }
}

// Size portal flashing

void start_bg_flash() {
    BGFlashData *data = &state.flash_data;
    data->flashing = true;
    data->timer = FLASH_TIME_1;
    data->state = FLASH_FIRST_LIGHT;
    data->use_lbg = true;
}

void handle_bg_flash() {
    BGFlashData *data = &state.flash_data;
    if (!data->flashing) return;

    // Run flashing state
    switch (data->state) {
        case FLASH_NONE:
            break;
        
        case FLASH_FIRST_LIGHT:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->state = FLASH_UNLIGHTED;
                data->timer = FLASH_TIME_2;
                data->use_lbg = false;
            }
            break;
        case FLASH_UNLIGHTED:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->state = FLASH_SECOND_LIGHT;
                data->timer = FLASH_TIME_2;
                data->use_lbg = true;
            }
            break;
        case FLASH_SECOND_LIGHT:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                // End flash
                data->state = FLASH_NONE;
                data->timer = 0;
                data->use_lbg = false;
                data->flashing = false;
            }
            break;
    }
}

void clear_bg_flash() {
    state.flash_data.flashing = false;
    state.flash_data.use_lbg = false;
}

void play_level_song() {
    if (level_info.custom_song_id > 0) {
        char full_path[273];
        snprintf(full_path, sizeof(full_path), "%s/%d.mp3", USER_SONGS_DIR, level_info.custom_song_id);
        song_loaded = play_mp3(full_path, false, level_info.song_offset);
    } else {
        if (state.custom_level) {
            song_loaded = play_mp3(main_levels[level_info.song_id].song_path, false, level_info.song_offset);
        } else {
            song_loaded = play_mp3(main_levels[curr_level_id].song_path, false, 0);
        }
    }
}