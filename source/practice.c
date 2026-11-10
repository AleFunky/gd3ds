#include "practice.h"
#include "icons.h"
#include <stdlib.h>
#include <string.h>
#include "groups.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "player/player.h"
#include "player/collision.h"
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
extern int touch_trigger_capacity;
extern int follow_player_y_trigger_capacity;

bool restored_checkpoint = false;

int used_count = 0;
int channels_used[COL_CHANNEL_NUM];

#define MAX_CHECKPOINTS 100
#define CHECKPOINT_GFX_ID 6

typedef struct {
    int source_obj;
    float time_run;
} PulseSnapshot;

typedef struct {
    int obj;
    float rotation, visual_rotation;
} RotateObjectSnapshot;

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
    
    bool show_player;
    
    float dual_portal_y;
    unsigned char speed;

    int current_fading_effect;
    bool p1_trail;

    // snapshot only of the color channels actually used by the level to avoid a 1024 entry copy per checkpoint
    // allocated dynamically in new_checkpoint() (tis is great)
    int channel_snapshot_count;
    ColorChannel *channel_snapshot;
    ColTriggerBuffer *trigger_snapshot;

    MoveTriggerBuffer *move_triggers;
    AlphaTriggerBuffer *alpha_triggers;
    SpawnTriggerBuffer *spawn_triggers;
    FollowPlayerYTriggerBuffer *follow_triggers;
    FollowTriggerBuffer *follow_trigger_buffers;
    RotateTriggerBuffer *rotate_triggers;
    RotateTargetState *rotate_targets;
    int *rotate_target_offsets;
    RotateObjectSnapshot *rotate_obj_snapshot;
    ItemCount *count_items;
    CountTriggerBuffer *count_triggers;
    TouchTriggerBuffer *touch_triggers;
    int *collected_pickups;
    int count_items_count;
    int count_triggers_count;
    int touch_triggers_count;
    int collected_pickups_count;
    int move_triggers_count;
    int alpha_triggers_count;
    int spawn_triggers_count;
    int follow_triggers_count;
    int follow_trigger_buffers_count;
    int rotate_triggers_count;
    int rotate_targets_count;
    int rotate_obj_count;

    // per-object state for triggers that were active at checkpoint time, so they
    // can continue from the exact point instead of restarting. bounded by the
    // number of active move/alpha triggers so it's usually empty
    int move_obj_count;
    uint8_t *move_idx_stream;
    int move_idx_bytes;
    uint8_t *move_obj_x;
    int move_x_bytes;
    int move_x_fmt;
    int move_x_u;
    int move_cx;
    uint8_t *move_obj_y;
    int move_y_bytes;
    int move_y_fmt;
    int move_y_u;
    int move_cy;

    int fired_move_count;
    int *fired_move_index;

    int key_count;
    int *key_index;

    int toggle_group_count;
    int *toggle_group_id;

    int alpha_group_count;
    int *alpha_group_id;
    float *alpha_group_alpha;

    int pulse_snap_count;
    PulseSnapshot *pulse_snap;

    float song_offset;

    void *alloc_base;
} CheckpointData;

CheckpointData checkpoints[MAX_CHECKPOINTS];

static CheckpointData *placed_checkpoint = NULL;

int checkpoint_count = 0;
int checkpoint_pointer = 0;
float checkpoint_timer = 0;
bool pseudo_checkpoint_exists = false;

static unsigned int ck_float_bits(float f);
static int ck_varint_len(unsigned int v);
static uint8_t *ck_varint_put(uint8_t *p, unsigned int v);
static unsigned int ck_varint_get(const uint8_t **pp);
static float ck_axis_get(const uint8_t *blob, int fmt, int u, int k);

static void add_used_channel(int *out, bool *seen, int *count, int channel) {
    if (channel <= 0 || channel >= COL_CHANNEL_NUM) return;

    int idx = get_col_channel_index(channel);
    if (seen[idx]) return;
    seen[idx] = true;
    out[(*count)++] = idx;
}


// collects the array indices of every color channel that the level can touch
// and returns how many unique indices were written to 'out'
static int collect_used_channels(int *out) {
    int n = 0;
    bool seen[COL_CHANNEL_NUM] = { false };

    // normal player channels
    add_used_channel(out, seen, &n, NONE);
    add_used_channel(out, seen, &n, COL_1);
    add_used_channel(out, seen, &n, COL_2);
    add_used_channel(out, seen, &n, COL_3);
    add_used_channel(out, seen, &n, COL_4);

    // built-in special channels
    for (int id = CHANNEL_BG; id < COL_CHANNEL_LAST; id++) {
        add_used_channel(out, seen, &n, id);
    }

    // channels declared in the level
    for (int i = 0; i < channelCount && n < COL_CHANNEL_NUM; i++) {
        add_used_channel(out, seen, &n, colorChannels[i].channelID);
        add_used_channel(out, seen, &n, colorChannels[i].inheritedChannelID);
    }

    // check for objects
    for (int i = 0; i < objects.count && n < COL_CHANNEL_NUM; i++) {
        add_used_channel(out, seen, &n, objects.col_channel[i]);
        add_used_channel(out, seen, &n, objects.detail_col_channel[i]);
        add_used_channel(out, seen, &n, objects.v1p9_col_channel[i]);

        if (is_color_trigger(objects.id[i])) {
            ColorTrigger *trigger = get_color_trigger(i);
            add_used_channel(out, seen, &n, trigger->target_color_id);
            add_used_channel(out, seen, &n, trigger->copied_color_id);
        } else if (objects.id[i] == PULSE_TRIGGER) {
            PulseTrigger *trigger = get_pulse_trigger(i);
            if (trigger->pulse_target_type == PULSE_TARGET_CHANNEL)
                add_used_channel(out, seen, &n, trigger->target_group);
            add_used_channel(out, seen, &n, trigger->copied_color_id);
        }
    }

    return n;
}

void load_used_channels() {
    used_count = collect_used_channels(channels_used);
}

// frees the dynamic snapshot buffers of a checkpoint
#define CK_ALIGN8(x) (((x) + 7u) & ~(size_t)7u)

static unsigned int ck_float_bits(float f) {
    unsigned int u;
    memcpy(&u, &f, sizeof u);
    return u;
}

static int ck_varint_len(unsigned int v) {
    int n = 1;
    while (v >= 128u) { v >>= 7; n++; }
    return n;
}
static uint8_t *ck_varint_put(uint8_t *p, unsigned int v) {
    while (v >= 128u) { *p++ = (uint8_t)(v | 0x80u); v >>= 7; }
    *p++ = (uint8_t)v;
    return p;
}
static unsigned int ck_varint_get(const uint8_t **pp) {
    unsigned int v = 0;
    int shift = 0;
    const uint8_t *p = *pp;
    for (;;) {
        uint8_t b = *p++;
        v |= (unsigned int)(b & 0x7Fu) << shift;
        if (!(b & 0x80u)) break;
        shift += 7;
    }
    *pp = p;
    return v;
}

static uint32_t ck_next_pow2(uint32_t x) {
    uint32_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

static int ck_dedup_put_h(int *h, uint32_t mask, float *tbl, int *n, float v) {
    if (*n > 65536) return 0;
    uint32_t bits = ck_float_bits(v);
    uint32_t i = (bits * 2654435761u) & mask;
    for (;;) {
        int val = h[i];
        if (val == 0) {
            h[i] = *n + 1;
            tbl[*n] = v;
            return (*n)++;
        }
        if (ck_float_bits(tbl[val - 1]) == bits) return val - 1;
        i = (i + 1) & mask;
    }
}

static void ck_axis_choose(int c, int u, int *fmt, int *bytes) {
    int best = (int)(sizeof(float) * (size_t)c);
    int best_fmt = 0;
    if (u > 0 && u <= 256) {
        int b = (int)(sizeof(float) * (size_t)u) + c;
        if (b < best) { best = b; best_fmt = 1; }
    }
    if (u > 0 && u <= 65536) {
        int b = (int)(sizeof(float) * (size_t)u) + 2 * c;
        if (b < best) { best = b; best_fmt = 2; }
    }
    *fmt = best_fmt;
    *bytes = best;
}

static float ck_axis_get(const uint8_t *blob, int fmt, int u, int k) {
    if (fmt == 0) {
        float v;
        memcpy(&v, blob + (size_t)k * sizeof(float), sizeof v);
        return v;
    }
    const float *tbl = (const float *)blob;
    unsigned int idx;
    if (fmt == 1) idx = ((const uint8_t *)(blob + (size_t)u * sizeof(float)))[k];
    else          idx = ((const uint16_t *)(blob + (size_t)u * sizeof(float)))[k];
    if (idx >= (unsigned int)u) idx = 0;
    return tbl[idx];
}

static void free_checkpoint_snapshot(CheckpointData *check) {
    if (check->alloc_base) { free(check->alloc_base); check->alloc_base = NULL; }

    memset(&check->channel_snapshot_count, 0,
           (size_t)((char *)&check->pulse_snap + sizeof(check->pulse_snap)
                    - (char *)&check->channel_snapshot_count));
}

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

static bool is_checkpoint_pickup(int obj) {
    PickupItem *pickup = get_pickup_item(obj);
    return pickup && objects.id[obj] != KEY_OBJ;
}

static bool is_collected_pickup(int obj) {
    return is_checkpoint_pickup(obj) &&
           (objects.flags[obj] & FLAG_TOGGLED) && (objects.activated[obj] & 3);
}

void new_checkpoint() {
    if (state.dead) return;

    flush_group_combination_visual_moves();

    int next_checkpoint = checkpoint_pointer + 1;
    if (next_checkpoint >= MAX_CHECKPOINTS) next_checkpoint = 0;

    CheckpointData *check = &checkpoints[next_checkpoint];
    placed_checkpoint = check;

    // release the snapshot of the checkpoint we are about to overwrite
    free_checkpoint_snapshot(check);

    int move_cap = 0;
    int move_cx = 0;
    int move_cy = 0;
    int move_idx_bytes = 0;
    int prev_index = 0;
    int *mv_list = NULL;
    if (objects.count > 0) mv_list = malloc(sizeof(int) * (size_t)objects.count);
    for (int i = 0; i < objects.count; i++) {
        unsigned int bx = ck_float_bits(objects.x[i]) ^ ck_float_bits(objects.original_x[i]);
        unsigned int by = ck_float_bits(objects.y[i]) ^ ck_float_bits(objects.original_y[i]);
        if (!bx && !by) continue;
        unsigned int gap = (unsigned int)(move_cap ? (i - prev_index) : i);
        unsigned int flags = (bx ? 2u : 0u) | (by ? 1u : 0u);
        move_idx_bytes += ck_varint_len((gap << 2) | flags);
        prev_index = i;
        if (mv_list) mv_list[move_cap] = i;
        move_cap++;
        if (bx) move_cx++;
        if (by) move_cy++;
    }

    int move_x_fmt = 0, move_y_fmt = 0, move_x_u = 0, move_y_u = 0;
    int move_x_bytes = 0, move_y_bytes = 0;
    int *ck_hx = NULL, *ck_hy = NULL;
    uint32_t ck_hx_slots = 0, ck_hy_slots = 0;
    if (mv_list && (move_cx > 0 || move_cy > 0)) {
        float *tbl = malloc(sizeof(float) * (size_t)(move_cx + move_cy));
        if (tbl) {
            uint32_t xb = move_cx > 65537 ? 65537u : (uint32_t)move_cx;
            uint32_t yb = move_cy > 65537 ? 65537u : (uint32_t)move_cy;
            ck_hx_slots = xb ? ck_next_pow2(xb * 2u) : 0;
            ck_hy_slots = yb ? ck_next_pow2(yb * 2u) : 0;
            if (ck_hx_slots) { ck_hx = malloc(sizeof(int) * ck_hx_slots); if (ck_hx) memset(ck_hx, 0, sizeof(int) * ck_hx_slots); }
            if (ck_hy_slots) { ck_hy = malloc(sizeof(int) * ck_hy_slots); if (ck_hy) memset(ck_hy, 0, sizeof(int) * ck_hy_slots); }
            float *tx = tbl;
            float *ty = tbl + move_cx;
            for (int e = 0; e < move_cap; e++) {
                int i = mv_list[e];
                if (ck_hx && (ck_float_bits(objects.x[i]) ^ ck_float_bits(objects.original_x[i])))
                    ck_dedup_put_h(ck_hx, ck_hx_slots - 1u, tx, &move_x_u, objects.x[i]);
                if (ck_hy && (ck_float_bits(objects.y[i]) ^ ck_float_bits(objects.original_y[i])))
                    ck_dedup_put_h(ck_hy, ck_hy_slots - 1u, ty, &move_y_u, objects.y[i]);
            }
            ck_axis_choose(move_cx, move_x_u, &move_x_fmt, &move_x_bytes);
            ck_axis_choose(move_cy, move_y_u, &move_y_fmt, &move_y_bytes);
            free(tbl);
        }
    }
    if (move_x_fmt == 0) move_x_bytes = (int)(sizeof(float) * (size_t)move_cx);
    if (move_y_fmt == 0) move_y_bytes = (int)(sizeof(float) * (size_t)move_cy);

    int fired_cap = 0;
    int key_cap = 0;
    for (int i = 0; i < objects.count; i++) {
        if ((objects.id[i] == MOVE_TRIGGER || objects.id[i] == ROTATE_TRIGGER) && GET_ACTIVATED(i))
            fired_cap++;
        else if (objects.id[i] == KEY_OBJ && (objects.flags[i] & FLAG_TOGGLED)
                 && (objects.activated[i] & 3))
            key_cap++;
    }

    int toggle_cap = 0;
    for (int g = 1; g < MAX_GROUPS; g++) {
        GroupNode *p = get_group(g);
        if (p && p->toggled) toggle_cap++;
    }

    int alpha_group_cap = 0;
    for (int g = 1; g < MAX_GROUPS; g++) {
        GroupNode *p = get_group(g);
        if (p && p->alpha != 1.0f) alpha_group_cap++;
    }

    int pulse_cap = 0;
    for (int i = 0; i < pulse_trigger_count; i++)
        if (pulse_trigger_buffer[i].active) pulse_cap++;

    int rotate_targets_total = 0;
    for (int i = 0; i < rotate_trigger_count; i++)
        rotate_targets_total += rotate_trigger_buffer[i].target_count;

    int rotate_obj_cap = 0;
    for (int i = 0; i < objects.count; i++) {
        if (objects.id[i] != ROTATE_TRIGGER) continue;
        RotateTrigger *rt = get_rotate_trigger(i);
        GroupNode *target_group = get_group(rt->target_group);
        GroupNode *center_group = get_group(rt->center_group);
        if (target_group) rotate_obj_cap += target_group->count;
        if (center_group) rotate_obj_cap += center_group->count;
    }

    int pickup_cap = 0;
    for (int i = 0; i < objects.count; i++) {
        if (is_collected_pickup(i)) pickup_cap++;
    }

    void *ck_ptr[24];
    size_t ck_size[24];
    int ck_n = 0;
    size_t total = 0;
    void *base = NULL;
#define CK_ADD(pp, type, count) do { \
        if ((count) > 0) { \
            (pp) = NULL; \
            ck_ptr[ck_n] = (void *)&(pp); \
            ck_size[ck_n] = CK_ALIGN8(sizeof(type) * (size_t)(count)); \
            total += ck_size[ck_n]; \
            ck_n++; \
        } else { \
            (pp) = NULL; \
        } \
    } while (0)

    CK_ADD(check->move_triggers,     MoveTriggerBuffer,  move_trigger_count);
    CK_ADD(check->alpha_triggers,    AlphaTriggerBuffer, alpha_trigger_count);
    CK_ADD(check->spawn_triggers,    SpawnTriggerBuffer, spawn_trigger_count);
    CK_ADD(check->follow_triggers,   FollowPlayerYTriggerBuffer, follow_player_y_trigger_count);
    CK_ADD(check->follow_trigger_buffers, FollowTriggerBuffer, follow_trigger_count);
    CK_ADD(check->rotate_triggers,   RotateTriggerBuffer, rotate_trigger_count);
    CK_ADD(check->rotate_targets,    RotateTargetState,   rotate_targets_total);
    CK_ADD(check->rotate_target_offsets, int,            rotate_trigger_count);
    CK_ADD(check->rotate_obj_snapshot, RotateObjectSnapshot, rotate_obj_cap);
    CK_ADD(check->count_items,       ItemCount,           item_counter_count);
    CK_ADD(check->count_triggers,    CountTriggerBuffer,  count_trigger_count);
    CK_ADD(check->touch_triggers,    TouchTriggerBuffer,  touch_trigger_count);
    CK_ADD(check->collected_pickups, int,                 pickup_cap);
    CK_ADD(check->channel_snapshot,  ColorChannel,       used_count);
    CK_ADD(check->trigger_snapshot,  ColTriggerBuffer,   used_count);
    CK_ADD(check->move_idx_stream,   uint8_t,            move_idx_bytes);
    CK_ADD(check->move_obj_x,        uint8_t,            move_x_bytes);
    CK_ADD(check->move_obj_y,        uint8_t,            move_y_bytes);
    CK_ADD(check->fired_move_index,  int,                fired_cap);
    CK_ADD(check->key_index,         int,                key_cap);
    CK_ADD(check->toggle_group_id,   int,                toggle_cap);
    CK_ADD(check->alpha_group_id,    int,                alpha_group_cap);
    CK_ADD(check->alpha_group_alpha, float,              alpha_group_cap);
    CK_ADD(check->pulse_snap,        PulseSnapshot,      pulse_cap);
#undef CK_ADD

    if (total > 0) {
        base = malloc(total);
        if (!base) {
            free(mv_list);
            free(ck_hx);
            free(ck_hy);
            free_checkpoint_snapshot(check);
            return;
        }
        size_t off = 0;
        for (int k = 0; k < ck_n; k++) {
            *(void **)ck_ptr[k] = (char *)base + off;
            off += ck_size[k];
        }
    }
    check->alloc_base = base;

    checkpoint_pointer = next_checkpoint;
    if (++checkpoint_count > MAX_CHECKPOINTS) checkpoint_count = MAX_CHECKPOINTS;

    check->move_triggers_count  = move_trigger_count;
    check->alpha_triggers_count = alpha_trigger_count;
    check->spawn_triggers_count = spawn_trigger_count;
    check->follow_triggers_count = follow_player_y_trigger_count;
    check->follow_trigger_buffers_count = follow_trigger_count;
    check->rotate_triggers_count = rotate_trigger_count;
    check->rotate_targets_count = rotate_targets_total;
    check->rotate_obj_count = 0;
    check->count_items_count = item_counter_count;
    check->count_triggers_count = count_trigger_count;
    check->touch_triggers_count = touch_trigger_count;
    check->collected_pickups_count = pickup_cap;
    check->channel_snapshot_count = used_count;

    check->camera_x = state.camera_x;
    check->camera_y = state.camera_y;

    check->p1 = state.player;

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

    check->show_player = state.show_player;

    check->speed = state.speed;

    check->current_fading_effect = current_fading_effect;
    check->p1_trail = p1_trail;

    check->wall_y = level_info.wall_y;

    check->song_offset = level_info.song_offset + state.player.timeElapsed;

    // snapshot only the channels used by this level
    for (int i = 0; i < used_count; i++) {
        int idx = channels_used[i];
        check->channel_snapshot[i] = channels[idx];
        check->trigger_snapshot[i] = col_trigger_buffer[idx];
    }

    if (check->move_triggers)  memcpy(check->move_triggers,  move_trigger_buffer,  sizeof(MoveTriggerBuffer)  * move_trigger_count);
    if (check->alpha_triggers) memcpy(check->alpha_triggers, alpha_trigger_buffer, sizeof(AlphaTriggerBuffer) * alpha_trigger_count);
    if (check->spawn_triggers) memcpy(check->spawn_triggers, spawn_trigger_buffer, sizeof(SpawnTriggerBuffer) * spawn_trigger_count);
    if (check->follow_triggers) memcpy(check->follow_triggers, follow_player_y_trigger_buffer, sizeof(FollowPlayerYTriggerBuffer) * follow_player_y_trigger_count);
    if (check->follow_trigger_buffers) memcpy(check->follow_trigger_buffers, follow_trigger_buffer, sizeof(FollowTriggerBuffer) * follow_trigger_count);
    if (check->rotate_triggers) memcpy(check->rotate_triggers, rotate_trigger_buffer, sizeof(RotateTriggerBuffer) * rotate_trigger_count);
    if (check->count_items) memcpy(check->count_items, item_counter_buffer, sizeof(ItemCount) * item_counter_count);
    if (check->count_triggers) memcpy(check->count_triggers, count_trigger_buffer, sizeof(CountTriggerBuffer) * count_trigger_count);
    if (check->touch_triggers) memcpy(check->touch_triggers, touch_trigger_buffer, sizeof(TouchTriggerBuffer) * touch_trigger_count);
    if (check->collected_pickups) {
        int n = 0;
        for (int i = 0; i < objects.count && n < check->collected_pickups_count; i++) {
            if (is_collected_pickup(i)) check->collected_pickups[n++] = i;
        }
    }

    if (check->rotate_triggers) {
        int offset = 0;
        for (int i = 0; i < rotate_trigger_count; i++) {
            check->rotate_target_offsets[i] = offset;
            int count = rotate_trigger_buffer[i].target_count;
            if (count > 0 && rotate_trigger_buffer[i].targets) {
                memcpy(check->rotate_targets + offset, rotate_trigger_buffer[i].targets, sizeof(RotateTargetState) * count);
            }
            offset += count;
        }
    }

    if (check->rotate_obj_snapshot) {
        int n = 0;
        for (int i = 0; i < objects.count && n < rotate_obj_cap; i++) {
            if (objects.id[i] != ROTATE_TRIGGER) continue;
            RotateTrigger *rt = get_rotate_trigger(i);
            GroupNode *target_group = get_group(rt->target_group);
            GroupNode *center_group = get_group(rt->center_group);
            if (target_group) {
                for (int j = 0; j < target_group->count && n < rotate_obj_cap; j++) {
                    int obj = target_group->objects[j];
                    check->rotate_obj_snapshot[n].obj = obj;
                    check->rotate_obj_snapshot[n].rotation = objects.rotation[obj];
                    check->rotate_obj_snapshot[n].visual_rotation = objects.visual_rotation[obj];
                    n++;
                }
            }
            if (center_group) {
                for (int j = 0; j < center_group->count && n < rotate_obj_cap; j++) {
                    int obj = center_group->objects[j];
                    check->rotate_obj_snapshot[n].obj = obj;
                    check->rotate_obj_snapshot[n].rotation = objects.rotation[obj];
                    check->rotate_obj_snapshot[n].visual_rotation = objects.visual_rotation[obj];
                    n++;
                }
            }
        }
        check->rotate_obj_count = n;
    }

    check->move_obj_count = 0;
    check->move_idx_bytes = 0;
    check->move_cx = 0;
    check->move_cy = 0;
    check->move_x_fmt = move_x_fmt;
    check->move_x_u = move_x_u;
    check->move_x_bytes = move_x_bytes;
    check->move_y_fmt = move_y_fmt;
    check->move_y_u = move_y_u;
    check->move_y_bytes = move_y_bytes;
    if (move_cap > 0) {
        int n = 0, kx = 0, ky = 0, prev = 0, scan = 0, tux = 0, tuy = 0;
        if (ck_hx && move_x_fmt != 0) memset(ck_hx, 0, sizeof(int) * ck_hx_slots);
        if (ck_hy && move_y_fmt != 0) memset(ck_hy, 0, sizeof(int) * ck_hy_slots);
        uint8_t *sp = check->move_idx_stream;
        float *tblx = (float *)check->move_obj_x;
        float *tbly = (float *)check->move_obj_y;
        uint8_t  *ix8x  = check->move_obj_x ? (uint8_t  *)(check->move_obj_x + sizeof(float) * (size_t)move_x_u) : NULL;
        uint16_t *ix16x = check->move_obj_x ? (uint16_t *)(check->move_obj_x + sizeof(float) * (size_t)move_x_u) : NULL;
        uint8_t  *ix8y  = check->move_obj_y ? (uint8_t  *)(check->move_obj_y + sizeof(float) * (size_t)move_y_u) : NULL;
        uint16_t *ix16y = check->move_obj_y ? (uint16_t *)(check->move_obj_y + sizeof(float) * (size_t)move_y_u) : NULL;
        for (int e = 0; e < move_cap; e++) {
            int i;
            if (mv_list) {
                i = mv_list[e];
            } else {
                do { i = scan++; } while (i < objects.count &&
                        !(ck_float_bits(objects.x[i]) ^ ck_float_bits(objects.original_x[i])) &&
                        !(ck_float_bits(objects.y[i]) ^ ck_float_bits(objects.original_y[i])));
            }
            unsigned int bx = ck_float_bits(objects.x[i]) ^ ck_float_bits(objects.original_x[i]);
            unsigned int by = ck_float_bits(objects.y[i]) ^ ck_float_bits(objects.original_y[i]);
            unsigned int gap = (unsigned int)(n ? (i - prev) : i);
            unsigned int flags = (bx ? 2u : 0u) | (by ? 1u : 0u);
            sp = ck_varint_put(sp, (gap << 2) | flags);
            prev = i;
            if (bx) {
                float v = objects.x[i];
                if (move_x_fmt == 0) {
                    tblx[kx] = v;
                } else {
                    int ix = ck_dedup_put_h(ck_hx, ck_hx_slots - 1u, tblx, &tux, v);
                    if (move_x_fmt == 1) ix8x[kx] = (uint8_t)ix; else ix16x[kx] = (uint16_t)ix;
                }
                kx++;
            }
            if (by) {
                float v = objects.y[i];
                if (move_y_fmt == 0) {
                    tbly[ky] = v;
                } else {
                    int iy = ck_dedup_put_h(ck_hy, ck_hy_slots - 1u, tbly, &tuy, v);
                    if (move_y_fmt == 1) ix8y[ky] = (uint8_t)iy; else ix16y[ky] = (uint16_t)iy;
                }
                ky++;
            }
            n++;
        }
        check->move_obj_count = n;
        check->move_idx_bytes = (int)(sp - check->move_idx_stream);
        check->move_cx = kx;
        check->move_cy = ky;
    }
    free(mv_list);
    mv_list = NULL;
    free(ck_hx);
    ck_hx = NULL;
    free(ck_hy);
    ck_hy = NULL;

    check->fired_move_count = 0;
    if (fired_cap > 0) {
        int n = 0;
        for (int i = 0; i < objects.count; i++) {
            if (n >= fired_cap) break;
            if ((objects.id[i] == MOVE_TRIGGER || objects.id[i] == ROTATE_TRIGGER) && GET_ACTIVATED(i))
                check->fired_move_index[n++] = i;
        }
        check->fired_move_count = n;
    }

    check->key_count = 0;
    if (key_cap > 0) {
        int n = 0;
        for (int i = 0; i < objects.count; i++) {
            if (n >= key_cap) break;
            if (objects.id[i] == KEY_OBJ && (objects.flags[i] & FLAG_TOGGLED)
                && (objects.activated[i] & 3))
                check->key_index[n++] = i;
        }
        check->key_count = n;
    }

    check->toggle_group_count = 0;
    if (toggle_cap > 0) {
        int n = 0;
        for (int g = 1; g < MAX_GROUPS && n < toggle_cap; g++) {
            GroupNode *p = get_group(g);
            if (p && p->toggled) check->toggle_group_id[n++] = g;
        }
        check->toggle_group_count = n;
    }

    check->alpha_group_count = 0;
    if (alpha_group_cap > 0) {
        int n = 0;
        for (int g = 1; g < MAX_GROUPS && n < alpha_group_cap; g++) {
            GroupNode *p = get_group(g);
            if (p && p->alpha != 1.0f) {
                check->alpha_group_id[n] = g;
                check->alpha_group_alpha[n] = p->alpha;
                n++;
            }
        }
        check->alpha_group_count = n;
    }

    check->pulse_snap_count = 0;
    if (pulse_cap > 0) {
        int n = 0;
        for (int i = 0; i < pulse_trigger_count && n < pulse_cap; i++) {
            PulseTriggerBuffer *pb = &pulse_trigger_buffer[i];
            if (!pb->active) continue;
            check->pulse_snap[n].source_obj = pb->source_obj;
            check->pulse_snap[n].time_run = pb->time_run;
            n++;
        }
        check->pulse_snap_count = n;
    }

    set_checkpoint_timer(AUTO_CHECKPOINT_TIME);
}

void restore_checkpoint() {
    CheckpointData *check = &checkpoints[checkpoint_pointer];
    discard_group_combination_visual_moves();

    state.camera_x = check->camera_x;
    state.camera_y = check->camera_y;

    state.player = check->p1;
    state.player2 = check->p2;
    state.player.platform_velocity = state.player.last_platform_velocity = 0.f;
    state.player2.platform_velocity = state.player2.last_platform_velocity = 0.f;

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
    
    state.show_player = check->show_player;

    state.speed = check->speed;

    level_info.wall_y = check->wall_y;

    current_fading_effect = check->current_fading_effect;
    p1_trail = check->p1_trail;
    
    if (settingsState.practiceMusicSync) seek_mp3(check->song_offset);
    
    for (int i = 0; i < check->channel_snapshot_count; i++) {
        int idx = channels_used[i];
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

    if (check->follow_triggers && check->follow_triggers_count > 0) {
        FollowPlayerYTriggerBuffer *buf = realloc(follow_player_y_trigger_buffer, sizeof(FollowPlayerYTriggerBuffer) * check->follow_triggers_count);
        if (buf) {
            follow_player_y_trigger_buffer = buf;
            follow_player_y_trigger_capacity = check->follow_triggers_count;
            follow_player_y_trigger_count = check->follow_triggers_count;
            memcpy(follow_player_y_trigger_buffer, check->follow_triggers, sizeof(FollowPlayerYTriggerBuffer) * check->follow_triggers_count);
        }
    }

    follow_trigger_count = 0;
    if (check->follow_trigger_buffers && check->follow_trigger_buffers_count > 0) {
        FollowTriggerBuffer *buf = realloc(follow_trigger_buffer, sizeof(FollowTriggerBuffer) * check->follow_trigger_buffers_count);
        if (buf) {
            follow_trigger_buffer = buf;
            follow_trigger_capacity = check->follow_trigger_buffers_count;
            follow_trigger_count = check->follow_trigger_buffers_count;
            memcpy(follow_trigger_buffer, check->follow_trigger_buffers, sizeof(FollowTriggerBuffer) * check->follow_trigger_buffers_count);
        }
    }

    for (int i = 0; i < rotate_trigger_count; i++) {
        free(rotate_trigger_buffer[i].targets);
        rotate_trigger_buffer[i].targets = NULL;
    }
    rotate_trigger_count = 0;
    if (check->rotate_triggers && check->rotate_triggers_count > 0) {
        RotateTriggerBuffer *buf = realloc(rotate_trigger_buffer, sizeof(RotateTriggerBuffer) * check->rotate_triggers_count);
        if (buf) {
            rotate_trigger_buffer = buf;
            rotate_trigger_capacity = check->rotate_triggers_count;
            rotate_trigger_count = check->rotate_triggers_count;
            memcpy(rotate_trigger_buffer, check->rotate_triggers, sizeof(RotateTriggerBuffer) * check->rotate_triggers_count);
            for (int i = 0; i < rotate_trigger_count; i++) {
                RotateTriggerBuffer *rb = &rotate_trigger_buffer[i];
                int count = rb->target_count;
                int offset = check->rotate_target_offsets[i];
                rb->targets = NULL;
                if (count > 0 && offset >= 0 && offset + count <= check->rotate_targets_count) {
                    rb->targets = malloc(sizeof(RotateTargetState) * count);
                    if (rb->targets) {
                        memcpy(rb->targets, check->rotate_targets + offset, sizeof(RotateTargetState) * count);
                    } else {
                        rb->target_count = 0;
                        rb->active = false;
                    }
                } else {
                    rb->target_count = 0;
                }
            }
        }
    }

    for (int i = 0; i < objects.count; i++) {
        PickupItem *pickup = get_pickup_item(i);
        if (pickup) {
            objects.flags[i] &= ~FLAG_TOGGLED;
            objects.activated[i] = 0;
        }
    }
    for (int i = 0; i < check->collected_pickups_count; i++) {
        int obj = check->collected_pickups[i];
        if (obj >= 0 && obj < objects.count && is_checkpoint_pickup(obj)) {
            objects.flags[obj] |= FLAG_TOGGLED;
            objects.activated[obj] = 3;
        }
    }

    if (check->count_items_count > 0) {
        ItemCount *buf = realloc(item_counter_buffer, sizeof(ItemCount) * check->count_items_count);
        if (buf) {
            item_counter_buffer = buf;
            item_counter_capacity = check->count_items_count;
            item_counter_count = check->count_items_count;
            memcpy(item_counter_buffer, check->count_items, sizeof(ItemCount) * check->count_items_count);
        }
    } else {
        item_counter_count = 0;
    }
    if (check->count_triggers_count > 0) {
        CountTriggerBuffer *buf = realloc(count_trigger_buffer, sizeof(CountTriggerBuffer) * check->count_triggers_count);
        if (buf) {
            count_trigger_buffer = buf;
            count_trigger_capacity = check->count_triggers_count;
            count_trigger_count = check->count_triggers_count;
            memcpy(count_trigger_buffer, check->count_triggers, sizeof(CountTriggerBuffer) * check->count_triggers_count);
        }
    } else {
        count_trigger_count = 0;
    }
    if (check->touch_triggers_count > 0) {
        TouchTriggerBuffer *buf = realloc(touch_trigger_buffer, sizeof(TouchTriggerBuffer) * check->touch_triggers_count);
        if (buf) {
            touch_trigger_buffer = buf;
            touch_trigger_capacity = check->touch_triggers_count;
            touch_trigger_count = check->touch_triggers_count;
            memcpy(touch_trigger_buffer, check->touch_triggers, sizeof(TouchTriggerBuffer) * check->touch_triggers_count);
        }
    } else {
        touch_trigger_count = 0;
    }

    for (int i = 0; i < count_trigger_count; i++)
        count_trigger_buffer[i].restored_from_checkpoint = count_trigger_buffer[i].active;

    for (int i = 0; i < move_trigger_count; i++)
        move_trigger_buffer[i].restored_from_checkpoint = move_trigger_buffer[i].active;
    for (int i = 0; i < alpha_trigger_count; i++)
        alpha_trigger_buffer[i].restored_from_checkpoint = alpha_trigger_buffer[i].active;
    for (int i = 0; i < spawn_trigger_count; i++)
        spawn_trigger_buffer[i].restored_from_checkpoint = spawn_trigger_buffer[i].active;
    for (int i = 0; i < follow_player_y_trigger_count; i++)
        follow_player_y_trigger_buffer[i].restored_from_checkpoint = follow_player_y_trigger_buffer[i].active;
    for (int i = 0; i < follow_trigger_count; i++)
        follow_trigger_buffer[i].restored_from_checkpoint = follow_trigger_buffer[i].active;
    for (int i = 0; i < rotate_trigger_count; i++)
        rotate_trigger_buffer[i].restored_from_checkpoint = rotate_trigger_buffer[i].active;

    {
        int kx = 0, ky = 0, oi = 0;
        const uint8_t *sp = check->move_idx_stream;
        const uint8_t *sp_end = sp + check->move_idx_bytes;
        for (int i = 0; i < check->move_obj_count; i++) {
            if (sp >= sp_end) break;
            unsigned int v = ck_varint_get(&sp);
            unsigned int gap = v >> 2;
            unsigned int flags = v & 3u;
            oi = (i == 0) ? (int)gap : (oi + (int)gap);
            if (flags & 2u) {
                float vx = ck_axis_get(check->move_obj_x, check->move_x_fmt, check->move_x_u, kx++);
                objects.x[oi] = vx;
            }
            if (flags & 1u) {
                float vy = ck_axis_get(check->move_obj_y, check->move_y_fmt, check->move_y_u, ky++);
                objects.y[oi] = vy; objects.last_y[oi] = vy;
            }
            update_object_section(oi);
        }
    }
    for (int i = 0; i < check->rotate_obj_count && check->rotate_obj_snapshot; i++) {
        RotateObjectSnapshot *rs = &check->rotate_obj_snapshot[i];
        objects.last_y[rs->obj] = objects.y[rs->obj];
        objects.rotation[rs->obj] = rs->rotation;
        objects.visual_rotation[rs->obj] = rs->visual_rotation;
    }
    for (int i = 0; i < check->alpha_group_count; i++) {
        GroupNode *p = get_group(check->alpha_group_id[i]);
        if (p) p->alpha = check->alpha_group_alpha[i];
    }
    for (int i = 0; i < check->alpha_group_count; i++) {
        GroupNode *p = get_group(check->alpha_group_id[i]);
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.alpha_trigger_opacity[object] = get_group_opacities(object);
        }
    }

    for (int i = 0; i < check->toggle_group_count; i++) {
        GroupNode *p = get_group(check->toggle_group_id[i]);
        if (!p) continue;
        p->toggled = true;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.flags[object] |= FLAG_TOGGLED;
        }
    }

    for (int i = 0; i < check->fired_move_count; i++) {
        SET_ACTIVATED(check->fired_move_index[i], true);
    }

    for (int i = 0; i < check->key_count; i++) {
        SET_ACTIVATED(check->key_index[i], true);
        objects.flags[check->key_index[i]] |= FLAG_TOGGLED;
    }

    pulse_trigger_count = 0;
    for (int i = 0; i < objects.count; i++) {
        objects.num_main_pulses[i] = 0;
        objects.num_detail_pulses[i] = 0;
        objects.main_being_pulsed[i] = false;
        objects.detail_being_pulsed[i] = false;
    }
    for (int i = 0; i < COL_CHANNEL_NUM; i++) {
        channels[i].num_pulses = 0;
    }

    for (int i = 0; i < check->pulse_snap_count; i++) {
        PulseSnapshot *ps = &check->pulse_snap[i];
        restore_pulse_trigger(ps->source_obj, ps->time_run);
    }

    update_attempt_text_pos();

    set_checkpoint_timer(AUTO_CHECKPOINT_TIME);
    restored_checkpoint = true;
}

void delete_last_checkpoint() {
    if (checkpoint_count > 0) {
        free_checkpoint_snapshot(&checkpoints[checkpoint_pointer]);
        checkpoint_count--;

        // Wrap around pointer
        if (checkpoint_pointer-- == 0) {
            checkpoint_pointer = MAX_CHECKPOINTS - 1;
        }
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
    clear_practice_mode();

    if (settingsState.practiceMusicSync) {
        seek_mp3(level_info.song_offset);
    } else {
        stop_mp3();
        play_level_song(level_info.song_offset);
    }
}

void update_checkpoint_with_p2() {
    // Exit if not in practice mode with autocheckpoints enabled
    if (!state.practice_mode || state.end_wall_anim_playing) return;

    if (placed_checkpoint) {
        placed_checkpoint->p2 = state.player2;
        placed_checkpoint = NULL;
    }
}

void handle_auto_checkpoints(float delta) {
    // Exit if not in practice mode with autocheckpoints enabled
    if (!(state.practice_mode && settingsState.autoCheckpoints) || state.end_wall_anim_playing) return;
    
    if (checkpoint_timer <= 0) {
        switch (state.player.gamemode) {
            case GAMEMODE_PLAYER:
            case GAMEMODE_BALL:
            case GAMEMODE_ROBOT:
            case GAMEMODE_SPIDER:
                if (state.player.landed_from_jump) {
                    new_checkpoint();
                    state.just_placed_auto_checkpoint = true;
                }
                break;

            case GAMEMODE_SHIP:
            case GAMEMODE_UFO:
            case GAMEMODE_WAVE:
                new_checkpoint();
                state.just_placed_auto_checkpoint = true;
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
        update_checkpoint_with_p2();
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
