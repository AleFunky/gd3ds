#include "practice.h"
#include "icons.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "player/player.h"
#include "state.h"
#include "mp3_player.h"
#include "math_helpers.h"
#include "color_channels.h"
#include "utils/gfx.h"
#include "menus/settings.h"

#define MAX_CHECKPOINTS 100
#define CHECKPOINT_GFX_ID 6

typedef struct CheckpointData {
    Player p1;
    Player p2;

    float camera_x;
    float camera_y;

    float camera_intended_y;

    float ground_y;
    float ceiling_y;
    float ground_y_gfx;

    float wall_y;

    bool mirroring;
    int mirror_mult;    
    float mirror_timer;

    float original_mirror_factor;
    float intended_mirror_factor;

    float mirror_speed_factor;
    float mirror_factor;
    
    bool dual;
    
    float dual_portal_y;
    unsigned char speed;

    int current_fading_effect;
    bool p1_trail;

    ColorChannel channels[COL_CHANNEL_NUM];
    ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];
    
    float song_offset;

} CheckpointData;

CheckpointData checkpoints[MAX_CHECKPOINTS];
int checkpoint_count = 0;
int checkpoint_pointer = 0;
float checkpoint_timer = 0;
bool pseudo_checkpoint_exists = false;

// static const int checkpoint_size = sizeof(checkpoints);

void set_checkpoint_timer(float timer) {
    if (state.practice_mode && settingsState.autoCheckpoints) {
        // Set auto checkpoints timer
        checkpoint_timer = timer;

        // Quick checkpoints halves the timer
        if (settingsState.quickCheckpoints) {
            checkpoint_timer /= 2;
        }
    }
}

void new_checkpoint() {
    if (state.dead) return;

    // Wrap around
    if (++checkpoint_pointer >= MAX_CHECKPOINTS) checkpoint_pointer = 0;

    // Cap checkpoint count
    if (++checkpoint_count > MAX_CHECKPOINTS) checkpoint_count = MAX_CHECKPOINTS;

    CheckpointData *check = &checkpoints[checkpoint_pointer];

    check->camera_x = state.camera_x;
    check->camera_y = state.camera_y;

    check->p1 = state.player;
    check->p2 = state.player2;

    check->camera_intended_y = state.camera_intended_y;

    check->ground_y = state.ground_y;
    check->ceiling_y = state.ceiling_y;
    check->ground_y_gfx = state.ground_y_gfx;

    check->mirroring = state.mirroring;
    check->mirror_mult = state.mirror_mult;
    check->mirror_timer = state.mirror_timer;
    check->original_mirror_factor = state.original_mirror_factor;
    check->intended_mirror_factor = state.intended_mirror_factor;
    check->mirror_speed_factor = state.mirror_speed_factor;
    check->mirror_factor = state.mirror_factor;

    check->dual = state.dual;
    check->dual_portal_y = state.dual_portal_y;

    check->speed = state.speed;

    check->current_fading_effect = current_fading_effect;
    check->p1_trail = p1_trail;

    check->wall_y = level_info.wall_y;

    check->song_offset = level_info.song_offset + state.player.timeElapsed;

    memcpy(check->channels, channels, sizeof(channels));
    memcpy(check->col_trigger_buffer, col_trigger_buffer, sizeof(col_trigger_buffer));

    set_checkpoint_timer(AUTO_CHECKPOINT_TIME);
}

void restore_checkpoint() {
    CheckpointData *check = &checkpoints[checkpoint_pointer];

    state.camera_x = check->camera_x;
    state.camera_y = check->camera_y;

    state.player = check->p1;
    state.player2 = check->p2;

    state.player.buffering_state = (state.input.holdJump ? BUFFER_READY : BUFFER_NONE);
    state.player2.buffering_state = (state.input.holdJump ? BUFFER_READY : BUFFER_NONE);

    state.player.buffer_ufo = true;
    state.player2.buffer_ufo = true;

    state.camera_intended_y = check->camera_intended_y;

    state.ground_y = check->ground_y;
    state.ceiling_y = check->ceiling_y;
    state.ground_y_gfx = check->ground_y_gfx;

    state.mirroring = check->mirroring;
    state.mirror_mult = check->mirror_mult;
    state.mirror_timer = check->mirror_timer;
    state.original_mirror_factor = check->original_mirror_factor;
    state.intended_mirror_factor = check->intended_mirror_factor;
    state.mirror_speed_factor = check->mirror_speed_factor;
    state.mirror_factor = check->mirror_factor;
    
    state.dual = check->dual;
    state.dual_portal_y = check->dual_portal_y;

    state.speed = check->speed;

    level_info.wall_y = check->wall_y;

    current_fading_effect = check->current_fading_effect;
    p1_trail = check->p1_trail;

    if (settingsState.practiceMusicSync) seek_mp3(check->song_offset);
    
    memcpy(channels, check->channels, sizeof(channels));
    memcpy(col_trigger_buffer, check->col_trigger_buffer, sizeof(col_trigger_buffer));

    update_attempt_text_pos();

    set_checkpoint_timer(AUTO_CHECKPOINT_TIME);
}

void delete_last_checkpoint() {
    if (checkpoint_count > 0) {
        checkpoint_count--;

        // Wrap around pointer
        if (checkpoint_pointer-- == 0) {
            checkpoint_pointer = MAX_CHECKPOINTS - 1;
        }
    }
}

void clear_practice_mode() {
    checkpoint_count = 0;
    checkpoint_pointer = 0;
    state.practice_mode = false;
}

void start_practice_mode() {
    checkpoint_count = 0;
    checkpoint_pointer = 0;
    pseudo_checkpoint_exists = false;
    state.practice_mode = true;
    
    if (!settingsState.practiceMusicSync) {
        stop_mp3();
        play_practice_song();
    }
}

void exit_practice_mode() {
    state.practice_mode = false;
    init_variables();
    reload_level(); 

    if (settingsState.practiceMusicSync) {
        seek_mp3(level_info.song_offset);
    } else {
        stop_mp3();
        play_level_song(level_info.song_offset);
    }
}

void handle_auto_checkpoints(float delta) {
    // Exit if not in practice mode with autocheckpoints enabled
    if (!(state.practice_mode && settingsState.autoCheckpoints) || state.end_wall_anim_playing) return;
    
    if (checkpoint_timer <= 0) {
        switch (state.player.gamemode) {
            case GAMEMODE_PLAYER:
            case GAMEMODE_PLAYER_BALL:
                if (state.player.landed_from_jump) {
                    new_checkpoint();
                }
                break;

            case GAMEMODE_SHIP:
            case GAMEMODE_BIRD:
            case GAMEMODE_DART:
                new_checkpoint();
                pseudo_checkpoint_exists = true;
                break;

            default:
                break;
        }
    } else {
        checkpoint_timer -= delta;
    }
}

void handle_practice_mode() {
    if (!state.practice_mode) return;

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    if (((kDown & KEY_L) && !((kHeld & KEY_B) && settingsState.enableDebugBindings)) || (kDown & KEY_ZL)) {
        
        if (settingsState.autoCheckpoints  && player_gamemode_is_flying(&state.player) && pseudo_checkpoint_exists) {
            pseudo_checkpoint_exists = false;
            delete_last_checkpoint();
        }
        new_checkpoint();
    }

    if (((kDown & KEY_R) && !((kHeld & KEY_B) && settingsState.enableDebugBindings)) || (kDown & KEY_ZR)) {
        delete_last_checkpoint();
    }
}

static void draw_checkpoint(float x, float y) {
    C2D_Sprite spr = { 0 };
    C2D_SpriteFromSheet(&spr, spriteSheet2, CHECKPOINT_GFX_ID);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetPos(&spr, get_mirror_x(x, state.mirror_factor), y);

    C2D_DrawSprite(&spr);
}

int get_checkpoint_count() {
    int count = checkpoint_count;
    if (settingsState.autoCheckpoints && player_gamemode_is_flying(&state.player) && pseudo_checkpoint_exists) {
        count--;
    }
    return MAX(0, count);
}

void draw_checkpoints() {
    if (!state.practice_mode) return;

    int start = 0;

    if (settingsState.autoCheckpoints && player_gamemode_is_flying(&state.player) && pseudo_checkpoint_exists)
        start++;

    for (u32 checkpoint = start; checkpoint < checkpoint_count; checkpoint++) {
        // Obtain buffer index
        s32 index = WRAP((s32) (checkpoint_pointer - checkpoint), 0, MAX_CHECKPOINTS);
        CheckpointData *curr_checkpoint = &checkpoints[index];

        float calc_x = (curr_checkpoint->p1.x - state.camera_x);
        float calc_y = SCREEN_HEIGHT - ((curr_checkpoint->p1.y - state.camera_y));  

        if (calc_x < -60 || calc_x >= (SCREEN_WIDTH / SCALE) + 60) continue;
        if (calc_y < -60 || calc_y >= (SCREEN_HEIGHT / SCALE) + 60) continue;

        draw_checkpoint(calc_x, calc_y);
    }
}
