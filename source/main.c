#include <citro2d.h>

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "objects.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "level/main_levels.h"
#include "fonts/bigFont.h"

#include "save/config.h"

#include <curl/curl.h>

#include "menus/main_menu.h"
#include "menus/level_select.h"
#include "menus/icon_kit.h"
#include "menus/gameplay.h"
#include "menus/components/ui_screen.h"

#include "player/collision.h"
#include "state.h"

#include "particles/particles.h"
#include "particles/object_particles.h"

#include <stdarg.h>

#include "player/player.h"
#include "particles/circles.h"
#include "menus/settings.h"
#include "menus/external_levels.h"

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

int game_state = STATE_MAIN_MENU;

bool playing_menu_loop = false;

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* bot;

SFX play_sound;
SFX quit_sound;
SFX explode_sound;

ParticleSystem touch_drag_particles;
ParticleSystem touch_explosion_particles;

float slow_speed_particles_timer = 0.f;
float normal_speed_particles_timer = 0.f;
float fast_speed_particles_timer = 0.f;
float faster_speed_particles_timer = 0.f;

bool alt_title_screen;

bool is_citra() {
    s64 version = 0;
    svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
    return version != 0;
}

void no_dsp_firmware(void) {
    consoleInit(GFX_TOP, NULL);
    printf("\x1b[01;00H/////////////////FATAL///ERROR////////////////////");
    printf("\x1b[03;00HNDSP could not be initalized.");
    printf("\x1b[05;00HThis is probably because your dspfirm is missing.");
    printf("\x1b[07;00HExtract the ndsp firmware (dspfirm.cdc) to");
    printf("\x1b[09;00Hsdmc:/3ds/ on your sd card.");
    printf("\x1b[11;00HCitra/Azahar users only need the file to be there,");
    printf("\x1b[13;00Hit can be empty.");
    printf("\x1b[15;00HPress start to exit.");
    printf("\x1b[30;00H//////////////////////////////////////////////////");
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

        gfxSwapBuffers();
        gfxFlushBuffers();
    }
    
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    exit(22);
}

static FILE *log_file = NULL;

static void ensure_log_file(void) {
    if (log_file) return;  // already open

    char log_filename[278];
    snprintf(log_filename, sizeof(log_filename), "%s/%s", CONFIG_ROOT, "output.txt");
    
    log_file = fopen(log_filename, "a");
}

static void close_log_file(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

int output_log(const char *fmt, ...) {
    ensure_log_file();

    if (!log_file) return 0; // failed to open log

    va_list args1, args2;
    int ret;

    va_start(args1, fmt);
    va_copy(args2, args1);  // duplicate args for two outputs

    // Output to log file
    ensure_log_file();
    if (log_file) {
        ret = vfprintf(log_file, fmt, args1);
        fflush(log_file);
    }

    // Output to console (stdout)
    ret = vprintf(fmt, args2);
    
    va_end(args2);
    va_end(args1);

    return ret;
}

float sprite_drawing_time = 0;
float physics_calc_time = 0;
float particle_calc_time = 0;
float triggers_time = 0;

float delta = 0;
unsigned int level_frame = 0;
unsigned int frame_counter = 0;

bool exiting_level = false;

bool song_loaded;

void game_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    draw_text(bigFont_fontCharset, bigFont_sheet, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 0.5f, 1.0f, "Loading...");
    C3D_FrameEnd(0);
    
    char *path;

    if (state.custom_level) {
        path = state.custom_level_path;
    } else {
        path = main_levels[curr_level_id].gmd_path;
        level_info.level_name = main_levels[curr_level_id].level_name;
    }

    int returned = load_level(path);
    if (returned) {
        printf("\x1b[9;1HFailed %d", returned);
        game_state = (state.custom_level ? STATE_EXTERNAL_LEVELS : STATE_LEVEL_SELECT);
        return;
    }
    

    if (level_info.custom_song_id >= 0) {
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

    if (song_loaded) pause_playback_mp3();

    state.camera_x = 0;
    state.camera_y = 0;
    current_fading_effect = FADE_NONE;

    set_fade_status(FADE_STATUS_IN);

    bool being_faded = true;

    init_variables();

    init_op_system();

    gameplay_screen_init();
    
    // Particle
    initParticleSystem(&drag_particles[0], &drag_effect);
    initParticleSystem(&drag_particles[1], &drag_effect);

    initParticleSystem(&drag_particles_2[0], &ship_drag_effect);
    initParticleSystem(&drag_particles_2[1], &ship_drag_effect);

    initParticleSystem(&ship_fire_particles[0], &drag_effect);
    initParticleSystem(&ship_fire_particles[1], &drag_effect);

    initParticleSystem(&secondary_particles[0], &ship_effect);
    initParticleSystem(&secondary_particles[1], &ship_effect);
    
    initParticleSystem(&ship_secondary_particles[0], &ship_effect);
    initParticleSystem(&ship_secondary_particles[1], &ship_effect);

    initParticleSystem(&burst_particles[0], &burst_effect);
    initParticleSystem(&burst_particles[1], &burst_effect);

    initParticleSystem(&land_particles[0], &land_effect);
    initParticleSystem(&land_particles[1], &land_effect);
    
    initParticleSystem(&explosion_particles[0], &explode_effect);
    initParticleSystem(&explosion_particles[1], &explode_effect);

    initParticleSystem(&brick_destroy_particles, &glass_destroy_01);
    initParticleSystem(&glitter_particles, &glitter_effect);
    initParticleSystem(&slow_speed_particles, &speed_effect_slow);
    initParticleSystem(&normal_speed_particles, &speed_effect_normal);
    initParticleSystem(&fast_speed_particles, &speed_effect_fast);
    initParticleSystem(&faster_speed_particles, &speed_effect_vfast);
    initParticleSystem(&coin_pickup_particles, &coin_pickup_effect);
    
    slow_speed_particles.stationary = true;
    normal_speed_particles.stationary = true;
    fast_speed_particles.stationary = true;
    faster_speed_particles.stationary = true;

    Color p1_not_white = get_white_if_black(p1_color);
    Color p2_not_white = get_white_if_black(p2_color);

    drag_particles[0].cfg.startColorRed   = p1_not_white.r / 255.f;
    drag_particles[0].cfg.startColorGreen = p1_not_white.g / 255.f;
    drag_particles[0].cfg.startColorBlue  = p1_not_white.b / 255.f;

    drag_particles[1].cfg.startColorRed   = p2_not_white.r / 255.f;
    drag_particles[1].cfg.startColorGreen = p2_not_white.g / 255.f;
    drag_particles[1].cfg.startColorBlue  = p2_not_white.b / 255.f;

    burst_particles[0].cfg.startColorRed   = p1_not_white.r / 255.f;
    burst_particles[0].cfg.startColorGreen = p1_not_white.g / 255.f;
    burst_particles[0].cfg.startColorBlue  = p1_not_white.b / 255.f;
    burst_particles[0].cfg.finishColorRed   = p1_not_white.r / 255.f;
    burst_particles[0].cfg.finishColorGreen = p1_not_white.g / 255.f;
    burst_particles[0].cfg.finishColorBlue  = p1_not_white.b / 255.f;

    burst_particles[1].cfg.startColorRed   = p2_not_white.r / 255.f;
    burst_particles[1].cfg.startColorGreen = p2_not_white.g / 255.f;
    burst_particles[1].cfg.startColorBlue  = p2_not_white.b / 255.f;
    
    burst_particles[1].cfg.finishColorRed   = p2_not_white.r / 255.f;
    burst_particles[1].cfg.finishColorGreen = p2_not_white.g / 255.f;
    burst_particles[1].cfg.finishColorBlue  = p2_not_white.b / 255.f;

    secondary_particles[0].cfg.startColorRed   = p2_not_white.r / 255.f;
    secondary_particles[0].cfg.startColorGreen = p2_not_white.g / 255.f;
    secondary_particles[0].cfg.startColorBlue  = p2_not_white.b / 255.f;

    secondary_particles[1].cfg.startColorRed   = p1_not_white.r / 255.f;
    secondary_particles[1].cfg.startColorGreen = p1_not_white.g / 255.f;
    secondary_particles[1].cfg.startColorBlue  = p1_not_white.b / 255.f;

    ship_secondary_particles[0].cfg.startColorRed   = 255.f/ 255.f;
    ship_secondary_particles[0].cfg.startColorGreen = 65.f / 255.f;
    ship_secondary_particles[0].cfg.startColorBlue  = 0.f / 255.f;

    ship_secondary_particles[1].cfg.startColorRed   = 255.f / 255.f;
    ship_secondary_particles[1].cfg.startColorGreen = 65.f / 255.f;
    ship_secondary_particles[1].cfg.startColorBlue  = 0.f/ 255.f;

    ship_fire_particles[0].cfg.startColorRed   = 255.f / 255.f;
    ship_fire_particles[0].cfg.startColorGreen = 65.f / 255.f;
    ship_fire_particles[0].cfg.startColorBlue  = 0.f / 255.f;

    ship_fire_particles[1].cfg.startColorRed   = 255.f / 255.f;
    ship_fire_particles[1].cfg.startColorGreen = 65.f / 255.f;
    ship_fire_particles[1].cfg.startColorBlue  = 0.f / 255.f;

    land_particles[0].cfg.startColorRed   = p1_not_white.r / 255.f;
    land_particles[0].cfg.startColorGreen = p1_not_white.g / 255.f;
    land_particles[0].cfg.startColorBlue  = p1_not_white.b / 255.f;

    land_particles[1].cfg.startColorRed   = p2_not_white.r / 255.f;
    land_particles[1].cfg.startColorGreen = p2_not_white.g / 255.f;
    land_particles[1].cfg.startColorBlue  = p2_not_white.b / 255.f;

    explosion_particles[0].cfg.startColorRed   = p1_not_white.r / 255.f;
    explosion_particles[0].cfg.startColorGreen = p1_not_white.g / 255.f;
    explosion_particles[0].cfg.startColorBlue  = p1_not_white.b / 255.f;

    explosion_particles[1].cfg.startColorRed   = p2_not_white.r / 255.f;
    explosion_particles[1].cfg.startColorGreen = p2_not_white.g / 255.f;
    explosion_particles[1].cfg.startColorBlue  = p2_not_white.b / 255.f;

    glitter_particles.cfg.startColorRed   = p1_not_white.r / 255.f;
    glitter_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
    glitter_particles.cfg.startColorBlue  = p1_not_white.b / 255.f;
    glitter_particles.cfg.startColorAlpha = 1.f;
    
    glitter_particles.cfg.finishColorRed   = p1_not_white.r / 255.f;
    glitter_particles.cfg.finishColorGreen = p1_not_white.g / 255.f;
    glitter_particles.cfg.finishColorBlue  = p1_not_white.b / 255.f;

    slow_speed_particles.cfg.startColorRed   = 255 / 255.f;
    slow_speed_particles.cfg.startColorGreen = 255 / 255.f;
    slow_speed_particles.cfg.startColorBlue  = 0 / 255.f;

    normal_speed_particles.cfg.startColorRed   = 0 / 255.f;
    normal_speed_particles.cfg.startColorGreen = 190 / 255.f;
    normal_speed_particles.cfg.startColorBlue  = 255 / 255.f;

    fast_speed_particles.cfg.startColorRed   = 0 / 255.f;
    fast_speed_particles.cfg.startColorGreen = 255 / 255.f;
    fast_speed_particles.cfg.startColorBlue  = 0 / 255.f;

    faster_speed_particles.cfg.startColorRed   = 230 / 255.f;
    faster_speed_particles.cfg.startColorGreen = 65 / 255.f;
    faster_speed_particles.cfg.startColorBlue  = 255 / 255.f;

    coin_pickup_particles.cfg.startColorRed   = 255 / 255.f;
    coin_pickup_particles.cfg.startColorGreen = 190 / 255.f;
    coin_pickup_particles.cfg.startColorBlue  = 0 / 255.f;

    exiting_level = false;

    float accumulator = 0.0f;
    u64 lastTime = svcGetSystemTick();
    u64 start = svcGetSystemTick();
    bool fixed_dt = true;
    bool old_wide = wideEnabled;

    // Main loop
    while (aptMainLoop()) {
        start = svcGetSystemTick();
        hidScanInput();
        
        touchPosition touchPos;
        hidTouchRead(&touchPos);

        // Respond to user input
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            if (game_paused) {
                unpause_game();
            } else {
                pause_game();
            }
        }

        if (kDown & KEY_X)
            state.noclip ^= 1;

        if (kDown & KEY_L)
            state.profiling ^= 1;

        if (kDown & KEY_R) {
            state.hitbox_display++;
            if (state.hitbox_display > 2) state.hitbox_display = 0;
        }      

        int steps = 0;

        u32 kHeld = hidKeysHeld();

        bool in_bounds = touchPos.px < 320 - 30 || touchPos.py > 30;
        
        bool buttonPressed = (yJump ? (kDown & KEY_Y) : (kDown & KEY_A)) || (kDown & KEY_UP);
        bool buttonHeld = (yJump ? (kHeld & KEY_Y) : (kHeld & KEY_A)) || (kHeld & KEY_UP);

        state.old_input = state.input;
        state.input.pressedJump = (buttonPressed || (in_bounds && (kDown & KEY_TOUCH))) == true;
        state.input.holdJump = (state.input.pressedJump || buttonHeld || (in_bounds && (kHeld & KEY_TOUCH))) == true;
        
        for (int i = 0; i < 2; i++) {
            drag_particles[i].emitting = false;
            drag_particles_2[i].stationary = true;
            drag_particles_2[i].emitting = false;
            ship_fire_particles[i].emitting = false;
            secondary_particles[i].emitting = false;
            ship_secondary_particles[i].emitting = false;
            burst_particles[i].emitting = false;
            land_particles[i].emitting = false;
        }

        if (state.death_timer <= 0)  {
            physics_calc_time = 0;
            number_of_collisions = 0;
            number_of_collisions_checks = 0;
            collision_time = 0;
            player_time = 0;
            handle_player_time = 0;
            
            brick_destroy_particles.emitting = false;
            glitter_particles.emitting = false;

            u64 now = svcGetSystemTick();
            delta = (now - lastTime) / (CPU_TICKS_PER_MSEC * 1000);
            lastTime = now;
            if (delta > 0.5f) delta = STEPS_DT_UNMOD; // Avoid spiral of death
            if (fixed_dt) {
                delta = STEPS_DT_UNMOD;
                if (!being_faded) fixed_dt = false;
            }

            if (!game_paused) {
                accumulator += delta;
                // Run simulation in fixed steps
                while (accumulator >= STEPS_DT_UNMOD) {
                    u64 start_physics = svcGetSystemTick();
                    state.current_player = 0;
                    state.old_player = state.player;
                    
                    trail = &trail_p1;
                    wave_trail = &wave_trail_p1;
                    handle_player(&state.player);
                    handle_mirror_transition();

                    state.level_progress = (state.player.x / level_info.last_obj_x) * 100;

                    if (state.dead) break;

                    if (state.dual) {
                        // Run second player
                        state.old_player = state.player2;
                        state.current_player = 1;
                        trail = &trail_p2;
                        wave_trail = &wave_trail_p2;
                        handle_player(&state.player2);

                        if (state.dead) break;
                    }
                    
                    run_camera();

                    u64 end_physics = svcGetSystemTick();
                    float physics_time = (end_physics - start_physics) / (CPU_TICKS_PER_MSEC);

                    if (physics_time / 1000 >= STEPS_DT_UNMOD) {
                        frame_skipped = (int) ((physics_time / 1000) * STEPS_HZ);
                    }
                    else frame_skipped = 0;

                    physics_calc_time += physics_time;

                    accumulator -= STEPS_DT;
                    steps++;
                    level_frame++;
                }
            }
        }

        if (!game_paused) {
            frame_counter++;

            if (state.dead && state.death_timer <= 0.f) {
                state.death_timer = 1.f;
                handle_death();
            }

            if (state.death_timer > 0.f) {
                state.death_timer -= DT;

                fade_to_amplitude(0);

                if (state.player.gamemode == GAMEMODE_DART) {
                    if (wave_trail_p1.opacity > 0) wave_trail_p1.opacity -= 0.02f * 4;
            
                    if (wave_trail_p1.opacity <= 0) {
                        wave_trail_p1.opacity = 0;
                        wave_trail_p1.nuPoints = 0;
                    }
                }

                if (state.player2.gamemode == GAMEMODE_DART) {
                    if (wave_trail_p2.opacity > 0) wave_trail_p2.opacity -= 0.02f * 4;

                    if (wave_trail_p2.opacity <= 0) {
                        wave_trail_p2.opacity = 0;
                        wave_trail_p2.nuPoints = 0;
                    }
                }

                if (state.death_timer <= 0.f) {
                    init_variables();
                    reload_level(); 
                    if (song_loaded) unpause_playback_mp3();
                    fixed_dt = true; 
                    state.dead = false;
                }
            }

            u64 start_trig = svcGetSystemTick();
            handle_triggers();
            handle_col_triggers();
            calculate_lbg();
            u64 end_trig = svcGetSystemTick();
            u64 ticks_trig = end_trig - start_trig;
            triggers_time = ticks_trig / CPU_TICKS_PER_MSEC;

            u64 start_obj = svcGetSystemTick();
            create_objects();
            u64 end_obj = svcGetSystemTick();
            u64 ticks_obj = end_obj - start_obj;
            sprite_drawing_time = ticks_obj / CPU_TICKS_PER_MSEC;

            u64 start_part = svcGetSystemTick();
            for (int i = 0; i < 2; i++) {
                updateParticleSystem(&drag_particles[i], delta);
                updateParticleSystem(&drag_particles_2[i], delta);
                updateParticleSystem(&ship_fire_particles[i], delta);
                updateParticleSystem(&ship_secondary_particles[i], delta);
                updateParticleSystem(&secondary_particles[i], delta);
                updateParticleSystem(&burst_particles[i], delta);
                updateParticleSystem(&land_particles[i], delta);
                updateParticleSystem(&explosion_particles[i], delta);
            }
            updateParticleSystem(&brick_destroy_particles, delta);
            updateParticleSystem(&glitter_particles, delta);
            updateParticleSystem(&slow_speed_particles, delta);
            updateParticleSystem(&normal_speed_particles, delta);
            updateParticleSystem(&fast_speed_particles, delta);
            updateParticleSystem(&faster_speed_particles, delta);
            updateParticleSystem(&coin_pickup_particles, delta);
            float calc_x_speed_particles = SCREEN_WIDTH_AREA;
            float calc_y_speed_particles = (SCREEN_HEIGHT_AREA / 2);

            slow_speed_particles.emitterX = calc_x_speed_particles;
            slow_speed_particles.emitterY = calc_y_speed_particles;
            slow_speed_particles.emitting = slow_speed_particles_timer > 0;
            if (slow_speed_particles_timer > 0) {
                slow_speed_particles_timer -= delta;
            }

            normal_speed_particles.emitterX = calc_x_speed_particles;
            normal_speed_particles.emitterY = calc_y_speed_particles;
            normal_speed_particles.emitting = normal_speed_particles_timer > 0;
            if (normal_speed_particles_timer > 0) {
                normal_speed_particles_timer -= delta;
            }

            fast_speed_particles.emitterX = calc_x_speed_particles;
            fast_speed_particles.emitterY = calc_y_speed_particles;
            fast_speed_particles.emitting = fast_speed_particles_timer > 0;
            if (fast_speed_particles_timer > 0) {
                fast_speed_particles_timer -= delta;
            }

            faster_speed_particles.emitterX = calc_x_speed_particles;
            faster_speed_particles.emitterY = calc_y_speed_particles;
            faster_speed_particles.emitting = faster_speed_particles_timer > 0;
            if (faster_speed_particles_timer > 0) {
                faster_speed_particles_timer -= delta;
            }
            

            update_use_effects(delta, GFX_TOP);
            update_object_particles();
            u64 end_part = svcGetSystemTick();
            u64 ticks_part = end_part - start_part;
            particle_calc_time = ticks_part / CPU_TICKS_PER_MSEC;

            // Update trails
            MotionTrail_Update(&trail_p1, delta);
            MotionTrail_UpdateWaveTrail(&wave_trail_p1, delta);
            update_p1_trail(&state.player);

            if (state.dual) {
                MotionTrail_Update(&trail_p2, delta);
                MotionTrail_UpdateWaveTrail(&wave_trail_p2, delta);
                update_p1_trail(&state.player2);
            }
        }
        
        if (wideEnabled != old_wide) {        
            gspWaitForVBlank();
            set_wide(wideEnabled);
            gspWaitForVBlank();
            reinitialize_screens();
            old_wide = wideEnabled;
        }
        
        u64 end = svcGetSystemTick();
        u64 ticks = end - start;

        // Render the scene
        do {
            update_touch_effect(delta);

            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
            draw_fade();

            // Top screen
            C2D_SceneBegin(top);
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            
            draw_background(state.background_x / 8, -(state.camera_y / 8) + 200);

            C2D_ViewScale(SCALE, SCALE);

            draw_objects();

            draw_ground(state.ground_x, state.camera_y, 0, false, SCREEN_WIDTH);
            
            if (state.ground_y_gfx > 2) {
                if (state.camera_y - CAMERA_Y_OFFSET + state.ground_y_gfx > 0) draw_ground(state.ground_x, state.camera_y, state.camera_y + state.ground_y_gfx - CAMERA_Y_OFFSET, false, SCREEN_WIDTH);
                draw_ground(state.ground_x, state.camera_y, state.camera_y - CAMERA_Y_OFFSET + SCREEN_HEIGHT_AREA - state.ground_y_gfx, true, SCREEN_WIDTH);
            }
            C2D_ViewScale(1/SCALE, 1/SCALE);
            gameplay_screen_top_loop();

            // Bottom screen
            C2D_SceneBegin(bot);
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));

            gameplay_screen_bot_loop();

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            if (state.profiling) {
                float processingTime = ((ticks / CPU_TICKS_PER_MSEC)) * 6;
                float drawingTime = C3D_GetDrawingTime() * 6;

                #define DEBUG_TEXT_SCALE 0.4f
                
                draw_text(bigFont_fontCharset, bigFont_sheet, 0, 6,  DEBUG_TEXT_SCALE, 0, "CPU: %6.2f%% (%6.2f%% %6.2f%%)", (C3D_GetProcessingTime() * 6) + processingTime, C3D_GetProcessingTime() * 6, processingTime);
                draw_text(bigFont_fontCharset, bigFont_sheet, 0, 18, DEBUG_TEXT_SCALE, 0, "GPU: %6.2f%%", drawingTime);
                draw_text(bigFont_fontCharset, bigFont_sheet, 0, 30, DEBUG_TEXT_SCALE, 0, "Linear free: %d", linearSpaceFree());
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 30, DEBUG_TEXT_SCALE, 0, "CMDBuf: %6.2f%%", C3D_GetCmdBufUsage()*100.0f);

                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 42,  DEBUG_TEXT_SCALE, 0, "%d steps", steps);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 54,  DEBUG_TEXT_SCALE, 0, "Particle: %6.2f%%", particle_calc_time * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 66,  DEBUG_TEXT_SCALE, 0, "Triggers: %6.2f%%", triggers_time * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 78,  DEBUG_TEXT_SCALE, 0, "Collision %d/%d", number_of_collisions, number_of_collisions_checks);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 90,  DEBUG_TEXT_SCALE, 0, "Physics: %6.2f%%", physics_calc_time * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 102, DEBUG_TEXT_SCALE, 0, " - Coll: %6.2f%%", collision_time * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 114, DEBUG_TEXT_SCALE, 0, " - Play: %6.2f%%", player_time * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 180, 126, DEBUG_TEXT_SCALE, 0, " - Hndl: %6.2f%%", handle_player_time * 6);

                draw_text(bigFont_fontCharset, bigFont_sheet, 0,   42,  DEBUG_TEXT_SCALE, 0, "SprDraw:  %6.2f%%", (sprite_drawing_time) * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 0,   54,  DEBUG_TEXT_SCALE, 0, " - Creating: %6.2f%%", (object_creating_time) * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 0,   66,  DEBUG_TEXT_SCALE, 0, " - Sorting:  %6.2f%%", (object_sorting_time) * 6);
                draw_text(bigFont_fontCharset, bigFont_sheet, 0,   90,  DEBUG_TEXT_SCALE, 0, "Drawing:  %6.2f%%", (object_drawing_time) * 6);
            }

            if (state.noclip) {
                draw_text(bigFont_fontCharset, bigFont_sheet, 0, 234, 0.5f, 0, "Noclip Activated");
            }
            C2D_ViewReset();

            C3D_FrameEnd(0);
        } while (handle_fading());

        if (being_faded) {
            if (song_loaded) unpause_playback_mp3();
            being_faded = false;
        }

        if (exiting_level) {
            game_paused = false;
            if (song_loaded) unpause_playback_mp3();
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        freeParticleData(&drag_particles[i].data);
        freeParticleData(&drag_particles_2[i].data);
        freeParticleData(&ship_fire_particles[i].data);
        freeParticleData(&secondary_particles[i].data);
        freeParticleData(&ship_secondary_particles[i].data);
        freeParticleData(&burst_particles[i].data);
        freeParticleData(&land_particles[i].data);
        freeParticleData(&explosion_particles[i].data);
    }

    freeParticleData(&brick_destroy_particles.data);
    freeParticleData(&glitter_particles.data);
    freeParticleData(&slow_speed_particles.data);
    freeParticleData(&normal_speed_particles.data);
    freeParticleData(&fast_speed_particles.data);
    freeParticleData(&faster_speed_particles.data);
    freeParticleData(&coin_pickup_particles.data);

    unload_level();

    game_state = (state.custom_level ? STATE_EXTERNAL_LEVELS : STATE_LEVEL_SELECT);
}

void game_assets_init() {
    bgSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sheet_01.t3x");
    if (!bgSheet) svcBreak(USERBREAK_PANIC);
    
    bg2Sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sheet_02.t3x");
    if (!bg2Sheet) svcBreak(USERBREAK_PANIC);
    
    groundSheet = C2D_SpriteSheetLoad("romfs:/gfx/grounds.t3x");
    if (!groundSheet) svcBreak(USERBREAK_PANIC);
    
    // Load graphics
    spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    if (!spriteSheet) svcBreak(USERBREAK_PANIC);
    
    spriteSheet2 = C2D_SpriteSheetLoad("romfs:/gfx/portals.t3x");
    if (!spriteSheet2) svcBreak(USERBREAK_PANIC);
    
    glowSheet = C2D_SpriteSheetLoad("romfs:/gfx/glow.t3x");
    if (!glowSheet) svcBreak(USERBREAK_PANIC);
    
    iconSheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
    if (!iconSheet) svcBreak(USERBREAK_PANIC);

    trailSheet = C2D_SpriteSheetLoad("romfs:/gfx/trails.t3x");
    if (!trailSheet) svcBreak(USERBREAK_PANIC);

    initParticleSystem(&touch_drag_particles, &touch_drag_effect);
    touch_drag_particles.relativeStationary = true;

    initParticleSystem(&touch_explosion_particles, &touch_explosion_effect);
    touch_explosion_particles.relativeStationary = true;
}

void load_sfx() {
    load_wav("romfs:/sfx/playSound_01.wav", &play_sound);
    load_wav("romfs:/sfx/quitSound_01.wav", &quit_sound);
    load_wav("romfs:/sfx/explode_11.wav", &explode_sound);
}

int main(int argc, char* argv[]) {
    // Init libs
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 4);
    C2D_Init(MAX_SPRITES);
    C2D_Prepare();
    osSetSpeedupEnable(1);

    cfg_init();
    
    C2D_SetTintMode(C2D_TintMult);
    
    if(ndspInit()) {
        no_dsp_firmware();
    }

    ui_assets_init();
    game_assets_init();

    cache_all_sprites();
    update_player_colors();

    make_opacity_lut();

    load_sfx();

    srand(time(NULL));

    alt_title_screen = (rand() & (128 - 1)) == 0;

    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, false);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, false);

    memset(&level_info, 0, sizeof(LoadedLevelInfo));

    bool exit = false;
    while (aptMainLoop() && !exit) {
        // Update color if changed menus
        Color p1_not_white = get_white_if_black(p1_color);

        touch_drag_particles.cfg.startColorRed   = p1_not_white.r / 255.f;
        touch_drag_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
        touch_drag_particles.cfg.startColorBlue  = p1_not_white.b / 255.f;

        touch_explosion_particles.cfg.startColorRed   = p1_not_white.r / 255.f;
        touch_explosion_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
        touch_explosion_particles.cfg.startColorBlue  = p1_not_white.b / 255.f;

        switch (game_state) {
            case STATE_MAIN_MENU:
                main_menu_loop();
                break;
            case STATE_LEVEL_SELECT:
                level_select_loop();
                break;
            case STATE_ICON_KIT:
                icon_kit_loop();
                break;
            case STATE_GAME:
                game_loop();
                break;
            case STATE_EXTERNAL_LEVELS:
                external_levels_loop();
                break;
            case STATE_EXIT:
                exit = true;
                break;
        }
    }

    close_log_file();

    free_cached_sprites();

    // Delete graphics
    C2D_SpriteSheetFree(spriteSheet);
    C2D_SpriteSheetFree(spriteSheet2);
    C2D_SpriteSheetFree(glowSheet);
    C2D_SpriteSheetFree(bgSheet);
    C2D_SpriteSheetFree(bg2Sheet);
    C2D_SpriteSheetFree(iconSheet);
    C2D_SpriteSheetFree(ui_sheet);
    C2D_SpriteSheetFree(ui_2_sheet);
    C2D_SpriteSheetFree(groundSheet);
    C2D_SpriteSheetFree(bigFont_sheet);
    C2D_SpriteSheetFree(window_sheet);
    C2D_SpriteSheetFree(bar_sheet);

    cfg_fini();

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    ndspExit();
    return 0;
}