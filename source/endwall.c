#include "main.h"
#include "state.h"
#include "math_helpers.h"
#include "mp3_player.h"
#include "utils/gfx.h"
#include "wav_player.h"

#include "endwall.h"
#include "particles/rays.h"
#include "particles/circles.h"

#include "menus/components/ui_screen.h"
#include "menus/level_complete.h"

static int fireworks_spawned = 0;
static int circunferences_spawned = 0;
static float completion_timer = 0.0f;
static float circunference_timer = 0.0f;
static bool level_complete_initialized = false;

typedef enum {
    SCALE_IN,
    WAITING,
    SCALE_OUT,
} LevelCompleteState;

typedef struct {
    bool active;
    
    LevelCompleteState state;

    float scale;

    float initial_scale;
    float target_scale;
    float timer;
    float duration;
    EaseTypes ease;
} LevelCompletePopup;

static void init_level_complete_popup();

static LevelCompletePopup level_complete_popup;

int handle_wall_cutscene(float delta) {
    // Init wall variables
    if (completion_timer == 0) {
        state.completion_shake = true;
        fireworks_spawned = 0;
        circunferences_spawned = 0;

        
        
        UseEffect *effect = add_use_effect(level_info.wall_x, level_info.wall_y, USE_EFFECT_OBJ_NOTHING, &end_wall_filled_first, GFX_TOP);
        if (effect) {
            Color p1_white = get_white_if_black(p1_color);
            effect->def.colorR = p1_white.r / 255.f;
            effect->def.colorG = p1_white.g / 255.f;
            effect->def.colorB = p1_white.b / 255.f;
        }
        
        play_sfx(&end_sound, 1);

        rays_start();
    }

    // Make circunferences
    if (circunference_timer >= (circunferences_spawned * CIRCUNFERENCE_SPAWN_DELAY) && circunferences_spawned < CIRCUNFERENCE_COUNT) {        
        UseEffect *effect = add_use_effect(level_info.wall_x, level_info.wall_y, USE_EFFECT_OBJ_NOTHING, &end_wall_circunference, GFX_TOP);
        if (effect) {
            Color p1_white = get_white_if_black(p1_color);
            effect->def.colorR = p1_white.r / 255.f;
            effect->def.colorG = p1_white.g / 255.f;
            effect->def.colorB = p1_white.b / 255.f;
        }
        circunferences_spawned++;
    }

    // Handle level complete text and co
    if (completion_timer >= FIREWORK_SPAWN_TIME && completion_timer <= MENU_TIME) {
        // Init circles
        if (fireworks_spawned == 0) {
            rays_start_fade();

            init_level_complete_popup();

            // Spawn again circunferences
            circunferences_spawned = 0;
            circunference_timer = 0.0f;

            // Endwall circle
            UseEffect *effect = add_use_effect(level_info.wall_x, level_info.wall_y, USE_EFFECT_OBJ_NOTHING, &end_wall_filled_second, GFX_TOP);
            if (effect) {
                Color p1_white = get_white_if_black(p1_color);
                effect->def.colorR = p1_white.r / 255.f;
                effect->def.colorG = p1_white.g / 255.f;
                effect->def.colorB = p1_white.b / 255.f;
            }

            // Pop up circle
            float calc_x = state.camera_x + (SCREEN_WIDTH_AREA / 2);
            float calc_y = state.camera_y + (SCREEN_HEIGHT - (SCREEN_HEIGHT_AREA / 2));
            UseEffect *effect2 = add_use_effect(calc_x, calc_y, USE_EFFECT_OBJ_NOTHING, &end_wall_filled_title, GFX_TOP_BUT_ABOVE_LEVEL);
            if (effect2) {
                Color p1_white = get_white_if_black(p1_color);
                effect2->def.colorR = p1_white.r / 255.f;
                effect2->def.colorG = p1_white.g / 255.f;
                effect2->def.colorB = p1_white.b / 255.f;
            }

            level_complete_effect_p1.emitterX = SCREEN_WIDTH_AREA / 2;
            level_complete_effect_p1.emitterY = SCREEN_HEIGHT_AREA / 2;
            level_complete_effect_p2.emitterX = SCREEN_WIDTH_AREA / 2;
            level_complete_effect_p2.emitterY = SCREEN_HEIGHT_AREA / 2;

            spawnMultipleParticles(&level_complete_effect_p1, 200);
            spawnMultipleParticles(&level_complete_effect_p2, 200);
        }

        // Fireworks
        if (completion_timer >= FIREWORK_SPAWN_TIME + (fireworks_spawned * FIREWORK_SPAWN_DELAY)) {
            float calc_x = state.camera_x + 100 + random_float(0, SCREEN_WIDTH_AREA - 200);
            float y = random_float(0, SCREEN_HEIGHT_AREA);
            float calc_y = state.camera_y + (SCREEN_HEIGHT - y);
            UseEffect *effect = add_use_effect(calc_x, calc_y, USE_EFFECT_OBJ_NOTHING, &end_wall_firework_circle, GFX_TOP_BUT_ABOVE_LEVEL);
            if (effect) {
                Color p2_white = get_white_if_black(p2_color);
                effect->def.colorR = p2_white.r / 255.f;
                effect->def.colorG = p2_white.g / 255.f;
                effect->def.colorB = p2_white.b / 255.f;
            }

            end_wall_firework.emitterX = calc_x;
            end_wall_firework.emitterY = calc_y;
            spawnMultipleParticles(&end_wall_firework, 25);
            
            fireworks_spawned++;
        }
        
        state.completion_shake = false;
    }

    // End the level!
    if (completion_timer > MENU_TIME) {
        if (!level_complete_initialized) {
            level_complete_init();
            level_complete_initialized = true;
        }

        // Handle level complete menu
        int status = level_complete_loop(delta);
        if (status) {
            // Exiting
            if (status == 1) {
                stop_mp3();
                set_fade_status(FADE_STATUS_OUT);
            } else if (status == 2) { // Restarting
                init_variables();
                reload_level(); 
                if (song_loaded) seek_mp3(level_info.song_offset);
                unpause_playback_mp3();
            }
            level_complete_initialized = false;
            completion_timer = 0.0f;
            circunference_timer = 0.0f;
            return status;
        }
    }

    completion_timer += delta;
    circunference_timer += delta;
    return 0;
}

static void init_level_complete_popup() {
    level_complete_popup.active = true;
    level_complete_popup.state = SCALE_IN;
    level_complete_popup.ease = ELASTIC_OUT;

    level_complete_popup.timer = 0;
    level_complete_popup.duration = LVL_COMPLETE_STATE_0_DURATION;
    level_complete_popup.initial_scale = 0.01;
    level_complete_popup.target_scale = LVL_COMPLETE_STATE_0_TARGET_SCALE;
}

void handle_level_complete_popup(float delta) {
    if (level_complete_popup.active) {
        switch (level_complete_popup.state) {
            case SCALE_IN:
                if (level_complete_popup.timer > LVL_COMPLETE_STATE_0_DURATION) {
                    level_complete_popup.state = WAITING;
                    level_complete_popup.ease = EASE_LINEAR;

                    level_complete_popup.timer = 0;
                    level_complete_popup.duration = LVL_COMPLETE_STATE_1_DURATION;
                    level_complete_popup.initial_scale = level_complete_popup.scale;
                    level_complete_popup.target_scale = level_complete_popup.scale;
                }
                break;
            case WAITING:
                if (level_complete_popup.timer > LVL_COMPLETE_STATE_1_DURATION) {
                    level_complete_popup.state = SCALE_OUT;
                    level_complete_popup.ease = QUAD_IN;
                    
                    level_complete_popup.timer = 0;
                    level_complete_popup.duration = LVL_COMPLETE_STATE_2_DURATION;
                    level_complete_popup.initial_scale = level_complete_popup.scale;
                    level_complete_popup.target_scale = LVL_COMPLETE_STATE_2_TARGET_SCALE;
                }
                break;
            case SCALE_OUT:
                if (level_complete_popup.timer > LVL_COMPLETE_STATE_2_DURATION) {
                    level_complete_popup.active = false;
                }
                break;
        }
        level_complete_popup.scale = easeValue(level_complete_popup.ease, level_complete_popup.initial_scale, level_complete_popup.target_scale, level_complete_popup.timer, level_complete_popup.duration, 1.f);

        level_complete_popup.timer += delta;
    }
}

void draw_level_complete_popup() {
    if (level_complete_popup.active) {
        float scale = level_complete_popup.scale;

        C2D_Sprite text = { 0 };
        C2D_SpriteFromSheet(&text, ui_sheet, LVL_COMPLETE_IMAGE_ID);
        C3D_TexSetFilter(text.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&text, 0.5f, 0.5f);
        C2D_SpriteSetPos(&text, SCREEN_WIDTH_AREA / 2, SCREEN_HEIGHT_AREA / 2);
        C2D_SpriteSetScale(&text, scale, scale);
        C2D_DrawSprite(&text);
    }
}