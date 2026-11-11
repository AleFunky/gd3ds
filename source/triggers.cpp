#include <citro2d.h>
#include "triggers.hpp"
#include "arena.hpp"
#include "color.h"
#include "icons.hpp"
#include "math_helpers.hpp"
#include <float.h>
#include <math.h>
#include "level_loading.hpp"
#include "main.hpp"
#include "graphics.hpp"
#include "groups.hpp"
#include "easing.hpp"
#include "objects.hpp"
#include "player/collision.hpp"

#include <stdlib.h>
#include <string.h>

#include "player/player.hpp"
#include "player/trail.hpp"
#include "state.hpp"

#define LAUNCH_CAP 1080.f

Color p1_color;
Color p2_color;
Color glow_color;

float trigger_dt = 0.f;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];
AlphaTriggerBuffer *alpha_trigger_buffer = NULL;
int alpha_trigger_count = 0;
int alpha_trigger_capacity = 0;

MoveTriggerBuffer *move_trigger_buffer = NULL;
int move_trigger_count = 0;
int move_trigger_capacity = 0;
CountTriggerBuffer *count_trigger_buffer = NULL;
int count_trigger_count = 0;
int count_trigger_capacity = 0;
TouchTriggerBuffer *touch_trigger_buffer = NULL;
int touch_trigger_count = 0;
int touch_trigger_capacity = 0;
ItemCount *item_counter_buffer = NULL;
int item_counter_count = 0;
int item_counter_capacity = 0;

SpawnTriggerBuffer *spawn_trigger_buffer = NULL;
int spawn_trigger_count = 0;
int spawn_trigger_capacity = 0;

RotateTriggerBuffer *rotate_trigger_buffer = NULL;
int rotate_trigger_count = 0;
int rotate_trigger_capacity = 0;
FollowPlayerYTriggerBuffer *follow_player_y_trigger_buffer = NULL;
int follow_player_y_trigger_count = 0;
int follow_player_y_trigger_capacity = 0;

FollowTriggerBuffer *follow_trigger_buffer = NULL;
int follow_trigger_count = 0;
int follow_trigger_capacity = 0;

PulseTriggerBuffer *pulse_trigger_buffer = NULL;
int pulse_trigger_count = 0;
int pulse_trigger_capacity = 0;

OnDeathEntry on_death_entries[ON_DEATH_MAX];
int on_death_count = 0;

AlphaTriggerBuffer *get_new_alpha_trigger() {
    if (alpha_trigger_count >= alpha_trigger_capacity) {
        size_t new_capacity = alpha_trigger_capacity == 0 ? 32 : alpha_trigger_capacity + 32;

        AlphaTriggerBuffer *new_list = (AlphaTriggerBuffer *)realloc(alpha_trigger_buffer, new_capacity * sizeof(AlphaTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        alpha_trigger_buffer = new_list;
        alpha_trigger_capacity = new_capacity;
    }

    AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[alpha_trigger_count++];
    memset(buffer, 0, sizeof(AlphaTriggerBuffer));
    return buffer;
}

MoveTriggerBuffer *get_new_move_trigger() {
    if (move_trigger_count >= move_trigger_capacity) {
        size_t new_capacity = move_trigger_capacity == 0 ? 32 : move_trigger_capacity + 32;

        MoveTriggerBuffer *new_list = (MoveTriggerBuffer *)realloc(move_trigger_buffer, new_capacity * sizeof(MoveTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        move_trigger_buffer = new_list;
        move_trigger_capacity = new_capacity;
    }
    MoveTriggerBuffer *buffer = &move_trigger_buffer[move_trigger_count++];
    memset(buffer, 0, sizeof(MoveTriggerBuffer));
    return buffer;
}

static CountTriggerBuffer *get_new_count_trigger(void) {
    if (count_trigger_count >= count_trigger_capacity) {
        size_t new_capacity = count_trigger_capacity == 0 ? 32 : count_trigger_capacity + 32;
        CountTriggerBuffer *new_list = (CountTriggerBuffer *)realloc(count_trigger_buffer, new_capacity * sizeof(CountTriggerBuffer));
        if (!new_list) return NULL;
        count_trigger_buffer = new_list;
        count_trigger_capacity = new_capacity;
    }
    CountTriggerBuffer *buffer = &count_trigger_buffer[count_trigger_count++];
    memset(buffer, 0, sizeof(CountTriggerBuffer));
    return buffer;
}

static TouchTriggerBuffer *get_new_touch_trigger(void) {
    if (touch_trigger_count >= touch_trigger_capacity) {
        size_t new_capacity = touch_trigger_capacity == 0 ? 32 : touch_trigger_capacity + 32;
        TouchTriggerBuffer *new_list = (TouchTriggerBuffer *)realloc(touch_trigger_buffer, new_capacity * sizeof(TouchTriggerBuffer));
        if (!new_list) return NULL;
        touch_trigger_buffer = new_list;
        touch_trigger_capacity = new_capacity;
    }
    TouchTriggerBuffer *buffer = &touch_trigger_buffer[touch_trigger_count++];
    memset(buffer, 0, sizeof(TouchTriggerBuffer));
    return buffer;
}

static ItemCount *find_item_count(int id) {
    for (int i = 0; i < item_counter_count; i++) {
        if (item_counter_buffer[i].id == id) return &item_counter_buffer[i];
    }
    return NULL;
}

static ItemCount *get_new_item_count(int id) {
    if (item_counter_count >= item_counter_capacity) {
        size_t new_capacity = item_counter_capacity == 0 ? 16 : item_counter_capacity + 16;
        ItemCount *new_list = (ItemCount *)realloc(item_counter_buffer, new_capacity * sizeof(ItemCount));
        if (!new_list) return NULL;
        item_counter_buffer = new_list;
        item_counter_capacity = new_capacity;
    }
    ItemCount *slot = &item_counter_buffer[item_counter_count++];
    slot->id = id;
    slot->value = 0;
    return slot;
}

int get_item_count(int id) {
    ItemCount *slot = find_item_count(id);
    return slot ? slot->value : 0;
}

void add_item_count(int id, int delta) {
    if (id < 0 || id > 9999) return;
    ItemCount *slot = find_item_count(id);
    if (!slot) slot = get_new_item_count(id);
    if (slot) slot->value += delta;
}

FollowPlayerYTriggerBuffer *get_new_follow_player_y_trigger() {
    if (follow_player_y_trigger_count >= follow_player_y_trigger_capacity) {
        size_t new_capacity = follow_player_y_trigger_capacity == 0 ? 32 : follow_player_y_trigger_capacity + 32;

        FollowPlayerYTriggerBuffer *new_list = (FollowPlayerYTriggerBuffer *)realloc(follow_player_y_trigger_buffer, new_capacity * sizeof(FollowPlayerYTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        follow_player_y_trigger_buffer = new_list;
        follow_player_y_trigger_capacity = new_capacity;
    }
    FollowPlayerYTriggerBuffer *buffer = &follow_player_y_trigger_buffer[follow_player_y_trigger_count++];
    memset(buffer, 0, sizeof(FollowPlayerYTriggerBuffer));
    return buffer;
}

FollowTriggerBuffer *get_new_follow_trigger() {
    if (follow_trigger_count >= follow_trigger_capacity) {
        size_t new_capacity = follow_trigger_capacity == 0 ? 32 : follow_trigger_capacity + 32;

        FollowTriggerBuffer *new_list = (FollowTriggerBuffer *)realloc(follow_trigger_buffer, new_capacity * sizeof(FollowTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        follow_trigger_buffer = new_list;
        follow_trigger_capacity = new_capacity;
    }
    FollowTriggerBuffer *buffer = &follow_trigger_buffer[follow_trigger_count++];
    memset(buffer, 0, sizeof(FollowTriggerBuffer));
    return buffer;
}

SpawnTriggerBuffer *get_new_spawn_trigger() {
    if (spawn_trigger_count >= spawn_trigger_capacity) {
        size_t new_capacity = spawn_trigger_capacity == 0 ? 32 : spawn_trigger_capacity + 32;

        SpawnTriggerBuffer *new_list = (SpawnTriggerBuffer *)realloc(spawn_trigger_buffer, new_capacity * sizeof(SpawnTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        spawn_trigger_buffer = new_list;
        spawn_trigger_capacity = new_capacity;
    }
    SpawnTriggerBuffer *buffer = &spawn_trigger_buffer[spawn_trigger_count++];
    memset(buffer, 0, sizeof(SpawnTriggerBuffer));
    return buffer;
}

RotateTriggerBuffer *get_new_rotate_trigger() {
    if (rotate_trigger_count >= rotate_trigger_capacity) {
        size_t new_capacity = rotate_trigger_capacity == 0 ? 32 : rotate_trigger_capacity + 32;

        RotateTriggerBuffer *new_list = (RotateTriggerBuffer *)realloc(rotate_trigger_buffer, new_capacity * sizeof(RotateTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        rotate_trigger_buffer = new_list;
        rotate_trigger_capacity = new_capacity;
    }

    RotateTriggerBuffer *buffer = &rotate_trigger_buffer[rotate_trigger_count++];
    memset(buffer, 0, sizeof(RotateTriggerBuffer));
    return buffer;
}

PulseTriggerBuffer *get_new_pulse_trigger() {
    if (pulse_trigger_count >= pulse_trigger_capacity) {
        size_t new_capacity = pulse_trigger_capacity == 0 ? 32 : pulse_trigger_capacity + 32;

        PulseTriggerBuffer *new_list = (PulseTriggerBuffer *)realloc(pulse_trigger_buffer, new_capacity * sizeof(PulseTriggerBuffer));
        if (!new_list) {
            return NULL;
        }

        pulse_trigger_buffer = new_list;
        pulse_trigger_capacity = new_capacity;
    }
    PulseTriggerBuffer *buffer = &pulse_trigger_buffer[pulse_trigger_count++];
    memset(buffer, 0, sizeof(PulseTriggerBuffer));
    return buffer;
}

typedef struct {
    bool saved;
    float rotation;
    float visual_rotation;
} RotateInitialRotation;

static RotateInitialRotation *rotate_initial_rotations = NULL;
static int rotate_initial_rotations_capacity = 0;

static void remember_rotate_initial_rotation(int obj) {
    if (obj < 0) return;
    if (obj >= rotate_initial_rotations_capacity) {
        int new_capacity = rotate_initial_rotations_capacity ? rotate_initial_rotations_capacity : 64;
        while (new_capacity <= obj) new_capacity *= 2;
        RotateInitialRotation *new_list = (RotateInitialRotation *)realloc(rotate_initial_rotations, sizeof(RotateInitialRotation) * new_capacity);
        if (!new_list) return;
        memset(new_list + rotate_initial_rotations_capacity, 0, sizeof(RotateInitialRotation) * (new_capacity - rotate_initial_rotations_capacity));
        rotate_initial_rotations = new_list;
        rotate_initial_rotations_capacity = new_capacity;
    }
    if (rotate_initial_rotations[obj].saved) return;
    rotate_initial_rotations[obj].saved = true;
    rotate_initial_rotations[obj].rotation = objects.rotation[obj];
    rotate_initial_rotations[obj].visual_rotation = objects.visual_rotation[obj];
}

static void restore_rotate_initial_rotations(void) {
    if (!rotate_initial_rotations) return;
    if (!objects.rotation || !objects.visual_rotation) {
        free(rotate_initial_rotations);
        rotate_initial_rotations = NULL;
        rotate_initial_rotations_capacity = 0;
        return;
    }
    int limit = objects.count < rotate_initial_rotations_capacity ? objects.count : rotate_initial_rotations_capacity;
    for (int i = 0; i < limit; i++) {
        if (!rotate_initial_rotations[i].saved) continue;
        objects.rotation[i] = rotate_initial_rotations[i].rotation;
        objects.visual_rotation[i] = rotate_initial_rotations[i].visual_rotation;
    }
}

void free_trigger_buffers() {
    restore_rotate_initial_rotations();
    free(alpha_trigger_buffer);
    free(move_trigger_buffer);
    free(count_trigger_buffer);
    free(touch_trigger_buffer);
    free(item_counter_buffer);
    free(spawn_trigger_buffer);
    free(follow_player_y_trigger_buffer);
    free(follow_trigger_buffer);

    if (rotate_trigger_buffer) {
        for (int i = 0; i < rotate_trigger_count; i++) {
            if (rotate_trigger_buffer[i].targets) { free(rotate_trigger_buffer[i].targets); rotate_trigger_buffer[i].targets = NULL; }
        }
    }
    free(rotate_trigger_buffer);

    free(pulse_trigger_buffer);

    alpha_trigger_buffer = NULL;
    move_trigger_buffer = NULL;
    count_trigger_buffer = NULL;
    touch_trigger_buffer = NULL;
    item_counter_buffer = NULL;
    spawn_trigger_buffer = NULL;
    follow_player_y_trigger_buffer = NULL;
    follow_trigger_buffer = NULL;
    rotate_trigger_buffer = NULL;
    pulse_trigger_buffer = NULL;

    alpha_trigger_count = 0;
    alpha_trigger_capacity = 0;

    move_trigger_count = 0;
    move_step_dt = 0.f;
    move_trigger_capacity = 0;

    count_trigger_count = 0;
    count_trigger_capacity = 0;
    touch_trigger_count = 0;
    touch_trigger_capacity = 0;
    item_counter_count = 0;
    item_counter_capacity = 0;

    follow_player_y_trigger_count = 0;
    follow_player_y_trigger_capacity = 0;

    follow_trigger_count = 0;
    follow_trigger_capacity = 0;

    spawn_trigger_count = 0;
    spawn_trigger_capacity = 0;

    rotate_trigger_count = 0;
    rotate_trigger_capacity = 0;

    pulse_trigger_count = 0;
    pulse_trigger_capacity = 0;
}

float move_lock_player_x_delta = 0.0f;
float move_lock_player_y_delta = 0.0f;
float move_step_dt = 0.0f;

static void player_follow_y_step(Player *player, float dt) {
    if (player->follow_y_index == 0 && player->follow_y_timer == 0.f) {
        for (int i = 0; i < FOLLOW_Y_SAMPLES; i++) player->follow_y[i] = player->y;
    }
    player->follow_y_timer += dt;
    if (player->follow_y_timer >= FOLLOW_Y_INTERVAL) {
        player->follow_y_timer -= FOLLOW_Y_INTERVAL;
        player->follow_y_index++;
    }
    player->follow_y[player->follow_y_index % FOLLOW_Y_SAMPLES] = player->y;
}

static float player_follow_y_old(Player *player, float time_ago) {
    if (time_ago <= 0.f) return player->y;
    int index = (int)floorf(time_ago / FOLLOW_Y_INTERVAL);
    if (index > FOLLOW_Y_SAMPLES - 1) index = FOLLOW_Y_SAMPLES - 1;
    int i = (player->follow_y_index - index) % FOLLOW_Y_SAMPLES;
    if (i < 0) i += FOLLOW_Y_SAMPLES;
    return player->follow_y[i];
}

static int toggle_spawn_buffer[8];
static int toggle_spawn_count = 0;
static bool toggle_spawn_active = false;

float get_group_opacities(int object) {
    float group_opacity = 1.f;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (!group) break;

        GroupNode *p = get_group(group);
        if (p) group_opacity *= p->alpha;
    }
    return group_opacity;
}

bool get_group_toggles(int object) {
    bool toggled = false;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (!group) break;

        GroupNode *p = get_group(group);
        if (p) toggled = toggled || p->toggled;
        if (toggled) break;
    }
    return toggled;
}

// Convert channel id to buffer index
int get_col_channel_index(int channel) {
    if (channel < 0 || channel >= COL_CHANNEL_NUM) {
        return 0;
    }
    return channel;
}

// Convert from buffer index to channel id
int get_col_channel_from_index(int index) {
    if (index < 0 || index >= COL_CHANNEL_NUM) {
        return 0;
    }
    return index;
}

int convert_one_point_nine_channel(int channel) {
    switch (channel) {
        case 1: return CHANNEL_P1;
        case 2: return CHANNEL_P2;
        case 3: return COL_1;
        case 4: return COL_2;
        case 5: return CHANNEL_LBG;
        case 6: return COL_3;
        case 7: return COL_4;
        case 8: return CHANNEL_3DL;
    }

    return channel;
}

Color HSV_combine(Color base, HSV hsv) {
    if (hsv.h == 0 && hsv.s == 0 && hsv.v == 0)
        return base;

    float r = base.r / 255.f;
    float g = base.g / 255.f;
    float b = base.b / 255.f;

    float cmax = fmaxf(fmaxf(r, g), b);
    float cmin = fminf(fminf(r, g), b);
    float delta = cmax - cmin;

    float hue = 0.f;
    if (delta != 0.f) {
        if (cmax == r) hue = 60.f * fmodf((g - b) / delta, 6.f);
        else if (cmax == g) hue = 60.f * ((b - r) / delta + 2.f);
        else hue = 60.f * ((r - g) / delta + 4.f);
    }
    if (hue < 0.f) hue += 360.f;

    float sat = (cmax == 0.f) ? 0.f : delta / cmax;
    float val = cmax;

    hue += hsv.h;
    if (hsv.sChecked) sat += hsv.s;
    else sat *= hsv.s;
    if (hsv.vChecked) val += hsv.v;
    else val *= hsv.v;

    while (hue < 0.f) hue += 360.f;
    while (hue >= 360.f) hue -= 360.f;
    sat = fminf(fmaxf(sat, 0.f), 1.f);
    val = fminf(fmaxf(val, 0.f), 1.f);

    if (sat == 0.f) {
        unsigned char v = (unsigned char)(val * 255.f);
        Color c = { v, v, v };
        return c;
    }

    float h = hue / 60.f;
    float hi = floorf(h);
    float f = h - hi;
    float p = val * (1.f - sat);
    float q = val * (1.f - sat * f);
    float t = val * (1.f - sat * (1.f - f));

    float rr, gg, bb;
    switch ((int)hi) {
        case 0: case 6: rr = val; gg = t;   bb = p; break;
        case 1:         rr = q;   gg = val; bb = p; break;
        case 2:         rr = p;   gg = val; bb = t; break;
        case 3:         rr = p;   gg = q;   bb = val; break;
        case 4:         rr = t;   gg = p;   bb = val; break;
        default:        rr = val; gg = p;   bb = q; break;
    }

    Color result = {
        (unsigned char)(fminf(rr, 1.f) * 255.f),
        (unsigned char)(fminf(gg, 1.f) * 255.f),
        (unsigned char)(fminf(bb, 1.f) * 255.f)
    };
    return result;
}

int trigger_pool_add(TriggerPool *pool, size_t element_size) {
    if (pool->count >= pool->capacity) {
        size_t new_capacity = pool->capacity ? pool->capacity * 2 : 8;
        void *new_data = arena_realloc(&level_arena, pool->data, pool->capacity * element_size, new_capacity * element_size);

        if (!new_data) {
            new_capacity = pool->capacity + 8;
            new_data = arena_realloc(&level_arena, pool->data, pool->capacity * element_size, new_capacity * element_size);
        }

        if (!new_data) return -1;

        pool->data = new_data;
        pool->capacity = new_capacity;
    }
    
    memset((char *)pool->data + pool->count * element_size, 0, element_size);
    return pool->count++;
}

void init_col_channels() {
    memset(col_trigger_buffer, 0, sizeof(col_trigger_buffer));

    channels[0].color.r = 0;
    channels[0].color.g = 0;
    channels[0].color.b = 0;
    channels[0].alpha = 1.0f;
    channels[0].blending = false;
    channels[0].copy_color_id = 0;
    memset(&channels[0].hsv, 0, sizeof(HSV));

    for (size_t chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        channels[chan].color.r = 255;
        channels[chan].color.g = 255;
        channels[chan].color.b = 255;
        channels[chan].non_pulse_color = channels[chan].color;
        channels[chan].alpha = 1.0f;
        channels[chan].blending = false;
        channels[chan].copy_color_id = 0;
        channels[chan].num_pulses = 0;
        memset(&channels[chan].hsv, 0, sizeof(HSV));
        col_trigger_buffer[chan].active = false;
    }

    int bg = get_col_channel_index(CHANNEL_BG);
    channels[bg].color.r = 0;
    channels[bg].color.g = 5;
    channels[bg].color.b = 100;
    
    int ground = get_col_channel_index(CHANNEL_GROUND);
    channels[ground].color.r = 40;
    channels[ground].color.g = 125;
    channels[ground].color.b = 255;
       
    int line = get_col_channel_index(CHANNEL_LINE);
    channels[line].color.r = 255;
    channels[line].color.g = 255;
    channels[line].color.b = 255;
    channels[line].blending = true;
    
    int obj = get_col_channel_index(CHANNEL_OBJ);
    channels[obj].color.r = 255;
    channels[obj].color.g = 255;
    channels[obj].color.b = 255;
    
    int obj_blend = get_col_channel_index(CHANNEL_OBJ_BLENDING);
    channels[obj_blend].color.r = 255;
    channels[obj_blend].color.g = 255;
    channels[obj_blend].color.b = 255;
    channels[obj_blend].blending = true;
    
    int threedl = get_col_channel_index(CHANNEL_3DL);
    channels[threedl].color.r = 255;
    channels[threedl].color.g = 255;
    channels[threedl].color.b = 255;
    
    int chn_p1 = get_col_channel_index(CHANNEL_P1);
    channels[chn_p1].color = get_p2_if_black(p1_color);
    channels[chn_p1].blending = true;
        
    int chn_p2 = get_col_channel_index(CHANNEL_P2);
    channels[chn_p2].color = get_p1_if_black(p2_color);
    channels[chn_p2].blending = true;
    
    int lbg = get_col_channel_index(CHANNEL_LBG);
    channels[lbg].color.r = 255;
    channels[lbg].color.g = 255;
    channels[lbg].color.b = 255;
    channels[lbg].blending = true;

    
    int black_chn = get_col_channel_index(CHANNEL_BLACK);
    channels[black_chn].color.r = 0;
    channels[black_chn].color.g = 0;
    channels[black_chn].color.b = 0;
    channels[black_chn].blending = false;

    int white_chn = get_col_channel_index(CHANNEL_WHITE);
    channels[white_chn].color.r = 255;
    channels[white_chn].color.g = 255;
    channels[white_chn].color.b = 255;
    channels[white_chn].blending = false;

    int yellow_glow = get_col_channel_index(CHANNEL_YELLOW_GLOW_INTERNAL);
    channels[yellow_glow].color.r = 255;
    channels[yellow_glow].color.g = 255;
    channels[yellow_glow].color.b = 0;
    channels[yellow_glow].blending = true;

    int blue_glow = get_col_channel_index(CHANNEL_BLUE_GLOW);
    channels[blue_glow].color.r = 0;
    channels[blue_glow].color.g = 255;
    channels[blue_glow].color.b = 255;
    channels[blue_glow].blending = true;

    int pink_glow = get_col_channel_index(CHANNEL_PINK_GLOW);
    channels[pink_glow].color.r = 255;
    channels[pink_glow].color.g = 0;
    channels[pink_glow].color.b = 255;
    channels[pink_glow].blending = true;
    
    int invis_glow = get_col_channel_index(CHANNEL_INVISIBLE_GLOW);
    channels[invis_glow].color.r = 255;
    channels[invis_glow].color.g = 255;
    channels[invis_glow].color.b = 255;
    channels[invis_glow].blending = true;
    
    int white_glow = get_col_channel_index(CHANNEL_WHITE_GLOW);
    channels[white_glow].color.r = 255;
    channels[white_glow].color.g = 255;
    channels[white_glow].color.b = 255;
    channels[white_glow].blending = true;

    for (int i = 0; i < COL_CHANNEL_NUM; i++)
        channels[i].non_pulse_color = channels[i].color;
}

void handle_col_channel(int chan) {
    int channel = get_col_channel_index(chan);

    if (channel == get_col_channel_index(CHANNEL_BLACK)) return;

    ColTriggerBuffer *buffer = &col_trigger_buffer[channel];

    if (buffer->active) {
        Color lerped_color;
        float lerped_alpha;
        Color color_to_lerp = buffer->new_color;

        if (buffer->copied_color_id > 0) {
            int src = get_col_channel_index(buffer->copied_color_id);
            color_to_lerp = channels[src].color;
            buffer->new_alpha = channels[src].alpha;
        }

        if (buffer->seconds > 0) {
            float multiplier = buffer->time_run / buffer->seconds;
            lerped_color = color_lerp(buffer->old_color, color_to_lerp, multiplier);
            lerped_alpha = (buffer->new_alpha - buffer->old_alpha) * multiplier + buffer->old_alpha;
        } else {
            lerped_color = color_to_lerp;
            lerped_alpha = buffer->new_alpha;
        }

        channels[channel].color = lerped_color;
        channels[channel].non_pulse_color = lerped_color;
        channels[channel].alpha = lerped_alpha;

        buffer->time_run += trigger_dt;

        if (buffer->time_run > buffer->seconds) {
            buffer->active = false;
            channels[channel].color = color_to_lerp;
            channels[channel].non_pulse_color = color_to_lerp;
            channels[channel].alpha = buffer->new_alpha;
        }
    }
}

ColTriggerBuffer *get_buffer(int chan) {
    return &col_trigger_buffer[get_col_channel_index(chan)];
}

void handle_col_triggers(void) {
    for (int chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        handle_col_channel(get_col_channel_from_index(chan));
    }
}

void handle_copy_channels() {
    for (int chan = 0; chan < COL_CHANNEL_NUM; chan++) {
        if (chan == get_col_channel_index(CHANNEL_BLACK)) continue;
        int copy_id = channels[chan].copy_color_id;
        if (copy_id > 0) {
            int src = get_col_channel_index(copy_id);
            Color color = channels[src].color;
            channels[chan].color = HSV_combine(color, channels[chan].hsv);
            channels[chan].non_pulse_color = channels[chan].color;
        }
    }
}

void upload_to_alpha_buffer(int obj) {
    AlphaTrigger *trigger = get_alpha_trigger(obj);
    int target_group = trigger->target_group;
    GroupNode *p = get_group(target_group);
    if (!p) return;

    if (trigger->trig_duration == 0) {
        float alpha = trigger->trigger_opacity;
        p->alpha = alpha;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.alpha_trigger_opacity[object] = get_group_opacities(object);
        }
        return;
    }

    int slot = -1;
    for (int i = 0; i < alpha_trigger_count; i++) {
        if (alpha_trigger_buffer[i].active && alpha_trigger_buffer[i].target_group == target_group) {
            slot = i;
            break;
        }
    }

    for (int i = 0; i < alpha_trigger_count; i++) {
        if (alpha_trigger_buffer[i].active &&
            alpha_trigger_buffer[i].target_group == target_group &&
            alpha_trigger_buffer[i].source_obj == obj &&
            alpha_trigger_buffer[i].restored_from_checkpoint) {
            alpha_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    AlphaTriggerBuffer *buffer;

    if (slot >= 0) {
        buffer = &alpha_trigger_buffer[slot];
    } else {
        buffer = get_new_alpha_trigger();
    }
    if (!buffer) return;

    buffer->target_group = target_group;
    buffer->source_obj = obj;
    buffer->new_alpha = trigger->trigger_opacity;
    buffer->old_alpha = p->alpha;
    buffer->seconds = trigger->trig_duration;
    buffer->time_run = 0;
    buffer->active = true;
}

void handle_alpha_triggers(void) {
    for (int slot = 0; slot < alpha_trigger_count; slot++) {
        AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[slot];
        if (!buffer->active) continue;

        float multiplier = buffer->time_run / buffer->seconds;
        float lerped = buffer->old_alpha + (buffer->new_alpha - buffer->old_alpha) * multiplier;

        GroupNode *p = get_group(buffer->target_group);
        if (p) {
            p->alpha = lerped;
            for (int i = 0; i < p->count; i++) {
                int object = p->objects[i];
                objects.alpha_trigger_opacity[object] = get_group_opacities(object);
            }
        }

        buffer->time_run += trigger_dt;
        if (buffer->time_run >= buffer->seconds) {
            if (p) {
                p->alpha = buffer->new_alpha;
                for (int i = 0; i < p->count; i++) {
                    int object = p->objects[i];
                    objects.alpha_trigger_opacity[object] = get_group_opacities(object);
                }
            }
            buffer->active = false;
            
            for (int j = slot; j < alpha_trigger_count - 1; j++) {
                // Shift buffer array
                alpha_trigger_buffer[j] = alpha_trigger_buffer[j + 1];
            }
            
            // Make the game run the pulse moved into the current slot
            slot--;
            alpha_trigger_count--;
        }
    }
}

static int convert_ease(int easing) {
    switch (easing) {
        case 0: return EASE_LINEAR;
        case 1: return EASE_IN_OUT;
        case 2: return EASE_IN;
        case 3: return EASE_OUT;
        case 4: return ELASTIC_IN_OUT;
        case 5: return ELASTIC_IN;
        case 6: return ELASTIC_OUT;
        case 7: return BOUNCE_IN_OUT;
        case 8: return BOUNCE_IN;
        case 9: return BOUNCE_OUT;
        case 10: return EXPO_IN_OUT;
        case 11: return EXPO_IN;
        case 12: return EXPO_OUT;
        case 13: return SINE_IN_OUT;
        case 14: return SINE_IN;
        case 15: return SINE_OUT;
        case 16: return BACK_IN_OUT;
        case 17: return BACK_IN;
        case 18: return BACK_OUT;
    }
    return EASE_LINEAR;
}

void upload_to_move_buffer(int obj) {
    MoveTrigger *trigger = get_move_trigger(obj);
    int target_group = trigger->target_group;
    if (!get_group(target_group)) return;

    for (int i = 0; i < move_trigger_count; i++) {
        if (move_trigger_buffer[i].active &&
            move_trigger_buffer[i].source_obj == obj &&
            move_trigger_buffer[i].restored_from_checkpoint) {
            move_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    MoveTriggerBuffer *buffer = get_new_move_trigger();
    if (!buffer) return;

    buffer->target_group = target_group;
    buffer->source_obj = obj;
    buffer->offset_x = trigger->move_offset_x;
    buffer->offset_y = trigger->move_offset_y;
    buffer->easing = trigger->move_easing;
    buffer->lock_to_player_x = trigger->lock_to_player_x;
    buffer->lock_to_player_y = trigger->lock_to_player_y;
    buffer->seconds = trigger->trig_duration;
    buffer->move_last_x = 0;
    buffer->move_last_y = 0;
    buffer->time_run = 0;
    buffer->active = true;
}

void upload_to_follow_player_y_buffer(int obj) {
    FollowPlayerYTrigger *trigger = get_follow_player_y_trigger(obj);
    GroupNode *group = get_group(trigger->target_group);
    if (!group || group->count <= 0) return;

    for (int i = 0; i < follow_player_y_trigger_count; i++) {
        if (follow_player_y_trigger_buffer[i].active &&
            follow_player_y_trigger_buffer[i].source_obj == obj &&
            follow_player_y_trigger_buffer[i].restored_from_checkpoint) {
            follow_player_y_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    FollowPlayerYTriggerBuffer *buffer = get_new_follow_player_y_trigger();
    if (!buffer) return;

    buffer->target_group = trigger->target_group;
    buffer->source_obj = obj;
    buffer->speed = trigger->follow_speed > 0.f ? trigger->follow_speed : 1.f;
    buffer->delay = trigger->follow_delay;
    buffer->offset = trigger->follow_offset;
    buffer->max_speed = trigger->follow_max_speed;
    buffer->group_y = objects.y[group->objects[0]];
    buffer->active = true;
}

void handle_move_triggers(void) {
    if (trigger_dt <= 0.f) return;
    move_step_dt = trigger_dt;
    
    begin_group_combination_move_step();

    for (int i = 0; i< move_trigger_count; i++) {
        if (!move_trigger_buffer[i].active)
            add_move_to_group_combinations(move_trigger_buffer[i].target_group, 0.f, 0.f);
    }
    
    int count = 0;
    for (int i = 0; i < move_trigger_count; i++) {
        if (move_trigger_buffer[i].active) move_trigger_buffer[count++] = move_trigger_buffer[i];
    }

    move_trigger_count = count;

    for (int slot = 0; slot < move_trigger_count; slot++) {
        MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];
        buffer->time_run += trigger_dt;
        float t = buffer->seconds > 0.f ? easeTime((EaseTypes)convert_ease(buffer->easing),
            buffer->time_run, buffer->seconds, 2.f) : 1.f;
        float delta_x, delta_y;
        if (buffer->lock_to_player_x) delta_x = move_lock_player_x_delta;
        else {
            float current_x = buffer->offset_x * t;
            delta_x = current_x - buffer->move_last_x;
            buffer->move_last_x = current_x;
        }
        if (buffer->lock_to_player_y) delta_y = move_lock_player_y_delta;
        else {
            float current_y = buffer->offset_y * t;
            delta_y = current_y - buffer->move_last_y;
            buffer->move_last_y = current_y;
        }
        add_move_to_group_combinations(buffer->target_group, delta_x, delta_y);
        if (buffer->time_run >= buffer->seconds) buffer->active = false;
    }

    apply_group_combination_move();
}

void upload_to_count_buffer(int obj) {
    CountTrigger *trigger = get_count_trigger(obj);
    for (int i = 0; i < count_trigger_count; i++) {
        if (count_trigger_buffer[i].active &&
            count_trigger_buffer[i].source_obj == obj &&
            count_trigger_buffer[i].restored_from_checkpoint) {
            count_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    CountTriggerBuffer *buffer = get_new_count_trigger();
    if (!buffer) return;

    buffer->target_group = trigger->target_group;
    buffer->source_obj = obj;
    buffer->item_id = trigger->item_id;
    buffer->target_count = trigger->target_count;
    buffer->activate_group = trigger->activate_group;
    buffer->multi_activate = trigger->multi_activate;
    buffer->previous_value = get_item_count(trigger->item_id);
    buffer->active = true;
}

void handle_count_triggers(void) {
    if (count_trigger_count == 0) return;

    int total = count_trigger_count;
    bool compact = false;
    for (int i = 0; i < total; i++) {
        CountTriggerBuffer buffer = count_trigger_buffer[i];
        if (!buffer.active) {
            compact = true;
            continue;
        }
        int value = get_item_count(buffer.item_id);
        bool fire = value == buffer.target_count && value != buffer.previous_value;
        count_trigger_buffer[i].previous_value = value;
        if (fire) {
            if (!buffer.multi_activate) {
                count_trigger_buffer[i].active = false;
                compact = true;
            }
            toggle_group(buffer.target_group, buffer.activate_group, true);
        }
    }

    if (compact) {
        int count = 0;
        for (int i = 0; i < count_trigger_count; i++) {
            if (count_trigger_buffer[i].active) count_trigger_buffer[count++] = count_trigger_buffer[i];
        }
        count_trigger_count = count;
    }
}

void upload_to_touch_buffer(int obj) {
    for (int i = 0; i < touch_trigger_count; i++) {
        if (touch_trigger_buffer[i].source_obj == obj) {
            return;
        }
    }

    TouchTriggerBuffer *buffer = get_new_touch_trigger();
    if (!buffer) return;

    buffer->source_obj = obj;
}

static bool touch_toggle_value(TouchTrigger *trigger) {
    switch (trigger->toggle_mode) {
        case 1: return true;
        case 2: return false;
        default: {
            GroupNode *group = get_group(trigger->target_group);
            return group && group->toggled;
        }
    }
}

void handle_touch_triggers(void) {
    if (state.dead) return;
    if (touch_trigger_count == 0) return;

    bool p1_pressed = state.input.pressedJump || (!state.old_input.holdJump && state.input.holdJump);
    bool p1_released = state.old_input.holdJump && !state.input.holdJump;
    bool p2_pressed = false;
    bool p2_released = false;

    if (state.dual) {
        p2_pressed = state.input_p2.pressedJump || (!state.old_input_p2.holdJump && state.input_p2.holdJump);
        p2_released = state.old_input_p2.holdJump && !state.input_p2.holdJump;
    }

    int total = touch_trigger_count;
    for (int i = 0; i < total; i++) {
        TouchTriggerBuffer buffer = touch_trigger_buffer[i];

        TouchTrigger *trigger = get_touch_trigger(buffer.source_obj);
        if (!trigger) continue;

        unsigned char held = buffer.held_mask;
        unsigned char next_held = held;

        if (state.dual) {
            if (p1_pressed && !(held & 1)) next_held |= 1;
            if (p1_released && (held & 1)) next_held &= ~1;
            if (p2_pressed && !(held & 2)) next_held |= 2;
            if (p2_released && (held & 2)) next_held &= ~2;
        } else {
            if (p1_pressed && !(held & 1)) next_held |= 1;
            if (p1_released && (held & 1)) next_held &= ~1;
        }

        if (trigger->hold_mode && held == 0) {
            if (state.input.holdJump) next_held |= 1;
            if (state.dual && state.input_p2.holdJump) next_held |= 2;
        }

        bool pressed = held == 0 && next_held != 0;
        bool released = held != 0 && next_held == 0;
        touch_trigger_buffer[i].held_mask = next_held;

        if (pressed) {
            bool applied = touch_toggle_value(trigger);
            touch_trigger_buffer[i].applied = applied;
            toggle_group(trigger->target_group, applied, true);
        } else if (released && trigger->hold_mode) {
            toggle_group(trigger->target_group, !buffer.applied, true);
        }
    }
}

void handle_follow_player_y_triggers(void) {
    if (trigger_dt <= 0.f) return;
    if (follow_player_y_trigger_count == 0) return;

    begin_group_combination_move_step();

    for (int slot = 0; slot < follow_player_y_trigger_count; slot++) {
        FollowPlayerYTriggerBuffer *buffer = &follow_player_y_trigger_buffer[slot];
        if (!buffer->active) continue;
        float target = player_follow_y_old(&state.player, buffer->delay) + buffer->offset;
        float delta = (target - buffer->group_y) * buffer->speed;
        if (buffer->max_speed > 0.f) {
            float max_delta = buffer->max_speed * trigger_dt;
            if (delta > max_delta) delta = max_delta;
            else if (delta < -max_delta) delta = -max_delta;
        }
        buffer->group_y += delta;
        add_move_to_group_combinations(buffer->target_group, 0.f, delta);
    }

    apply_group_combination_move();

    int count = 0;
    for (int slot = 0; slot < follow_player_y_trigger_count; slot++) {
        if (!follow_player_y_trigger_buffer[slot].active) continue;
        if (count != slot) follow_player_y_trigger_buffer[count] = follow_player_y_trigger_buffer[slot];
        count++;
    }

    follow_player_y_trigger_count = count;
}

void upload_to_follow_buffer(int obj) {
    FollowTrigger *trigger = get_follow_trigger(obj);
    if (trigger->trig_duration <= 0.f) return;
    if (trigger->target_group == trigger->follow_group) return;

    GroupNode *target = get_group(trigger->target_group);
    GroupNode *follow = get_group(trigger->follow_group);
    if (!target || target->count <= 0) return;
    if (!follow || follow->count <= 0) return;

    for (int i = 0; i < follow_trigger_count; i++) {
        if (follow_trigger_buffer[i].active &&
            follow_trigger_buffer[i].source_obj == obj &&
            follow_trigger_buffer[i].restored_from_checkpoint) {
            follow_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    FollowTriggerBuffer *buffer = get_new_follow_trigger();
    if (!buffer) return;

    int ref_obj = follow->objects[0];

    buffer->target_group = trigger->target_group;
    buffer->follow_group = trigger->follow_group;
    buffer->source_obj = obj;
    buffer->ref_obj = ref_obj;
    buffer->x_mod = trigger->x_mod;
    buffer->y_mod = trigger->y_mod;
    buffer->seconds = trigger->trig_duration;
    buffer->time_run = 0.f;
    buffer->last_x = objects.x[ref_obj];
    buffer->last_y = objects.y[ref_obj];
    buffer->active = true;
}

static void compact_follow_triggers(void) {
    int count = 0;
    for (int i = 0; i < follow_trigger_count; i++) {
        if (!follow_trigger_buffer[i].active) continue;
        if (count != i) {
            follow_trigger_buffer[count] = follow_trigger_buffer[i];
        }
        count++;
    }
    follow_trigger_count = count;
}

void handle_follow_triggers(void) {
    if (trigger_dt <= 0.f) return;
    if (follow_trigger_count == 0) return;

    begin_group_combination_move_step();

    for (int slot = 0; slot < follow_trigger_count; slot++) {
        FollowTriggerBuffer *buffer = &follow_trigger_buffer[slot];
        if (!buffer->active) continue;

        GroupNode *follow = get_group(buffer->follow_group);
        if (!follow || follow->count <= 0) {
            buffer->active = false;
            continue;
        }

        int ref_obj = buffer->ref_obj;
        if (ref_obj < 0 || ref_obj >= (int)objects.count) {
            buffer->active = false;
            continue;
        }

        float delta_x = (objects.x[ref_obj] - buffer->last_x) * buffer->x_mod;
        float delta_y = (objects.y[ref_obj] - buffer->last_y) * buffer->y_mod;
        buffer->last_x = objects.x[ref_obj];
        buffer->last_y = objects.y[ref_obj];

        if (delta_x != 0.f || delta_y != 0.f) {
            add_move_to_group_combinations(buffer->target_group, delta_x, delta_y);
        }

        buffer->time_run += trigger_dt;
        if (buffer->time_run >= buffer->seconds) {
            buffer->active = false;
        }
    }

    apply_group_combination_move();
    compact_follow_triggers();
}

void upload_to_rotate_buffer(int obj) {
    RotateTrigger *trigger = get_rotate_trigger(obj);
    GroupNode *group = get_group(trigger->target_group);
    if (!group || group->count <= 0) return;

    for (int i = 0; i < rotate_trigger_count; i++) {
        if (rotate_trigger_buffer[i].active &&
            rotate_trigger_buffer[i].source_obj == obj &&
            rotate_trigger_buffer[i].restored_from_checkpoint) {
            rotate_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
    }

    RotateTriggerBuffer *buffer = get_new_rotate_trigger();
    if (!buffer) return;

    buffer->center_obj = -1;
    buffer->use_center = trigger->center_group != 0;
    buffer->source_obj = obj;
    buffer->total_degrees = trigger->degrees + (float)trigger->times360 * 360.f;
    buffer->lock_object_rotation = trigger->lock_object_rotation;
    buffer->easing_type = convert_ease(trigger->rotate_easing);
    buffer->easing_rate = trigger->easing_rate > 0.f ? trigger->easing_rate : 2.f;
    buffer->seconds = trigger->trig_duration;
    buffer->time_run = 0.f;
    buffer->target_group = trigger->target_group;

    buffer->target_count = group->count;
    buffer->targets = (RotateTargetState *)malloc(sizeof(RotateTargetState) * group->count);
    if (!buffer->targets) {
        buffer->target_count = 0;
        buffer->active = false;
        return;
    }
    for (int i = 0; i < group->count; i++) {
        int target = group->objects[i];
        remember_rotate_initial_rotation(target);
        buffer->targets[i].obj = target;
    }

    if (buffer->use_center) {
        GroupNode *center = get_group(trigger->center_group);
        if (center && center->count > 0 && trigger->center_group != trigger->target_group) {
            buffer->center_obj = center->objects[0];
        } else {
            buffer->use_center = false;
        }
    }
    buffer->active = true;
}

static void compact_rotate_triggers(void) {
    int count = 0;
    for (int i = 0; i < rotate_trigger_count; i++) {
        RotateTriggerBuffer *buffer = &rotate_trigger_buffer[i];
        if (!buffer->active) {
            free(buffer->targets);
            buffer->targets = NULL;
            continue;
        }
        if (count != i) {
            rotate_trigger_buffer[count] = *buffer;
            buffer->targets = NULL;
        }
        count++;
    }
    rotate_trigger_count = count;
}

static bool rotate_target_is_solid(int obj) {
    int id = objects.id[obj];
    if (id <= 0 || id >= GAME_OBJECT_COUNT) return false;
    const ObjectHitbox *hitbox = game_objects[id].hitbox;
    return hitbox && hitbox->collision_type == HITBOX_SOLID;
}

void handle_rotate_triggers(void) {
    if (trigger_dt <= 0.f) return;

    begin_group_combination_rotation_step();

    for (int slot = 0; slot < rotate_trigger_count; slot++) {
        RotateTriggerBuffer *buffer = &rotate_trigger_buffer[slot];
        if (!buffer->active) continue;
        buffer->time_run += trigger_dt;
        float prev_time = buffer->time_run - trigger_dt;
        if (prev_time < 0.f) prev_time = 0.f;
        float t = buffer->seconds > 0.f ? easeTime((EaseTypes)buffer->easing_type,
            buffer->time_run, buffer->seconds, buffer->easing_rate) : 1.f;
        float prev_t = buffer->seconds > 0.f ? easeTime((EaseTypes)buffer->easing_type,
            prev_time, buffer->seconds, buffer->easing_rate) : 0.f;
        float angle = buffer->total_degrees * t;
        float prev_angle = buffer->total_degrees * prev_t;
        float angle_delta = angle - prev_angle;
        Matrix2x2 rotation_matrix = matrix2x2_identity();
        float pivot_x = 0.f;
        float pivot_y = 0.f;
        if (buffer->use_center) {
            get_group_object_position(buffer->center_obj, &pivot_x, &pivot_y);
            float rad = -angle_delta * (float)(M_PI / 180.0);
            rotation_matrix = matrix2x2_rotation(rad);
        }

        bool is_last_for_group = true;
        for (int later = slot + 1; later < rotate_trigger_count; later++) {
            if (rotate_trigger_buffer[later].active && rotate_trigger_buffer[later].target_group == buffer->target_group) {
                is_last_for_group = false;
                break;
            }
        }

        if (is_last_for_group && add_rotation_to_group_combinations(buffer->target_group, &rotation_matrix, pivot_x, pivot_y)) {
            for (int i = 0; i < buffer->target_count; i++) {
                RotateTargetState *target = &buffer->targets[i];
                int obj = target->obj;
                if (!buffer->lock_object_rotation && !rotate_target_is_solid(obj)) {
                    objects.rotation[obj] += angle_delta;
                    objects.visual_rotation[obj] += angle_delta;
                }
            }
        }
        if (buffer->time_run >= buffer->seconds) buffer->active = false;
    }

    apply_group_combination_rotation();
    compact_rotate_triggers();
}

void upload_to_spawn_buffer(int obj, bool from_spawn) {
    for (int i = 0; i < spawn_trigger_count; i++) {
        SpawnTriggerBuffer *existing = &spawn_trigger_buffer[i];
        if (!existing->active || existing->source_obj != obj) continue;

        if (existing->restored_from_checkpoint) {
            existing->restored_from_checkpoint = false;
        }
        return;
    }

    SpawnTriggerBuffer *buffer = get_new_spawn_trigger();
    if (!buffer) return;
    
    SpawnTrigger *trigger = get_spawn_trigger(obj);
    buffer->queued = from_spawn;
    buffer->target_group = trigger->target_group;
    buffer->source_obj = obj;
    buffer->seconds = trigger->spawn_delay ? trigger->spawn_delay : trigger->trig_duration;
    buffer->time_run = 0;
    buffer->active = true;
}

void handle_spawn_triggers(void) {
    int count = 0;
    for (int slot = 0; slot < spawn_trigger_count; slot++) {
        SpawnTriggerBuffer *buffer = &spawn_trigger_buffer[slot];
        if (!buffer->active) continue;
        buffer->queued = false;
        if (count != slot) spawn_trigger_buffer[count] = *buffer;
        count++;
    }

    spawn_trigger_count = count;

    for (int slot = 0; slot < spawn_trigger_count; slot++) {
        SpawnTriggerBuffer *buffer = &spawn_trigger_buffer[slot];
        if (!buffer->active || buffer->queued) continue;

        buffer->time_run += trigger_dt;
        if (buffer->time_run > buffer->seconds) {
            buffer->active = false;

            spawn_group(buffer->target_group);

            for (int j = slot; j < spawn_trigger_count - 1; j++) {
                // Shift buffer array
                spawn_trigger_buffer[j] = spawn_trigger_buffer[j + 1];
            }
            
            // Make the game run the pulse moved into the current slot
            slot--;
            spawn_trigger_count--;
        }
    }
}

static void pulse_register(PulseTriggerBuffer *buffer, PulseTrigger *trigger) {
    int channel = trigger->target_group;

    buffer->target_group = trigger->target_group;

    buffer->color.r = trigger->trig_colorR;
    buffer->color.g = trigger->trig_colorG;
    buffer->color.b = trigger->trig_colorB;

    buffer->copied_color_id = trigger->copied_color_id;
    buffer->copied_hsv = trigger->copied_hsv;

    buffer->main_only = trigger->pulse_main_only;
    buffer->detail_only = trigger->pulse_detail_only;

    buffer->fade_in  = trigger->pulse_fade_in;
    buffer->hold     = trigger->pulse_hold;
    buffer->fade_out = trigger->pulse_fade_out;

    buffer->pulse_mode = trigger->pulse_mode;
    buffer->pulse_target_type = trigger->pulse_target_type;

    buffer->target_color_id = channel;

    if (buffer->pulse_target_type == PULSE_TARGET_CHANNEL) {
        ColorChannel *chan = &channels[get_col_channel_index(buffer->target_color_id)];
        buffer->pulse_index = chan->num_pulses;
        chan->num_pulses++;
    } else if (buffer->pulse_target_type == PULSE_TARGET_GROUP) {
        bool both = !buffer->main_only && !buffer->detail_only;
        GroupNode *p = get_group(buffer->target_group);
        if (!p) return;

        for (int i = 0; i < p->count; i++) {
            int obj_idx = p->objects[i];
            if (both || buffer->main_only) {
                objects.num_main_pulses[obj_idx]++;
                objects.main_being_pulsed[obj_idx] = true;
            }

            if (both || buffer->detail_only) {
                objects.num_detail_pulses[obj_idx]++;
                objects.detail_being_pulsed[obj_idx] = true;
            }
        }
    }

    buffer->seconds = trigger->pulse_fade_in + trigger->pulse_hold + trigger->pulse_fade_out;
}


void upload_to_pulse_buffer(int obj) {
    PulseTrigger *trigger = get_pulse_trigger(obj);
    int channel = trigger->target_group;
    if (channel == 0) return;

    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *existing = &pulse_trigger_buffer[i];
        if (!existing->active || existing->source_obj != obj) continue;

        if (existing->restored_from_checkpoint) {
            existing->restored_from_checkpoint = false;
            return;
        }

        existing->time_run = 0;
        return;
    }

    PulseTriggerBuffer *buffer = get_new_pulse_trigger();
    if (!buffer) return;

    pulse_register(buffer, trigger);

    buffer->source_obj = obj;
    buffer->time_run = 0;
    buffer->active = true;
}

void restore_pulse_trigger(int obj, float time_run) {
    PulseTrigger *trigger = get_pulse_trigger(obj);
    if (trigger->target_group == 0) return;

    PulseTriggerBuffer *buffer = get_new_pulse_trigger();
    if (!buffer) return;

    pulse_register(buffer, trigger);

    buffer->source_obj = obj;
    buffer->restored_from_checkpoint = true;
    buffer->time_run = time_run;
    buffer->active = true;
}

void handle_pulse_triggers(void) {
    // preprocess HSV for all active pulses
    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[i];
        if (!buffer->active) continue;
        if (buffer->pulse_mode == PULSE_MODE_HSV) {
            int source_id = buffer->copied_color_id;
            if (source_id <= 0 || source_id >= COL_CHANNEL_NUM)
                source_id = buffer->target_color_id;
            int source_idx = get_col_channel_index(source_id);
            Color copy_color = channels[source_idx].non_pulse_color;
            buffer->color = HSV_combine(copy_color, buffer->copied_hsv);
        }
    }

    // process channel pulses in pulse_index order
    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[i];
        if (!buffer->active || buffer->pulse_target_type != PULSE_TARGET_CHANNEL) continue;

        ColorChannel *chan = &channels[get_col_channel_index(buffer->target_color_id)];
        int idx = buffer->pulse_index;
        if (idx < 0 || idx >= chan->num_pulses) continue;

        Color base_color = (idx == 0) ? chan->non_pulse_color : chan->color;

        Color result_color;
        if (buffer->time_run <= buffer->fade_in) {
            float fade_time = 1.f;
            if (buffer->fade_in > 0) fade_time = buffer->time_run / buffer->fade_in;
            float r = (buffer->color.r - (buffer->color.r - base_color.r) * (1.f - fade_time));
            float g = (buffer->color.g - (buffer->color.g - base_color.g) * (1.f - fade_time));
            float b = (buffer->color.b - (buffer->color.b - base_color.b) * (1.f - fade_time));
            result_color.r = (unsigned char)r;
            result_color.g = (unsigned char)g;
            result_color.b = (unsigned char)b;
        } else if (buffer->time_run >= buffer->fade_in + buffer->hold) {
            float fade_time = 1.f;
            if (buffer->fade_out > 0) fade_time = (buffer->time_run - buffer->hold - buffer->fade_in) / buffer->fade_out;
            float r = (buffer->color.r - (buffer->color.r - base_color.r) * fade_time);
            float g = (buffer->color.g - (buffer->color.g - base_color.g) * fade_time);
            float b = (buffer->color.b - (buffer->color.b - base_color.b) * fade_time);
            result_color.r = (unsigned char)r;
            result_color.g = (unsigned char)g;
            result_color.b = (unsigned char)b;
        } else {
            result_color = buffer->color;
        }

        chan->color = result_color;
    }

    for (int gi = 0; gi < pulse_trigger_count; gi++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[gi];
        if (!buffer->active || buffer->pulse_target_type != PULSE_TARGET_GROUP) continue;
        bool both = !buffer->main_only && !buffer->detail_only;
        GroupNode *p = get_group(buffer->target_group);
        if (!p) continue;
        for (int i = 0; i < p->count; i++) {
            int obj_idx = p->objects[i];
            if (obj_idx < 0 || obj_idx >= objects.count) continue;
            if (both || buffer->main_only) {
                objects.main_color[obj_idx] = objects.main_non_pulse_color[obj_idx];
                objects.main_being_pulsed[obj_idx] = true;
            }
            if (both || buffer->detail_only) {
                objects.detail_color[obj_idx] = objects.detail_non_pulse_color[obj_idx];
                objects.detail_being_pulsed[obj_idx] = true;
            }
        }
    }

    for (int gi = 0; gi < pulse_trigger_count; gi++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[gi];
        if (!buffer->active || buffer->pulse_target_type != PULSE_TARGET_GROUP) continue;
        bool both = !buffer->main_only && !buffer->detail_only;

        float alpha;
        if (buffer->time_run <= buffer->fade_in) {
            alpha = (buffer->fade_in > 0.f) ? (buffer->time_run / buffer->fade_in) : 1.f;
        } else if (buffer->time_run >= buffer->fade_in + buffer->hold) {
            float fade_time = 1.f;
            if (buffer->fade_out > 0.f) fade_time = (buffer->time_run - buffer->hold - buffer->fade_in) / buffer->fade_out;
            alpha = 1.f - fade_time;
        } else {
            alpha = 1.f;
        }
        if (alpha < 0.f) alpha = 0.f;
        else if (alpha > 1.f) alpha = 1.f;

        GroupNode *p = get_group(buffer->target_group);
        if (!p) continue;
        for (int i = 0; i < p->count; i++) {
            int obj_idx = p->objects[i];
            if (obj_idx < 0 || obj_idx >= objects.count) continue;
            if (both || buffer->main_only) {
                Color target = buffer->color;
                if (buffer->pulse_mode == PULSE_MODE_HSV && buffer->copied_color_id == 0) {
                    target = HSV_combine(objects.main_color[obj_idx], buffer->copied_hsv);
                }

                if (objects.main_col_HSV_enabled[obj_idx]) target = HSV_combine(target, objects.main_col_HSV[obj_idx]);
                Color base = objects.main_color[obj_idx];
                objects.main_color[obj_idx].r = (unsigned char)(base.r + (target.r - base.r) * alpha);
                objects.main_color[obj_idx].g = (unsigned char)(base.g + (target.g - base.g) * alpha);
                objects.main_color[obj_idx].b = (unsigned char)(base.b + (target.b - base.b) * alpha);
                objects.main_being_pulsed[obj_idx] = true;
            }
            if (both || buffer->detail_only) {
                Color target = buffer->color;
                if (buffer->pulse_mode == PULSE_MODE_HSV && buffer->copied_color_id == 0) {
                    target = HSV_combine(objects.detail_color[obj_idx], buffer->copied_hsv);
                }
                if (objects.detail_col_HSV_enabled[obj_idx]) target = HSV_combine(target, objects.detail_col_HSV[obj_idx]);
                Color base = objects.detail_color[obj_idx];
                objects.detail_color[obj_idx].r = (unsigned char)(base.r + (target.r - base.r) * alpha);
                objects.detail_color[obj_idx].g = (unsigned char)(base.g + (target.g - base.g) * alpha);
                objects.detail_color[obj_idx].b = (unsigned char)(base.b + (target.b - base.b) * alpha);
                objects.detail_being_pulsed[obj_idx] = true;
            }
        }
    }

    // time update and cleanup for all pulses
    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[i];
        if (!buffer->active) continue;

        buffer->time_run += trigger_dt;
        if (buffer->time_run > buffer->seconds) {
            if (buffer->pulse_target_type == PULSE_TARGET_GROUP) {
                bool both = !buffer->main_only && !buffer->detail_only;
                GroupNode *p = get_group(buffer->target_group);
                int count = p ? p->count : 0;
                for (int i = 0; i < count; i++) {
                    int obj_idx = p->objects[i];
                    if (obj_idx < 0 || obj_idx >= objects.count) continue;
                    if (both || buffer->main_only) {
                        objects.num_main_pulses[obj_idx]--;
                        if (!objects.num_main_pulses[obj_idx]) {
                            objects.main_being_pulsed[obj_idx] = false;
                            objects.main_color[obj_idx] = objects.main_non_pulse_color[obj_idx];
                        }
                    }
                    if (both || buffer->detail_only) {
                        objects.num_detail_pulses[obj_idx]--;
                        if (!objects.num_detail_pulses[obj_idx]) {
                            objects.detail_being_pulsed[obj_idx] = false;
                            objects.detail_color[obj_idx] = objects.detail_non_pulse_color[obj_idx];
                        }
                    }
                }
            } else if (buffer->pulse_target_type == PULSE_TARGET_CHANNEL) {
                int chan_idx = get_col_channel_index(buffer->target_color_id);
                ColorChannel *chan = &channels[chan_idx];
                int removed_idx = buffer->pulse_index;

                chan->num_pulses--;
                if (!chan->num_pulses) chan->color = chan->non_pulse_color;

                for (int j = 0; j < pulse_trigger_count; j++) {
                    if (j == i) continue;
                    PulseTriggerBuffer *other = &pulse_trigger_buffer[j];
                    if (!other->active) continue;
                    if (other->pulse_target_type != PULSE_TARGET_CHANNEL) continue;
                    if (other->target_color_id != buffer->target_color_id) continue;
                    if (other->pulse_index > removed_idx) other->pulse_index--;
                }
            }

            for (int j = i; j < pulse_trigger_count - 1; j++) {
                // Shift buffer array
                pulse_trigger_buffer[j] = pulse_trigger_buffer[j + 1];
            }

            pulse_trigger_count--;
            // Make the game run the pulse moved into the current slot
            i--;
        }
    }
}

void upload_to_buffer(int obj, int channel) {
    if (channel == 0) channel = 1;
    int buffer_channel = get_col_channel_index(channel);

    ColTriggerBuffer *buffer = &col_trigger_buffer[buffer_channel];
    ColorTrigger *trigger =  get_color_trigger(obj);

    buffer->old_color = channels[buffer_channel].color;
    buffer->old_alpha = channels[buffer_channel].alpha;
    if (trigger && trigger->p1_color) {
        buffer->new_color = get_p2_if_black(p1_color);
        buffer->new_alpha = 1.0f;
    } else if (trigger && trigger->p2_color) {
        buffer->new_color = get_p1_if_black(p2_color);
        buffer->new_alpha = 1.0f;
    } else {
        buffer->new_color.r = trigger->trig_colorR;
        buffer->new_color.g = trigger->trig_colorG;
        buffer->new_color.b = trigger->trig_colorB;
        buffer->new_alpha = trigger->opacity;
    }

    int copy_id = trigger->copied_color_id;
    if (copy_id > 0) {
        buffer->copied_color_id = copy_id;
        buffer->copied_hsv = trigger->copied_hsv;
        channels[buffer_channel].copy_color_id = copy_id;
        channels[buffer_channel].hsv = trigger->copied_hsv;
    } else {
        buffer->copied_color_id = 0;
        channels[buffer_channel].copy_color_id = 0;
    }

    if (channel < CHANNEL_BG) {
        channels[buffer_channel].blending = trigger->blending;
    }
    
    
    float duration = trigger->trig_duration;
    if (duration == 0) {
        Color color_to_lerp = buffer->new_color;

        if (buffer->copied_color_id > 0) {
            int src = get_col_channel_index(buffer->copied_color_id);
            color_to_lerp = channels[src].color;
            buffer->new_alpha = channels[src].alpha;
        }

        channels[buffer_channel].color = color_to_lerp;
        channels[buffer_channel].non_pulse_color = color_to_lerp;
        channels[buffer_channel].alpha = buffer->new_alpha;
        return;
    }
    
    buffer->seconds = duration;
    buffer->time_run = 0;
    buffer->source_obj = obj;
    buffer->active = true;
}

void upload_color_to_buffer(int channel, u32 color, float seconds) {
    int buffer_channel = get_col_channel_index(channel);

    ColTriggerBuffer *buffer = &col_trigger_buffer[buffer_channel];
    buffer->old_color = channels[buffer_channel].color;
    buffer->old_alpha = channels[buffer_channel].alpha;
    buffer->new_color.r = GET_R(color);
    buffer->new_color.g = GET_G(color);
    buffer->new_color.b = GET_B(color);
    buffer->new_alpha = 1.0f;
    buffer->seconds = seconds;
    buffer->time_run = 0;
    buffer->source_obj = 0;
    buffer->active = true;
}

void toggle_group(int group, bool activate, bool spawn) {
    GroupNode *p = get_group(group);
    if (p) {
        p->toggled = !activate;
        for (int i = 0; i < p->count; i++) {
            int object = p->objects[i];
            objects.flags[object] &= ~FLAG_TOGGLED;
            objects.flags[object] |= get_group_toggles(object) ? FLAG_TOGGLED : 0;
        }

        if (activate && spawn) {
            if (toggle_spawn_count < 8) toggle_spawn_buffer[toggle_spawn_count++] = group;
            if (toggle_spawn_active) return;

            toggle_spawn_active = true;
            int guard = 0;
            while (toggle_spawn_count > 0 && guard++ < 64) {
                int next = toggle_spawn_buffer[0];
                for (int i = 1; i < toggle_spawn_count; i++) toggle_spawn_buffer[i - 1] = toggle_spawn_buffer[i];
                toggle_spawn_count--;
                spawn_group(next);
            }
            toggle_spawn_active = false;
        }
    }
}

void spawn_group(int group) {
    GroupNode *p = get_group(group);
    if (!p) return;

    for (int i = 0; i < p->count; i++) {
        int obj_idx = p->objects[i];
        if (trigger_is_spawn_triggered(objects.id[obj_idx], obj_idx)
            && is_trigger_object(objects.id[obj_idx])
            && (trigger_is_multi_triggered(objects.id[obj_idx], obj_idx) || !GET_ACTIVATED(obj_idx))
            && !(objects.flags[obj_idx] & FLAG_TOGGLED)) {
            run_trigger(obj_idx, true);
        }
    }
}

static bool object_in_group(int obj, int group) {
    if (obj < 0 || obj >= (int)objects.count) return false;
    for (int i = 0; i < objects.group_count[obj]; i++) {
        if (objects.groups[obj][i] == group) return true;
    }
    return false;
}

static void register_on_death_trigger(int obj) {
    OnDeathTrigger *trigger = get_on_death_trigger(obj);

    for (int i = 0; i < on_death_count; i++) {
        if (on_death_entries[i].source_obj == obj) return;
    }

    if (on_death_count >= ON_DEATH_MAX) return;

    OnDeathEntry *entry = &on_death_entries[on_death_count++];
    entry->source_obj = obj;
    entry->target_group = trigger->target_group;
    entry->activate = trigger->activate;
    entry->multi = trigger->multi_triggered;
}

void run_on_death_triggers() {
    int count = on_death_count;
    int kept = 0;

    for (int i = 0; i < count; i++) {
        OnDeathEntry *entry = &on_death_entries[i];

        toggle_group(entry->target_group, entry->activate, true);

        if (!entry->multi) continue;
        if (kept != i) on_death_entries[kept] = *entry;
        kept++;
    }

    on_death_count = kept;
}

void clear_on_death_triggers() {
    on_death_count = 0;
}

static void stop_actions(int group) {
    for (int i = 0; i < move_trigger_count; i++) {
        MoveTriggerBuffer *buffer = &move_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < rotate_trigger_count; i++) {
        RotateTriggerBuffer *buffer = &rotate_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < alpha_trigger_count; i++) {
        AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->time_run = buffer->seconds + 0.001f;
    }

    for (int i = 0; i < follow_trigger_count; i++) {
        FollowTriggerBuffer *buffer = &follow_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < follow_player_y_trigger_count; i++) {
        FollowPlayerYTriggerBuffer *buffer = &follow_player_y_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < COL_CHANNEL_NUM; i++) {
        ColTriggerBuffer *buffer = &col_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    for (int i = 0; i < spawn_trigger_count; i++) {
        SpawnTriggerBuffer *buffer = &spawn_trigger_buffer[i];
        if (!buffer->active || !object_in_group(buffer->source_obj, group)) continue;
        buffer->active = false;
    }

    int kept = 0;
    for (int i = 0; i < on_death_count; i++) {
        if (object_in_group(on_death_entries[i].source_obj, group)) continue;
        if (kept != i) on_death_entries[kept] = on_death_entries[i];
        kept++;
    }

    on_death_count = kept;
}

void run_trigger(int obj, bool from_spawn) {
    if (objects.flags[obj] & FLAG_TOGGLED) return;

    switch (objects.id[obj]) {
        case TRIGGER_FADE_SIMPLE:
            current_fading_effect = FADE_SIMPLE;
            break;
            
        case TRIGGER_FADE_UP:
            current_fading_effect = FADE_UP;
            break;
            
        case TRIGGER_FADE_DOWN:
            current_fading_effect = FADE_DOWN;
            break;
            
        case TRIGGER_FADE_RIGHT:
            current_fading_effect = FADE_RIGHT;
            break;
            
        case TRIGGER_FADE_LEFT:
            current_fading_effect = FADE_LEFT;
            break;
            
        case TRIGGER_FADE_SCALE_IN:
            current_fading_effect = FADE_SCALE_IN;
            break;
            
        case TRIGGER_FADE_SCALE_OUT:
            current_fading_effect = FADE_SCALE_OUT;
            break;
        
        case TRIGGER_FADE_INWARDS:
            current_fading_effect = FADE_INWARDS;
            break;

        case TRIGGER_FADE_OUTWARDS:
            current_fading_effect = FADE_OUTWARDS;
            break;
        
        case TRIGGER_FADE_LEFT_SEMICIRCLE:
            current_fading_effect = FADE_CIRCLE_LEFT;
            break;

        case TRIGGER_FADE_RIGHT_SEMICIRCLE:
            current_fading_effect = FADE_CIRCLE_RIGHT;
            break;

        case BG_TRIGGER:
            upload_to_buffer(obj, CHANNEL_BG);
            if (!get_color_trigger(obj)->tintGround) break;
        
        case GROUND_TRIGGER:
            upload_to_buffer(obj, CHANNEL_GROUND);
            break;
                    
        case LINE_TRIGGER:
        case V2_0_LINE_TRIGGER: // gd converts 1.4 line trigger to 2.0 one for some reason
            upload_to_buffer(obj, CHANNEL_LINE);
            break;
        
        case OBJ_TRIGGER:
            upload_to_buffer(obj, CHANNEL_OBJ);
            upload_to_buffer(obj, CHANNEL_OBJ_BLENDING);
            break;
        
        case OBJ_2_TRIGGER:
            upload_to_buffer(obj, 1);
            break;
        
        case COL2_TRIGGER: // col 2
            upload_to_buffer(obj, 2);
            break;

        case COL3_TRIGGER: // col 3
            upload_to_buffer(obj, 3);
            break;
            
        case COL4_TRIGGER: // col 4
            upload_to_buffer(obj, 4);
            break;
            
        case THREEDL_TRIGGER: // 3DL
            upload_to_buffer(obj, CHANNEL_3DL);
            break;

        case GROUND_2_TRIGGER:
            upload_to_buffer(obj, CHANNEL_GROUND_2);
            break;

        case ENABLE_TRAIL:
            p1_trail = true;
            break;
        
        case DISABLE_TRAIL:
            p1_trail = false;
            break;

        case COL_TRIGGER: // 2.0 color trigger
            if (get_col_channel_index(get_color_trigger(obj)->target_color_id) == get_col_channel_index(CHANNEL_BLACK)) break;
            upload_to_buffer(obj, get_color_trigger(obj)->target_color_id);
            break;
        case ALPHA_TRIGGER:
            upload_to_alpha_buffer(obj);
            break;
        case MOVE_TRIGGER:
            upload_to_move_buffer(obj);
            break;
        case COUNT_TRIGGER:
            upload_to_count_buffer(obj);
            break;
        case INSTANT_COUNT_TRIGGER:
        {
            InstantCountTrigger *trigger = get_instant_count_trigger(obj);
            int value = get_item_count(trigger->item_id);
            bool pass = false;
            switch (trigger->instant_count_mode) {
                case INSTANT_COUNT_MODE_EQUALS: pass = value == trigger->target_count; break;
                case INSTANT_COUNT_MODE_LARGER: pass = value > trigger->target_count; break;
                case INSTANT_COUNT_MODE_SMALLER: pass = value < trigger->target_count; break;
            }
            if (pass) toggle_group(trigger->target_group, trigger->activate_group, true);
            break;
        }
        case FOLLOW_PLAYER_Y_TRIGGER:
            upload_to_follow_player_y_buffer(obj);
            break;
        case FOLLOW_TRIGGER:
            upload_to_follow_buffer(obj);
            break;
        case ROTATE_TRIGGER:
            upload_to_rotate_buffer(obj);
            break;
        case SPAWN_TRIGGER:
            upload_to_spawn_buffer(obj, from_spawn);
            break;
        case PULSE_TRIGGER:
            if (get_pulse_trigger(obj)->pulse_target_type == PULSE_TARGET_CHANNEL &&
                get_col_channel_index(get_pulse_trigger(obj)->target_group) == get_col_channel_index(CHANNEL_BLACK)) break;
            upload_to_pulse_buffer(obj);
            break;
        case TOGGLE_TRIGGER:
        {
            ToggleTrigger *trigger = get_toggle_trigger(obj);
            toggle_group(trigger->target_group, trigger->activate_group, false);
            break;
        }
        case TOUCH_TRIGGER:
            upload_to_touch_buffer(obj);
            break;
        case STOP_TRIGGER:
        {
            StopTrigger *trigger = get_stop_trigger(obj);
            stop_actions(trigger->target_group);
            break;
        }
        case ON_DEATH_TRIGGER:
            register_on_death_trigger(obj);
            break;
        case HIDE_PLAYER_TRIGGER:
            MotionTrail_StopStroke(&trail_p1);
            MotionTrail_StopStroke(&trail_p2);
            MotionTrail_Clear(&wave_trail_p1);
            MotionTrail_Clear(&wave_trail_p2);
            MotionTrail_StopStroke(&wave_trail_p1);
            MotionTrail_StopStroke(&wave_trail_p2);
            state.show_player = false;
            break;
        case SHOW_PLAYER_TRIGGER:
            MotionTrail_ResumeStroke(&trail_p1);
            MotionTrail_ResumeStroke(&trail_p2);
            MotionTrail_ResumeStroke(&wave_trail_p1);
            MotionTrail_ResumeStroke(&wave_trail_p2);
            state.show_player = true;
            break;
        default:
            return;
    }
    SET_ACTIVATED(obj, true);
}

int compare_triggers(const void *a, const void *b) {
    int ta = *((int*) a);
    int tb = *((int*) b);
    
    float xa = objects.x[ta];
    float xb = objects.x[tb];

    if (xa != xb) {
        return xa - xb;
    }
    
    float ya = objects.y[ta];
    float yb = objects.y[tb];

    return yb - ya;
}

#define TOUCH_TRIGGER_EPSILON 0.5f

int triggers_buffer[TRIGGER_BUFFER_SIZE];
int trigger_count;

bool object_can_be_x_moved(int obj) {
    int obj_id = objects.id[obj];
    if (is_trigger_object(obj_id) && !objects.touch_triggered[obj]) {
        return false;
    }

    // Speed portals
    if (obj_id >= 200 && obj_id <= 203) return false;

    return true;
}

void handle_triggers(void) {
    trigger_count = 0;
    int cam_sx = (int)((state.player.x) / SECTION_SIZE);
    
    for (int sx = -1; sx < 1; sx++) {
        for (int sy = -(400 / SECTION_SIZE); sy <= MAX_LEVEL_HEIGHT / SECTION_SIZE; sy++) {
            int sec_x = cam_sx + sx;
            if (sec_x < 0) continue;

            Section *sec = get_section(sec_x, sy);
            const SectionTriggers *list = sec->triggers;
            if (!list) continue;
            bool cached = list->count >= 0;
            int count = cached ? list->count : sec->object_count;
            for (int i = 0; i < count; i++) {
                int obj = sec->objects[cached ? list->indices[i] : i];
                if (!cached && !is_trigger_object(objects.id[obj])) continue;
                
                if (!GET_ACTIVATED(obj)) {
                    if (objects.touch_triggered[obj]) {
                        // Try p1
                        if (intersect(
                            state.player.x, state.player.y, state.player.width, state.player.height, 0, 
                            objects.x[obj], objects.y[obj], objects.width[obj] + TOUCH_TRIGGER_EPSILON, objects.height[obj] + TOUCH_TRIGGER_EPSILON, objects.rotation[obj]
                        )) {
                            run_trigger(obj, false);
                        } else
                        // Try now p2
                        if (intersect(
                            state.player2.x, state.player2.y, state.player2.width, state.player2.height, 0, 
                            objects.x[obj], objects.y[obj], objects.width[obj] + TOUCH_TRIGGER_EPSILON, objects.height[obj] + TOUCH_TRIGGER_EPSILON, objects.rotation[obj]
                        )) {
                            run_trigger(obj, false);
                        }
                    } else if (!trigger_is_spawn_triggered(objects.id[obj], obj) && objects.x[obj] < state.player.x) {
                        if (trigger_count < TRIGGER_BUFFER_SIZE) {
                            triggers_buffer[trigger_count++] = obj;
                        }
                    }
                }
            }
        }
    }

    qsort(triggers_buffer, trigger_count, sizeof(int), compare_triggers);

    for (int i = 0; i < trigger_count; i++) {
        run_trigger(triggers_buffer[i], false);
    }
}

void handle_step_triggers(void) {
    if (has_follow_player_y) {
        player_follow_y_step(&state.player, trigger_dt);
        if (state.dual) player_follow_y_step(&state.player2, trigger_dt);
    }
    handle_triggers();
    handle_touch_triggers();
    handle_count_triggers();
    handle_spawn_triggers();
    handle_move_triggers();
    handle_follow_player_y_triggers();
    handle_rotate_triggers();
    handle_follow_triggers();
}

// https://github.com/gd-programming/gd.docs/issues/87
void calculate_lbg() {
    ColorChannel channel = channels[get_col_channel_index(CHANNEL_BG)];
    float h,s,v;
    
    convertRGBtoHSV(channel.color.r, channel.color.g, channel.color.b, &h, &s, &v);

    s -= 0.20f;
    s = clampf(s, 0.f, 1.f);
    v += 0.20f;
    v = clampf(v, 0.f, 1.f);

    unsigned char r,g,b;

    convertHSVtoRGB(h, s, v, &r, &g, &b);

    int chan_lbg_nolerp = get_col_channel_index(CHANNEL_LBG_NOLERP);
    channels[chan_lbg_nolerp].color.r = r;
    channels[chan_lbg_nolerp].color.g = g;
    channels[chan_lbg_nolerp].color.b = b;
    channels[chan_lbg_nolerp].blending = true;

    float factor = (channel.color.r + channel.color.g + channel.color.b) / 150.f;

    if (factor < 1.f) {
        Color p1 = get_white_if_black(p1_color);
        r = r * factor + p1.r * (1 - factor);
        g = g * factor + p1.g * (1 - factor);
        b = b * factor + p1.b * (1 - factor);
    }

    // Set here lerped LBG
    int chan_lbg = get_col_channel_index(CHANNEL_LBG);
    channels[chan_lbg].color.r = r;
    channels[chan_lbg].color.g = g;
    channels[chan_lbg].color.b = b;
    channels[chan_lbg].blending = true;
}