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
#include "menus/soggy.h"
#include "menus/core/ui_screen.h"

#include "player/collision.h"
#include "state.h"

#include "particles/particles.h"
#include "particles/object_particles.h"
#include "particles/coin_effect.h"

#include <stdarg.h>

#include "player/player.h"
#include "particles/circles.h"
#include "menus/settings.h"
#include "menus/creator_menu.h"
#include "menus/external_levels.h"
#include "menus/search_menu.h"
#include "menus/saved_levels.h"
#include "menus/loading_screen.h"
#include "menus/level_complete.h"
#include "menus/online_level_menu.h"

#include "save/saving.h"

#include "endwall.h"
#include "practice.h"

#include "new_best.h"

#include "math_helpers.h"

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

int game_state = STATE_MAIN_MENU;

bool playing_menu_loop = false;

int level_result = 0;

bool cheated = false;

float global_volume;
float music_volume;
float sound_volume;

bool cheats_used[CHEAT_COUNT];

const char *cheat_names[CHEAT_COUNT] = {
    "Noclip",
    "Hitboxes"
};

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* bot;

SFX play_sound;
SFX quit_sound;
SFX explode_sound;
SFX end_sound;
SFX honk;
SFX coin_sound;

ParticleSystem touch_drag_particles;
ParticleSystem touch_explosion_particles;
ParticleSystem glitter_particles_bottom;
ParticleSystem slow_speed_particles_bottom;
ParticleSystem normal_speed_particles_bottom;
ParticleSystem fast_speed_particles_bottom;
ParticleSystem faster_speed_particles_bottom;
ParticleSystem end_wall_particles;
ParticleSystem end_wall_firework;
ParticleSystem level_complete_effect_p1;
ParticleSystem level_complete_effect_p2;

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
    printf("\x1b[01;00H///////////////////FATAL///ERROR//////////////////");
    printf("\x1b[03;00HNDSP could not be initalized!");
    printf("\x1b[05;00HThis is probably because your dspfirm is missing.");
    printf("\x1b[07;00HFor 3DS users, open the Luma menu, go to");
    printf("\x1b[08;00Hmiscellaneous options and press \"Dump DSP");
    printf("\x1b[09;00Hfirmware\".");
    printf("\x1b[11;00HFor Citra/Azahar users, open your emulator folder");
    printf("\x1b[12;00Hand create a file named \"dspfirm.cdc\" in ");
    printf("\x1b[13;00H/sdmc/3ds/. You only need the file to be there, ");
    printf("\x1b[14;00Hit can be empty for all the emulator cares.");
    printf("\x1b[16;00HFor more information, check the GitHub.");
    printf("\x1b[18;00HPress start to exit.");
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
    
    log_file = fopen(CONFIG_ROOT "output.txt", "w");
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

#define MAX_AUDIO_DB 9

float get_volume_slider() {
    u8 slider = 63;
    HIDUSER_GetSoundVolume(&slider);

    return slider / 63.f;
}

float slider_to_volume(float v) {
    return expm1f(v * log1pf(MAX_AUDIO_DB)) / MAX_AUDIO_DB;
}

void set_channel_volume(int channel, float volume) {
    float mix[12] = {0};

    float gain = slider_to_volume(volume);

    mix[0] = gain;
    mix[1] = gain;

    ndspChnSetMix(channel, mix);
}

void apply_volume_settings() {
    set_channel_volume(0, music_volume);

    for (int i = 1; i <= 7; i++) {
        set_channel_volume(i, sound_volume);
    }
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

void update_player_effects(float delta) {
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
}

void allocate_particles() {
    initParticleSystem(&drag_particles[0], &drag_effect);
    initParticleSystem(&drag_particles[1], &drag_effect);

    initParticleSystem(&drag_particles_2[0], &ship_drag_effect);
    initParticleSystem(&drag_particles_2[1], &ship_drag_effect);

    initParticleSystem(&ship_fire_particles[0], &drag_effect);
    initParticleSystem(&ship_fire_particles[1], &drag_effect);

    initParticleSystem(&secondary_particles[0], &drag_effect);
    initParticleSystem(&secondary_particles[1], &drag_effect);
    
    initParticleSystem(&ship_secondary_particles[0], &drag_effect);
    initParticleSystem(&ship_secondary_particles[1], &drag_effect);

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

    initParticleSystem(&end_wall_particles, &end_effect_portal);
    initParticleSystem(&end_wall_firework, &firework);
    initParticleSystem(&level_complete_effect_p1, &level_complete_01);
    initParticleSystem(&level_complete_effect_p2, &level_complete_01);
}

void free_particles() {
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

    freeParticleData(&end_wall_particles.data);
    freeParticleData(&end_wall_firework.data);
    freeParticleData(&level_complete_effect_p1.data);
    freeParticleData(&level_complete_effect_p2.data);
}

void init_particles(Color p1_color, Color p2_color) {    
    slow_speed_particles.stationary = true;
    normal_speed_particles.stationary = true;
    fast_speed_particles.stationary = true;
    faster_speed_particles.stationary = true;

    slow_speed_particles.affectedByMirror = false;
    normal_speed_particles.affectedByMirror = false;
    fast_speed_particles.affectedByMirror = false;
    faster_speed_particles.affectedByMirror = false;

    end_wall_firework.affectedByMirror = false;
    
    level_complete_effect_p1.stationary = true;
    level_complete_effect_p2.stationary = true;

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
    secondary_particles[0].cfg.finishColorAlpha = 1.f;
    secondary_particles[0].cfg.maxParticles = 15;
    secondary_particles[0].cfg.speed = 75 - 30;

    secondary_particles[1].cfg.startColorRed   = p1_not_white.r / 255.f;
    secondary_particles[1].cfg.startColorGreen = p1_not_white.g / 255.f;
    secondary_particles[1].cfg.startColorBlue  = p1_not_white.b / 255.f;
    secondary_particles[1].cfg.finishColorAlpha = 1.f;
    secondary_particles[1].cfg.maxParticles = 15;
    secondary_particles[1].cfg.speed = 75 - 30;

    ship_secondary_particles[0].cfg.startColorRed   = 255.f/ 255.f;
    ship_secondary_particles[0].cfg.startColorGreen = 70.f / 255.f;
    ship_secondary_particles[0].cfg.startColorBlue  = 10.f / 255.f;

    ship_secondary_particles[0].cfg.finishColorRed   = 255.f/ 255.f;
    ship_secondary_particles[0].cfg.finishColorGreen = 20.f / 255.f;
    ship_secondary_particles[0].cfg.finishColorBlue  = 0.f / 255.f;

    ship_secondary_particles[1].cfg = ship_secondary_particles[0].cfg;

    ship_fire_particles[0].cfg.startColorRed   = 255.f / 255.f;
    ship_fire_particles[0].cfg.startColorGreen = 170.f / 255.f;
    ship_fire_particles[0].cfg.startColorBlue  = 10.f / 255.f;
    
    ship_fire_particles[0].cfg.finishColorRed   = 255.f / 255.f;
    ship_fire_particles[0].cfg.finishColorGreen = 65.f / 255.f;
    ship_fire_particles[0].cfg.finishColorBlue  = 0.f / 255.f;

    ship_fire_particles[0].cfg.speed = 75 - 30;
    ship_fire_particles[0].cfg.maxRadius = 130;
    ship_fire_particles[1].cfg = ship_fire_particles[0].cfg;

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
    
    end_wall_particles.cfg.startColorRed   = p1_not_white.r / 255.f;
    end_wall_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
    end_wall_particles.cfg.startColorBlue  = p1_not_white.b / 255.f;
    
    end_wall_particles.cfg.finishColorRed   = p1_not_white.r / 255.f;
    end_wall_particles.cfg.finishColorGreen = p1_not_white.g / 255.f;
    end_wall_particles.cfg.finishColorBlue  = p1_not_white.b / 255.f;

    end_wall_particles.scale = 1.5f;

    end_wall_firework.cfg.startColorRed   = p1_not_white.r / 255.f;
    end_wall_firework.cfg.startColorGreen = p1_not_white.g / 255.f;
    end_wall_firework.cfg.startColorBlue  = p1_not_white.b / 255.f;
    
    end_wall_firework.cfg.finishColorRed   = p1_not_white.r / 255.f;
    end_wall_firework.cfg.finishColorGreen = p1_not_white.g / 255.f;
    end_wall_firework.cfg.finishColorBlue  = p1_not_white.b / 255.f;

    
    level_complete_effect_p1.cfg.startColorRed   = p1_not_white.r / 255.f;
    level_complete_effect_p1.cfg.startColorGreen = p1_not_white.g / 255.f;
    level_complete_effect_p1.cfg.startColorBlue  = p1_not_white.b / 255.f;

    level_complete_effect_p1.cfg.finishColorRed   = p1_not_white.r / 255.f;
    level_complete_effect_p1.cfg.finishColorGreen = p1_not_white.g / 255.f;
    level_complete_effect_p1.cfg.finishColorBlue  = p1_not_white.b / 255.f;
    
    level_complete_effect_p2.cfg.startColorRed   = p2_not_white.r / 255.f;
    level_complete_effect_p2.cfg.startColorGreen = p2_not_white.g / 255.f;
    level_complete_effect_p2.cfg.startColorBlue  = p2_not_white.b / 255.f;

    level_complete_effect_p2.cfg.finishColorRed   = p2_not_white.r / 255.f;
    level_complete_effect_p2.cfg.finishColorGreen = p2_not_white.g / 255.f;
    level_complete_effect_p2.cfg.finishColorBlue  = p2_not_white.b / 255.f;
}

void game_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));

    C2D_Fade(0);
    draw_text(&bigFont_fontCharset, &bigFont_sheet, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 0.5f, 0.5f, 1.0f, true, "Loading...");
    C3D_FrameEnd(0);
    
    char *path;

    if (state.custom_level) {
        path = state.custom_level_path;
    } else {
        path = main_levels[curr_level_id].gmd_path;
    }

    update_player_colors();

    int returned = load_level(path);
    level_result = returned;
    if (returned) {
        printf("\x1b[9;1HFailed %d", returned);

        game_state = (state.custom_level ? STATE_EXTERNAL_LEVELS : STATE_LEVEL_SELECT);
        return;
    }

    if (!state.custom_level) {
        level_info.level_name = main_levels[curr_level_id].level_name;
    }

    play_level_song();

    if (song_loaded) pause_playback_mp3();

    set_fade_status(FADE_STATUS_IN);

    bool being_faded = true;

    first_load_init_variables();

    init_op_system();

    gameplay_screen_init();
    
    // Particle
    allocate_particles();
    init_particles(p1_color, p2_color);
    clear_practice_mode();

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
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();
        static u32 kHeldPaused;

        state.hitbox_display = 0;

        if (kDown & KEY_X && enableDebugBindings) {
            state.noclip ^= 1;
        }

        if (state.noclip) {
            cheated = true;
            cheats_used[CHEAT_NOCLIP] = true;
        }

        if ((kDown & KEY_L) && (kHeld & KEY_B) && enableDebugBindings)
            state.profiling ^= 1;

        if ((kDown & KEY_R) && (kHeld & KEY_B) && enableDebugBindings) {
            cheated = true;
            cheats_used[CHEAT_HITBOX_DISPLAY] = true;
            if (hitboxesEnabled && hitboxTrail) {
                hitboxesEnabled = false;
                hitboxTrail = false;
            } else if (hitboxesEnabled && !hitboxTrail) {
                hitboxTrail = true;
            } else hitboxesEnabled = true;
        }      

        if (hitboxesEnabled || state.hitbox_enabled_when_dead) {
            state.hitbox_display = 1;
        }
        
        if (hitboxTrail) {
            hitboxesEnabled = true;
            state.hitbox_display = 2;
        }

        if (hitboxesEnabled || hitboxTrail) {
            cheated = true;
            cheats_used[CHEAT_HITBOX_DISPLAY] = true;
        }
        
        int steps = 0;

        bool in_bounds = !((touchPos.px > 320 - 30 && touchPos.py < 30) || (state.practice_mode && (touchPos.px > 92 && touchPos.px < 222 && touchPos.py > 175 && touchPos.py < 222)));
        
        kHeldPaused &= ~kUp;
        if(!game_paused){
            kHeld &= ~kHeldPaused;
        }
        
        global_volume = get_volume_slider();

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

        u64 now = svcGetSystemTick();
        delta = (now - lastTime) / (CPU_TICKS_PER_MSEC * 1000);
        lastTime = now;

        if (state.death_timer <= 0)  {
            float physics_delta = delta;
            physics_calc_time = 0;
            number_of_collisions = 0;
            number_of_collisions_checks = 0;
            collision_time = 0;
            player_time = 0;
            handle_player_time = 0;
            
            brick_destroy_particles.emitting = false;
            glitter_particles.emitting = false;
            glitter_particles_bottom.emitting = false;
            slow_speed_particles_bottom.emitting = false;
            normal_speed_particles_bottom.emitting = false;
            fast_speed_particles_bottom.emitting = false;
            faster_speed_particles_bottom.emitting = false;
            
            if (physics_delta > 0.5f) physics_delta = STEPS_DT_UNMOD; // Avoid spiral of death
            if (fixed_dt) {
                physics_delta = STEPS_DT_UNMOD;
                if (!being_faded) fixed_dt = false;
            }

            if (!game_paused) {
                accumulator += physics_delta;
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
                    if (state.level_progress < 0) {
                        state.level_progress = 0;
                    }
                    
                    if (state.level_progress > 100) {
                        state.level_progress = 100;
                    }

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
                    handle_bg_flash();
                    handle_respawn_effect();

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
                state.death_timer = (quickRetry ? 0.5f : 1.f);
                bool had_new_best = false;
                if (!cheated) {
                    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);
                    // Save new best
                    int progress = (int)state.level_progress;
                    if (state.practice_mode) {
                        if (state.current_data.max_practice < progress) {
                            state.current_data.max_practice = progress;
                            level_data_sel->practice_progress = progress;
                        }
                    } else {
                        if (state.current_data.max_normal < progress) {
                            init_new_best_popup(progress);
                            had_new_best = true;
                            state.current_data.max_normal = progress;
                            level_data_sel->normal_progress = progress;
                        }
                    }
                }

                // New best makes the respawn timer a bit longer
                if (had_new_best) state.death_timer = 1.4f;

                handle_death((state.current_player == 1) ? &state.player2 : &state.player, true);
            }

            if (state.death_timer > 0.f) {
                state.death_timer -= delta;

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
                
                handle_mirror_transition();

                if (state.death_timer <= 0.f) {
                    init_variables();
                    reload_level(); 
                    //resets UI coins on gameplay screen
                    reset_coins();

                    if (state.practice_mode && checkpoint_count > 0) {
                        restore_checkpoint();
                    }

                    if (song_loaded) unpause_playback_mp3();
                    fixed_dt = true; 
                    state.dead = false;
                    state.hitbox_display = 0;
                }
            }

            handle_practice_mode();

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
            update_player_effects(delta);

            // End wall particles
            if (level_info.wall_y > 0) {
                updateParticleSystem(&end_wall_particles, delta);
                updateParticleSystem(&end_wall_firework, delta);
                updateParticleSystem(&level_complete_effect_p1, delta);
                updateParticleSystem(&level_complete_effect_p2, delta);

                handle_level_complete_popup(delta);
            }

            handle_new_best_popup(delta);

            update_collect_effect(delta);

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
            
            end_wall_particles.emitterX = level_info.wall_x - 15;
            end_wall_particles.emitterY = level_info.wall_y;
            end_wall_particles.emitting = level_info.wall_y > 0;

            update_use_effects(delta, get_use_effect_array_ptr(GFX_TOP));
            update_use_effects(delta, get_use_effect_array_ptr(GFX_TOP_BUT_ABOVE_LEVEL));
            update_object_particles(delta);
            u64 end_part = svcGetSystemTick();
            u64 ticks_part = end_part - start_part;
            particle_calc_time = ticks_part / CPU_TICKS_PER_MSEC;

            // Update trails
            MotionTrail_Update(&trail_p1, delta);
            MotionTrail_UpdateWaveTrail(&wave_trail_p1, delta);
            update_p1_trail(&state.player, 0);

            MotionTrail_Update(&trail_p2, delta);
            MotionTrail_UpdateWaveTrail(&wave_trail_p2, delta);
            update_p1_trail(&state.player2, 1);
            
            // If not dual, fade the p2 wave trial
            if (!state.dual) {
                if (wave_trail_p2.opacity > 0) wave_trail_p2.opacity -= (0.02f * STEPS_HZ) * delta;
                
                if (wave_trail_p2.opacity <= 0) {
                    wave_trail_p2.opacity = 0;
                    wave_trail_p2.nuPoints = 0;
                }
            }

            if (state.mirroring) {
                state.mirror_timer += delta;
                if (state.mirror_timer > MIRROR_DURATION) {
                    state.mirroring = false;
                    state.mirror_factor = state.intended_mirror_factor;
                    state.mirror_speed_factor = 1 - 2*state.mirror_factor;

                    // When mirror transition ends, put a wave trail point
                    if (state.player.gamemode == GAMEMODE_DART) {
                        wave_trail_p1.positionR = (Vec2D){ state.player.x, state.player.y };
                        MotionTrail_AddWavePoint(&wave_trail_p1);
                    }
                    if (state.dual && state.player2.gamemode == GAMEMODE_DART) {
                        wave_trail_p2.positionR = (Vec2D){ state.player2.x, state.player2.y };
                        MotionTrail_AddWavePoint(&wave_trail_p2);
                    }
                }
            }
        } else{
            kHeldPaused = kHeld;
        }
        
        // If the wide settings has been changed, reinitialize screens
        if (wideEnabled != old_wide) {        
            gspWaitForVBlank();
            set_wide(wideEnabled);
            gspWaitForVBlank();
            reinitialize_screens();
            old_wide = wideEnabled;
        }

        // Handle level being completed
        if (level_info.completing) {
            int status = handle_wall_cutscene(delta);
            // Exiting
            if (status == 1) {
                exiting_level = true;
                // Restarting level
            } else if (status == 2) {
                level_info.completing = false;
                level_complete_destroy();
                continue;
            }
        }

        u64 end = svcGetSystemTick();
        u64 ticks = end - start;

        // Render the scene
        do {
            update_bottom_particles(delta);
            update_touch_effect(delta);

            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
            draw_fade();

            // Top screen
            C2D_SceneBegin(top);
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));

            draw_background(state.background_x / 8, -(state.camera_y / 8) + 200);

            C2D_ViewScale(SCALE, SCALE);
            C2D_ViewTranslate(0, CAM_Y_MTX_OFFSET);

            draw_objects();

            draw_end_wall(delta);
            
            draw_attempt_text();

            draw_ground(state.ground_x, state.camera_y, 0, false, SCREEN_WIDTH);
            
            if (state.ground_y_gfx > 2) {
                if (state.camera_y - LEVEL_Y_OFFSET + state.ground_y_gfx > 0) draw_ground(state.ground_x, state.camera_y, state.camera_y + state.ground_y_gfx - LEVEL_Y_OFFSET, false, SCREEN_WIDTH);
                draw_ground(state.ground_x, state.camera_y, state.camera_y - LEVEL_Y_OFFSET + SCREEN_HEIGHT_AREA - state.ground_y_gfx, true, SCREEN_WIDTH);
            }

            change_blending(true);
            draw_use_effects(get_use_effect_array_ptr(GFX_TOP_BUT_ABOVE_LEVEL));

            if (level_info.wall_y > 0) {
                drawParticleSystem(&end_wall_firework, 0, 0, 1);
                drawParticleSystem(&level_complete_effect_p1, 0, 0, 1);
                drawParticleSystem(&level_complete_effect_p2, 0, 0, 1);
                change_blending(false);

                draw_level_complete_popup();
            }

            change_blending(false);

            draw_new_best_popup();

            C2D_ViewTranslate(0, -CAM_Y_MTX_OFFSET);
            C2D_ViewScale(1/SCALE, 1/SCALE);
            gameplay_screen_top_loop();
            draw_level_complete_top();

            // Bottom screen
            C2D_SceneBegin(bot);
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));

            gameplay_screen_bot_loop();
            draw_level_complete();

            change_blending(true);
            draw_bottom_particles();
            draw_touch_effect();
            change_blending(false);

            if (state.profiling) {
                float processingTime = ((ticks / CPU_TICKS_PER_MSEC)) * 6;
                float drawingTime = C3D_GetDrawingTime() * 6;
                float fps = 1 / delta;
                if (fps > 60) fps = 60;

                #define DEBUG_TEXT_SCALE 0.4f, 0.4f
                
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 6,  DEBUG_TEXT_SCALE, 0, true, "CPU: %6.2f%% (%6.2f%% %6.2f%%)", (C3D_GetProcessingTime() * 6) + processingTime, C3D_GetProcessingTime() * 6, processingTime);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 18, DEBUG_TEXT_SCALE, 0, true, "GPU: %6.2f%%", drawingTime);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 30, DEBUG_TEXT_SCALE, 0, true, "FPS: %6.1f", fps);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 42, DEBUG_TEXT_SCALE, 0, true, "Linear free: %d", linearSpaceFree());
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 42, DEBUG_TEXT_SCALE, 0, true, "CMDBuf: %6.2f%%", C3D_GetCmdBufUsage()*100.0f);

                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 66,  DEBUG_TEXT_SCALE, 0, true, "%d steps", steps);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 54,  DEBUG_TEXT_SCALE, 0, true, "Particle: %6.2f%%", particle_calc_time * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 78,  DEBUG_TEXT_SCALE, 0, true, "Triggers: %6.2f%%", triggers_time * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 90,  DEBUG_TEXT_SCALE, 0, true, "Collision %d/%d", number_of_collisions, number_of_collisions_checks);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 102, DEBUG_TEXT_SCALE, 0, true, "Physics: %6.2f%%", physics_calc_time * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 114, DEBUG_TEXT_SCALE, 0, true, " - Coll: %6.2f%%", collision_time * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 126, DEBUG_TEXT_SCALE, 0, true, " - Play: %6.2f%%", player_time * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 180, 138, DEBUG_TEXT_SCALE, 0, true, " - Hndl: %6.2f%%", handle_player_time * 6);

                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   54,  DEBUG_TEXT_SCALE, 0, true, "SprDraw:  %6.2f%%", (sprite_drawing_time) * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   66,  DEBUG_TEXT_SCALE, 0, true, " - Creating: %6.2f%%", (object_creating_time) * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   78,  DEBUG_TEXT_SCALE, 0, true, " - Sorting:  %6.2f%%", (object_sorting_time) * 6);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   102,  DEBUG_TEXT_SCALE, 0, true, "Drawing:  %6.2f%%", (object_drawing_time) * 6);

                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   114,  DEBUG_TEXT_SCALE, 0, true, "Touch:  %d, %d", touchPos.px, touchPos.py);

                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   138,  DEBUG_TEXT_SCALE, 0, true, "Player");
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   150,  DEBUG_TEXT_SCALE, 0, true, "- Tick: %d", state.player.frame);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   162,  DEBUG_TEXT_SCALE, 0, true, "- X: %.2f", state.player.x);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   174,  DEBUG_TEXT_SCALE, 0, true, "- Y: %.2f", state.player.y);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   186,  DEBUG_TEXT_SCALE, 0, true, "- VX: %.2f", state.player.vel_x * STEPS_DT);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0,   198,  DEBUG_TEXT_SCALE, 0, true, "- VY: %.2f", state.player.vel_y * STEPS_DT);

                
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 110,   138,  DEBUG_TEXT_SCALE, 0, true, "Camera");
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 110,   150,  DEBUG_TEXT_SCALE, 0, true, "- X: %.2f", state.camera_x);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 110,   162,  DEBUG_TEXT_SCALE, 0, true, "- Y: %.2f", state.camera_y);
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 110,   174,  DEBUG_TEXT_SCALE, 0, true, "- IntY: %.2f", state.camera_intended_y);
            }

            if (state.noclip) {
                draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 234, 0.5f, 0.5f, 0, true, "Noclip Activated");
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
            in_level_complete = false;
            break;
        }
    }

    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);

    level_data_sel->attempts += state.current_data.attempts;
    level_data_sel->jumps += state.current_data.jumps;
    level_data_sel->normal_progress = state.current_data.max_normal;
    level_data_sel->practice_progress = state.current_data.max_practice;

    total_attempts += state.current_data.attempts;
    total_jumps += state.current_data.jumps;

    if (state.custom_level) {
        save_level_progress();
    } else {
        save_main_level_progress(curr_level_id);
    }

    cfg_save();

    free_particles();

    unload_level();

    clear_respawn_effect();

    level_complete_destroy();

    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
    
    if (song_loaded) unpause_playback_mp3();

    game_state = (state.custom_level ? STATE_EXTERNAL_LEVELS : STATE_LEVEL_SELECT);
}

void game_assets_init() {
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

    particleSheet = C2D_SpriteSheetLoad("romfs:/gfx/particles.t3x");
    if (!particleSheet) svcBreak(USERBREAK_PANIC);

    initParticleSystem(&touch_drag_particles, &touch_drag_effect);
    touch_drag_particles.relativeStationary = true;

    initParticleSystem(&touch_explosion_particles, &touch_explosion_effect);
    touch_explosion_particles.relativeStationary = true;

    initParticleSystem(&glitter_particles_bottom, &glitter_effect);
    glitter_particles_bottom.relativeStationary = true;
    glitter_particles_bottom.cfg.startColorAlpha = 1.f;

    initParticleSystem(&slow_speed_particles_bottom, &speed_effect_slow);
    slow_speed_particles_bottom.relativeStationary = true;

    initParticleSystem(&normal_speed_particles_bottom, &speed_effect_normal);
    normal_speed_particles_bottom.relativeStationary = true;

    initParticleSystem(&fast_speed_particles_bottom, &speed_effect_fast);
    fast_speed_particles_bottom.relativeStationary = true;

    initParticleSystem(&faster_speed_particles_bottom, &speed_effect_vfast);
    faster_speed_particles_bottom.relativeStationary = true;

    
}

void load_sfx() {
    load_wav("romfs:/sfx/playSound_01.wav", &play_sound);
    load_wav("romfs:/sfx/quitSound_01.wav", &quit_sound);
    load_wav("romfs:/sfx/explode_11.wav", &explode_sound);
    load_wav("romfs:/sfx/endStart_02.wav", &end_sound);
    load_wav("romfs:/sfx/honk.wav", &honk);
    load_wav("romfs:/sfx/highscoreGet02.wav", &coin_sound);
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

    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, false);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, false);

    srand(time(NULL));
    alt_title_screen = (rand() & (128 - 1)) == 0;
    
    C2D_SetTintMode(C2D_TintMult);
    
    if(ndspInit()) {
        no_dsp_firmware();
    }

    required_loading_screen_assets_init();

    loading_screen_init();

    u64 start = svcGetSystemTick();

    loading_screen_update(0);

    ui_assets_init();
    game_assets_init();
    loading_screen_update(10);

    load_main_level_progress();
    
    loading_screen_update(25);

    calculate_stats();

    cache_all_sprites();

    loading_screen_update(40);
    
    init_default_use_effect_pools();
    update_player_colors();

    loading_screen_update(75);

    load_sfx();

    memset(&level_info, 0, sizeof(LoadedLevelInfo));
    
    loading_screen_update(90);

    u64 end = svcGetSystemTick();
    float loading_time = (end - start) / (CPU_TICKS_PER_MSEC) / 1000;
    
    // Wait a minimum of 3 seconds
    long waiting = (long)((3 - loading_time) * 1e9);
    if (waiting > 0) svcSleepThread(waiting);
    loading_screen_update(100);

    // Unload loading screen
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);

    // Set to known value
    change_blending(false);

    bool exit = false;
    while (aptMainLoop() && !exit) {
        // Update color if changed menus
        Color p1_not_white = get_white_if_black(p1_color);

        touch_drag_particles.cfg.startColorRed = p1_not_white.r / 255.f;
        touch_drag_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
        touch_drag_particles.cfg.startColorBlue = p1_not_white.b / 255.f;

        touch_explosion_particles.cfg.startColorRed = p1_not_white.r / 255.f;
        touch_explosion_particles.cfg.startColorGreen = p1_not_white.g / 255.f;
        touch_explosion_particles.cfg.startColorBlue = p1_not_white.b / 255.f;

        glitter_particles_bottom.cfg.startColorRed = get_white_if_black(p1_color).r / 255.f;
        glitter_particles_bottom.cfg.startColorGreen = get_white_if_black(p1_color).g / 255.f;
        glitter_particles_bottom.cfg.startColorBlue = get_white_if_black(p1_color).b / 255.f;
        glitter_particles_bottom.cfg.finishColorRed = get_white_if_black(p1_color).r / 255.f;
        glitter_particles_bottom.cfg.finishColorGreen = get_white_if_black(p1_color).g / 255.f;
        glitter_particles_bottom.cfg.finishColorBlue = get_white_if_black(p1_color).b / 255.f;

        slow_speed_particles_bottom.cfg.startColorRed = 255 / 255.f;
        slow_speed_particles_bottom.cfg.startColorGreen = 255 / 255.f;
        slow_speed_particles_bottom.cfg.startColorBlue = 0 / 255.f;

        normal_speed_particles_bottom.cfg.startColorRed = 0 / 255.f;
        normal_speed_particles_bottom.cfg.startColorGreen = 190 / 255.f;
        normal_speed_particles_bottom.cfg.startColorBlue = 255 / 255.f;

        fast_speed_particles_bottom.cfg.startColorRed = 0 / 255.f;
        fast_speed_particles_bottom.cfg.startColorGreen = 255 / 255.f;
        fast_speed_particles_bottom.cfg.startColorBlue = 0 / 255.f;

        faster_speed_particles_bottom.cfg.startColorRed = 230 / 255.f;
        faster_speed_particles_bottom.cfg.startColorGreen = 65 / 255.f;
        faster_speed_particles_bottom.cfg.startColorBlue = 255 / 255.f;

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
            case STATE_CREATOR_MENU:
                creator_menu_loop();
                break;
            case STATE_SEARCH_MENU:
                search_menu_loop();
                break;
            case STATE_SAVED_LEVELS:
                saved_levels_loop();
                break;
            case STATE_ONLINE_LEVEL:
                online_menu_loop();
                break;
            case STATE_EXTERNAL_LEVELS:
                external_levels_loop();
                break;
            case STATE_SOGGY: // Sog
                soggy_menu_loop();
                break;
            case STATE_EXIT:
                exit = true;
                break;
        }
    }

    close_log_file();

    free_cached_sprites();

    free_main_level_progress();

    // Delete graphics
    C2D_SpriteSheetFree(spriteSheet);
    C2D_SpriteSheetFree(spriteSheet2);
    C2D_SpriteSheetFree(glowSheet);
    C2D_SpriteSheetFree(bgSheet);
    C2D_SpriteSheetFree(bg2Sheet);
    C2D_SpriteSheetFree(iconSheet);
    C2D_SpriteSheetFree(trailSheet);
    C2D_SpriteSheetFree(particleSheet);
    C2D_SpriteSheetFree(ui_sheet);
    C2D_SpriteSheetFree(ui_2_sheet);
    C2D_SpriteSheetFree(groundSheet);
    C2D_SpriteSheetFree(bigFont_sheet);
    C2D_SpriteSheetFree(chatFont_sheet);
    C2D_SpriteSheetFree(goldFont_sheet);
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
