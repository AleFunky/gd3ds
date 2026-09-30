#include "practice.h"
#include "icons.h"
#include <stdlib.h>
#include <string.h>
#include "groups.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "player/player.h"
#include "state.h"
#include "mp3_player.h"
#include "math_helpers.h"
#include "triggers.h"
#include "utils/gfx.h"
#include "menus/settings_hub/settings.h"

// needed to re-create the runtime trigger buffers
// after reload_level() frees them
extern int alpha_trigger_capacity;
extern int move_trigger_capacity;
extern int spawn_trigger_capacity;

#define MAX_CHECKPOINTS 100
#define MAX_CHECKPOINT_CHANNELS (COL_CHANNEL_LAST + 256)
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

    // snapshot only of the color channels actually used by the level to avoid a 1024 entry copy per checkpoint
    // allocated dynamically in new_checkpoint() (tis is great)
    int channel_snapshot_count;
    int *channel_indices;
    ColorChannel *channel_snapshot;
    ColTriggerBuffer *trigger_snapshot;

    MoveTriggerBuffer *move_triggers;
    AlphaTriggerBuffer *alpha_triggers;
    SpawnTriggerBuffer *spawn_triggers;
    int move_triggers_count;
    int alpha_triggers_count;
    int spawn_triggers_count;

    // per-object state for triggers that were active at checkpoint time, so they
    // can continue from the exact point instead of restarting. bounded by the
    // number of active move/alpha triggers so it's usually empty
    int move_obj_count;
    int *move_obj_index;     // object indices in move groups
    float *move_obj_x;       // saved objects.x at checkpoint
    float *move_obj_y;       // saved objects.y at checkpoint
    int alpha_obj_count;
    int *alpha_obj_index;    // object indices in alpha groups
    float *alpha_obj_alpha;  // saved objects.alpha_trigger_opacity at checkpoint

    int fired_move_count;
    int *fired_move_index;

    float song_offset;

} CheckpointData;

CheckpointData checkpoints[MAX_CHECKPOINTS];
int checkpoint_count = 0;
int checkpoint_pointer = 0;
float checkpoint_timer = 0;
bool pseudo_checkpoint_exists = false;

// max number of distinct channel indices a level can use: built-in special
// channels + a generous cap for the level's own channels
#define MAX_CHECKPOINT_CHANNELS (COL_CHANNEL_LAST + 256)

// collects the array indices of every color channel that the level can touch
// and returns how many unique indices were written to 'out'
static int collect_used_channels(int *out) {
    int n = 0;

    // normal player channels
    out[n++] = get_col_channel_index(NONE);
    out[n++] = get_col_channel_index(COL_1);
    out[n++] = get_col_channel_index(COL_2);
    out[n++] = get_col_channel_index(COL_3);
    out[n++] = get_col_channel_index(COL_4);

    // built-in special channels
    for (int id = CHANNEL_BG; id < COL_CHANNEL_LAST; id++) {
        int idx = get_col_channel_index(id);
        bool seen = false;
        for (int i = 0; i < n; i++) {
            if (out[i] == idx) { seen = true; break; }
        }
        if (!seen && idx < COL_CHANNEL_NUM) out[n++] = idx;
    }

    // channels declared in the level
    for (int i = 0; i < channelCount && n < MAX_CHECKPOINT_CHANNELS; i++) {
        int idx = get_col_channel_index(colorChannels[i].channelID);
        if (idx < 0 || idx >= COL_CHANNEL_NUM) continue;
        bool seen = false;
        for (int j = 0; j < n; j++) {
            if (out[j] == idx) { seen = true; break; }
        }
        if (!seen) out[n++] = idx;
    }

    return n;
}

// frees the dynamic snapshot buffers of a checkpoint
static void free_checkpoint_snapshot(CheckpointData *check) {
    if (check->channel_indices) { free(check->channel_indices); check->channel_indices = NULL; }
    if (check->channel_snapshot) { free(check->channel_snapshot); check->channel_snapshot = NULL; }
    if (check->trigger_snapshot) { free(check->trigger_snapshot); check->trigger_snapshot = NULL; }
    check->channel_snapshot_count = 0;

    if (check->move_obj_index) { free(check->move_obj_index); check->move_obj_index = NULL; }
    if (check->move_obj_x)     { free(check->move_obj_x);     check->move_obj_x = NULL; }
    if (check->move_obj_y)     { free(check->move_obj_y);     check->move_obj_y = NULL; }
    check->move_obj_count = 0;
    if (check->alpha_obj_index) { free(check->alpha_obj_index); check->alpha_obj_index = NULL; }
    if (check->alpha_obj_alpha) { free(check->alpha_obj_alpha); check->alpha_obj_alpha = NULL; }
    check->alpha_obj_count = 0;

    if (check->fired_move_index) { free(check->fired_move_index); check->fired_move_index = NULL; }
    check->fired_move_count = 0;

    if (check->move_triggers)  { free(check->move_triggers);  check->move_triggers = NULL; }
    if (check->alpha_triggers) { free(check->alpha_triggers); check->alpha_triggers = NULL; }
    if (check->spawn_triggers) { free(check->spawn_triggers); check->spawn_triggers = NULL; }
    check->move_triggers_count = 0;
    check->alpha_triggers_count = 0;
    check->spawn_triggers_count = 0;
}

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

    int next_checkpoint = checkpoint_pointer + 1;
    if (next_checkpoint >= MAX_CHECKPOINTS) next_checkpoint = 0;

    CheckpointData *check = &checkpoints[next_checkpoint];

    // release the snapshot of the checkpoint we are about to overwrite
    free_checkpoint_snapshot(check);

    // reserve the runtime trigger buffers
    check->move_triggers  = (move_trigger_count  > 0) ? malloc(sizeof(MoveTriggerBuffer)  * move_trigger_count)  : NULL;
    check->alpha_triggers = (alpha_trigger_count > 0) ? malloc(sizeof(AlphaTriggerBuffer) * alpha_trigger_count) : NULL;
    check->spawn_triggers = (spawn_trigger_count > 0) ? malloc(sizeof(SpawnTriggerBuffer) * spawn_trigger_count) : NULL;
    check->move_triggers_count  = move_trigger_count;
    check->alpha_triggers_count = alpha_trigger_count;
    check->spawn_triggers_count = spawn_trigger_count;

    if ((move_trigger_count  > 0 && !check->move_triggers) ||
        (alpha_trigger_count > 0 && !check->alpha_triggers) ||
        (spawn_trigger_count > 0 && !check->spawn_triggers)) {
        // if out of memory leave the slot empty and skip this checkpoint so it doesn't crashes
        free_checkpoint_snapshot(check);
        return;
    }

    checkpoint_pointer = next_checkpoint;
    if (++checkpoint_count > MAX_CHECKPOINTS) checkpoint_count = MAX_CHECKPOINTS;

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

    // snapshot only the channels used by this level
    int used[MAX_CHECKPOINT_CHANNELS];
    int used_count = collect_used_channels(used);

    check->channel_indices = malloc(sizeof(int) * used_count);
    check->channel_snapshot = malloc(sizeof(ColorChannel) * used_count);
    check->trigger_snapshot = malloc(sizeof(ColTriggerBuffer) * used_count);
    check->channel_snapshot_count = used_count;

    if (!check->channel_indices || !check->channel_snapshot || !check->trigger_snapshot) {
        free_checkpoint_snapshot(check);
        return;
    }

    for (int i = 0; i < used_count; i++) {
        int idx = used[i];
        check->channel_indices[i] = idx;
        check->channel_snapshot[i] = channels[idx];
        check->trigger_snapshot[i] = col_trigger_buffer[idx];
    }

    if (check->move_triggers)  memcpy(check->move_triggers,  move_trigger_buffer,  sizeof(MoveTriggerBuffer)  * move_trigger_count);
    if (check->alpha_triggers) memcpy(check->alpha_triggers, alpha_trigger_buffer, sizeof(AlphaTriggerBuffer) * alpha_trigger_count);
    if (check->spawn_triggers) memcpy(check->spawn_triggers, spawn_trigger_buffer, sizeof(SpawnTriggerBuffer) * spawn_trigger_count);

    check->move_obj_count = 0;
    check->move_obj_index = NULL;
    check->move_obj_x = NULL;
    check->move_obj_y = NULL;
    int move_cap = 0;
    for (int i = 0; i < move_trigger_count; i++)
        if (move_trigger_buffer[i].active)
            for (GroupNode *p = get_group(move_trigger_buffer[i].target_group); p; p = p->next)
                move_cap++;
    for (int i = 0; i < objects.count; i++)
        if (objects.x[i] != objects.original_x[i] || objects.y[i] != objects.original_y[i])
            move_cap++;
    if (move_cap > 0) {
        check->move_obj_index = malloc(sizeof(int) * move_cap);
        check->move_obj_x = malloc(sizeof(float) * move_cap);
        check->move_obj_y = malloc(sizeof(float) * move_cap);
        if (check->move_obj_index && check->move_obj_x && check->move_obj_y) {
            int n = 0;
            for (int i = 0; i <move_trigger_count ; i++) {
                if (!move_trigger_buffer[i].active) continue;
                for (GroupNode *p = get_group(move_trigger_buffer[i].target_group); p; p = p->next) {
                    if (n >= move_cap) break;
                    int oi = p->obj;
                    check->move_obj_index[n] = oi;
                    check->move_obj_x[n] = objects.x[oi];
                    check->move_obj_y[n] = objects.y[oi];
                    n++;
                }
            }
            for (int i = 0; i < objects.count; i++) {
                if (objects.x[i] == objects.original_x[i] && objects.y[i] == objects.original_y[i]) continue;
                bool dup = false;
                for (int k = 0; k < n; k++)
                    if (check->move_obj_index[k] == i) { dup = true; break; }
                if (dup) continue;
                if (n >= move_cap) break;
                check->move_obj_index[n] = i;
                check->move_obj_x[n] = objects.x[i];
                check->move_obj_y[n] = objects.y[i];
                n++;
            }
            check->move_obj_count = n;
        }
    }

    check->alpha_obj_count = 0;
    check->alpha_obj_index = NULL;
    check->alpha_obj_alpha = NULL;
    int alpha_cap = 0;
    for (int i = 0; i < alpha_trigger_count; i++)
        if (alpha_trigger_buffer[i].active)
            for (GroupNode *p = get_group(alpha_trigger_buffer[i].target_group); p; p = p->next)
                alpha_cap++;
    if (alpha_cap > 0) {
        check->alpha_obj_index = malloc(sizeof(int) * alpha_cap);
        check->alpha_obj_alpha = malloc(sizeof(float) * alpha_cap);
        if (check->alpha_obj_index && check->alpha_obj_alpha) {
            int n = 0;
            for (int i = 0; i < alpha_trigger_count; i++) {
                if (!alpha_trigger_buffer[i].active) continue;
                for (GroupNode *p = get_group(alpha_trigger_buffer[i].target_group); p; p = p->next) {
                    if (n >= alpha_cap) break;
                    int oi = p->obj;
                    check->alpha_obj_index[n] = oi;
                    check->alpha_obj_alpha[n] = objects.alpha_trigger_opacity[oi];
                    n++;
                }
            }
            check->alpha_obj_count = n;
        }
    }

    check->fired_move_count = 0;
    check->fired_move_index = NULL;
    int fired_cap = 0;
    for (int i = 0; i < objects.count; i++)
        if (objects.id[i] == MOVE_TRIGGER && GET_ACTIVATED(i))
            fired_cap++;
    if (fired_cap > 0) {
        check->fired_move_index = malloc(sizeof(int) * fired_cap);
        if (check->fired_move_index) {
            int n = 0;
            for (int i = 0; i < objects.count; i++) {
                if (n >= fired_cap) break;
                if (objects.id[i] == MOVE_TRIGGER && GET_ACTIVATED(i))
                    check->fired_move_index[n++] = i;
            }
            check->fired_move_count = n;
        }
    }

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
    
    for (int i = 0; i < check->channel_snapshot_count; i++) {
        int idx = check->channel_indices[i];
        channels[idx] = check->channel_snapshot[i];
        col_trigger_buffer[idx] = check->trigger_snapshot[i];
    }

    if (check->move_triggers && check->move_triggers_count > 0) {
        MoveTriggerBuffer *buf = realloc(move_trigger_buffer, sizeof(MoveTriggerBuffer) * check->move_triggers_count);
        if (buf) {
            move_trigger_buffer = buf;
            move_trigger_capacity = check->move_triggers_count;
            move_trigger_count = check->move_triggers_count;
            memcpy(move_trigger_buffer, check->move_triggers, sizeof(MoveTriggerBuffer) * check->move_triggers_count);
        }
    }
    if (check->alpha_triggers && check->alpha_triggers_count > 0) {
        AlphaTriggerBuffer *buf = realloc(alpha_trigger_buffer, sizeof(AlphaTriggerBuffer) * check->alpha_triggers_count);
        if (buf) {
            alpha_trigger_buffer = buf;
            alpha_trigger_capacity = check->alpha_triggers_count;
            alpha_trigger_count = check->alpha_triggers_count;
            memcpy(alpha_trigger_buffer, check->alpha_triggers, sizeof(AlphaTriggerBuffer) * check->alpha_triggers_count);
        }
    }
    if (check->spawn_triggers && check->spawn_triggers_count > 0) {
        SpawnTriggerBuffer *buf = realloc(spawn_trigger_buffer, sizeof(SpawnTriggerBuffer) * check->spawn_triggers_count);
        if (buf) {
            spawn_trigger_buffer = buf;
            spawn_trigger_capacity = check->spawn_triggers_count;
            spawn_trigger_count = check->spawn_triggers_count;
            memcpy(spawn_trigger_buffer, check->spawn_triggers, sizeof(SpawnTriggerBuffer) * check->spawn_triggers_count);
        }
    }

    for (int i = 0; i < move_trigger_count; i++)
        move_trigger_buffer[i].restored_from_checkpoint = move_trigger_buffer[i].active;
    for (int i = 0; i < alpha_trigger_count; i++)
        alpha_trigger_buffer[i].restored_from_checkpoint = alpha_trigger_buffer[i].active;
    for (int i = 0; i < spawn_trigger_count; i++)
        spawn_trigger_buffer[i].restored_from_checkpoint = spawn_trigger_buffer[i].active;

    for (int i = 0; i < check->move_obj_count; i++) {
        int oi = check->move_obj_index[i];
        if (objects.x[oi] != check->move_obj_x[i] || objects.y[oi] != check->move_obj_y[i])
            objects.flags[oi] |= FLAG_DIRTY;
        objects.x[oi] = check->move_obj_x[i];
        objects.y[oi] = check->move_obj_y[i];
        objects.last_x[oi] = check->move_obj_x[i];
        objects.last_y[oi] = check->move_obj_y[i];
        update_object_section(oi);
    }
    for (int i = 0; i < check->alpha_obj_count; i++) {
        objects.alpha_trigger_opacity[check->alpha_obj_index[i]] = check->alpha_obj_alpha[i];
    }

    for (int i = 0; i < check->fired_move_count; i++) {
        SET_ACTIVATED(check->fired_move_index[i], true);
    }

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

        free_checkpoint_snapshot(&checkpoints[checkpoint_pointer]);
    }
}

void clear_practice_mode() {
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        free_checkpoint_snapshot(&checkpoints[i]);
    }
    checkpoint_count = 0;
    checkpoint_pointer = 0;
    state.practice_mode = false;
}
void start_practice_mode() {
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        free_checkpoint_snapshot(&checkpoints[i]);
    }
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
            case GAMEMODE_BALL:
                if (state.player.landed_from_jump) {
                    new_checkpoint();
                }
                break;

            case GAMEMODE_SHIP:
            case GAMEMODE_UFO:
            case GAMEMODE_WAVE:
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
