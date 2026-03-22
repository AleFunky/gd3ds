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
#include "menus/components/ui_screen.h"

#include "player/collision.h"
#include "state.h"

#include "particles/particles.h"
#include "particles/object_particles.h"

#include <stdarg.h>

#include "player/player.h"

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

int game_state = STATE_MAIN_MENU;

bool playing_menu_loop = false;

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* bot;

bool is_citra() {
    s64 version = 0;
    svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
    return version != 0;
}

void no_dsp_firmware(void) {
    consoleInit(GFX_TOP, NULL);
    printf("\x1b[01;00H/////////////FATAL///ERROR////////////////////////");
    printf("\x1b[03;00HNDSP could not be initalized.");
    printf("\x1b[05;00HThis is probably because your dspfirm is missing.");
    printf("\x1b[07;00HPut the ndsp firmwared called dspfirm.cdc");
    printf("\x1b[09;00Hon your sd card in sdmc:/3ds");
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

void game_loop() {
    int returned = load_level(main_levels[curr_level_id].gmd_path);
    if (returned) {
        printf("\x1b[9;1HFailed %d", returned);
        game_state = STATE_LEVEL_SELECT;
        return;
    }

    returned = play_mp3(main_levels[curr_level_id].song_path, false);
    toggle_playback_mp3();

    state.camera_x = 0;
    state.camera_y = 0;
    current_fading_effect = FADE_NONE;

    set_fade_status(FADE_STATUS_IN);

    bool being_faded = true;
    bool exiting = false;

    init_variables();

    init_op_system();
    initParticleSystem(&drag_particles, &drag_effect);

    drag_particles.cfg.startColorRed   = get_white_if_black(p1_color).r / 255.f;
    drag_particles.cfg.startColorGreen = get_white_if_black(p1_color).g / 255.f;
    drag_particles.cfg.startColorBlue  = get_white_if_black(p1_color).b / 255.f;

    float accumulator = 0.0f;
    u64 lastTime = svcGetSystemTick();
    u64 start = svcGetSystemTick();
    bool fixed_dt = true;

    // Main loop
    while (aptMainLoop()) {
        start = svcGetSystemTick();
        hidScanInput();

        // Respond to user input
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            exiting = true;
            set_fade_status(FADE_STATUS_OUT);
        }
            
        if (kDown & KEY_X)
            state.noclip ^= 1;

        int steps = 0;

        u32 kHeld = hidKeysHeld();
        // Compare with true to store it in a single bit
        state.input.pressedJump = ((kDown & KEY_A) || (kDown & KEY_TOUCH)) == true;
        state.input.holdJump = (state.input.pressedJump || (kHeld & KEY_A) || (kHeld & KEY_TOUCH)) == true;
        if (state.death_timer <= 0 && !(kHeld & KEY_Y))  {
            physics_calc_time = 0;
            u64 now = svcGetSystemTick();
            delta = (now - lastTime) / (CPU_TICKS_PER_MSEC * 1000);
            lastTime = now;
            if (delta > 0.5f) delta = STEPS_DT_UNMOD; // Avoid spiral of death
            if (fixed_dt) {
                delta = STEPS_DT_UNMOD;
                if (!being_faded) fixed_dt = false;
            }

            accumulator += delta;

            
            // Run simulation in fixed steps
            while (accumulator >= STEPS_DT_UNMOD) {
                u64 start_physics = svcGetSystemTick();
                state.current_player = 0;
                state.old_player = state.player;
                
                trail = trail_p1;
                wave_trail = wave_trail_p1;
                handle_player(&state.player);
                handle_mirror_transition();
                trail_p1 = trail;
                wave_trail_p1 = wave_trail;

                if (state.dead) break;

                if (state.dual) {
                    // Run second player
                    state.old_player = state.player2;
                    state.current_player = 1;
                    trail = trail_p2;
                    wave_trail = wave_trail_p2;
                    handle_player(&state.player2);
                    trail_p2 = trail;
                    wave_trail_p2 = wave_trail;

                    if (state.dead) break;
                }
                
                run_camera();

                u64 end_physics = svcGetSystemTick();
                float physics_time = (end_physics - start_physics) / (CPU_TICKS_PER_MSEC);

                if (physics_time / 1000 >= STEPS_DT_UNMOD) {
                    frame_skipped = (int) (physics_time / 1000 * STEPS_HZ);
                }
                else frame_skipped = 0;

                physics_calc_time += physics_time;

                accumulator -= STEPS_DT_UNMOD;
                steps++;
                level_frame++;
            }
        }
        
        frame_counter++;

        if (state.dead && state.death_timer <= 0.f) {
            state.death_timer = 1.f;
            handle_death();
            state.dead = false;
        }

        if (state.death_timer > 0.f) {
            state.death_timer -= DT;

            if (state.death_timer <= 0.f) {
                init_variables();
                reload_level(); 
                toggle_playback_mp3();
                fixed_dt = true; 
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
        updateParticleSystem(&drag_particles, DT);
        update_object_particles();
        u64 end_part = svcGetSystemTick();
        u64 ticks_part = end_part - start_part;
        particle_calc_time = ticks_part / CPU_TICKS_PER_MSEC;
        
        u64 end = svcGetSystemTick();
        u64 ticks = end - start;


        // Render the scene
        do {
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

            // Bottom screen
            C2D_SceneBegin(bot);
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));

            float processingTime = ((ticks / CPU_TICKS_PER_MSEC)) * 6;
            float drawingTime = C3D_GetDrawingTime() * 6;
            
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 6, 0.5f, 0, "CPU: %6.2f%% (%6.2f%% %6.2f%%)", (C3D_GetProcessingTime() * 6) + processingTime, C3D_GetProcessingTime() * 6, processingTime);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 18, 0.5f, 0, "GPU: %6.2f%%", drawingTime);

            draw_text(bigFont_fontCharset, bigFont_sheet, 320-8, 42, 0.5f, 1.0, "%d steps", steps);
            draw_text(bigFont_fontCharset, bigFont_sheet, 320-32, 54, 0.5f, 1.0, "%.2f%% Physics", physics_calc_time * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 320-32, 66, 0.5f, 1.0, "%.2f%% Particle", particle_calc_time * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 320-32, 78, 0.5f, 1.0, "%.2f%% Triggers", triggers_time * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 320-32, 90, 0.5f, 1.0, "%d/%d Collision", number_of_collisions, number_of_collisions_checks);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 42, 0.5f, 0, "SprDraw:  %6.2f%%", (sprite_drawing_time) * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 54, 0.5f, 0, " - Creating: %6.2f%%", (object_creating_time) * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 66, 0.5f, 0, " - Sorting:  %6.2f%%", (object_sorting_time) * 6);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 90, 0.5f, 0, "Drawing:  %6.2f%%", (object_drawing_time) * 6);

            if (state.noclip) {
                draw_text(bigFont_fontCharset, bigFont_sheet, 0, 234, 0.5f, 0, "Noclip Activated");
            }
            C2D_ViewReset();

            C3D_FrameEnd(0);
        } while (handle_fading());

        if (being_faded) {
            toggle_playback_mp3();
            being_faded = false;
        }

        if (exiting) {
            // If death timer is set, the music is muted, unmute it
            if (state.death_timer > 0) {
                toggle_playback_mp3();
            }
            break;
        }
    }

    freeParticleData(&drag_particles.data);

    unload_level();

    game_state = STATE_LEVEL_SELECT;
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
}


int main(int argc, char* argv[]) {
    // Init libs
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
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

    srand(time(NULL));

    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, false);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, false);

    memset(&level_info, 0, sizeof(LoadedLevelInfo));

    bool exit = false;
    while (aptMainLoop() && !exit) {
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

    cfg_fini();

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    ndspExit();
    return 0;
}