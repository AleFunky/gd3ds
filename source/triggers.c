#include <citro2d.h>
#include "triggers.h"
#include "color.h"
#include "icons.h"
#include "math_helpers.h"
#include <float.h>
#include <math.h>
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "groups.h"
#include "easing.h"
#include "objects.h"
#include "player/collision.h"

#include <stdlib.h>
#include <string.h>

#include "player/player.h"
#include "state.h"

#define LAUNCH_CAP 1080.f

Color p1_color;
Color p2_color;
Color glow_color;

float g_trigger_dt = 0.f;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];
AlphaTriggerBuffer *alpha_trigger_buffer = NULL;
int alpha_trigger_count = 0;
int alpha_trigger_capacity = 0;

MoveTriggerBuffer *move_trigger_buffer = NULL;
int move_trigger_count = 0;
int move_trigger_capacity = 0;

SpawnTriggerBuffer *spawn_trigger_buffer = NULL;
int spawn_trigger_count = 0;
int spawn_trigger_capacity = 0;

PulseTriggerBuffer *pulse_trigger_buffer = NULL;
int pulse_trigger_count = 0;
int pulse_trigger_capacity = 0;

AlphaTriggerBuffer *get_new_alpha_trigger() {
    if (alpha_trigger_count >= alpha_trigger_capacity) {
        size_t new_capacity = alpha_trigger_capacity == 0 ? 32 : alpha_trigger_capacity + 32;

        AlphaTriggerBuffer *new_list = realloc(alpha_trigger_buffer, new_capacity * sizeof(AlphaTriggerBuffer));
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

        MoveTriggerBuffer *new_list = realloc(move_trigger_buffer, new_capacity * sizeof(MoveTriggerBuffer));
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

SpawnTriggerBuffer *get_new_spawn_trigger() {
    if (spawn_trigger_count >= spawn_trigger_capacity) {
        size_t new_capacity = spawn_trigger_capacity == 0 ? 32 : spawn_trigger_capacity + 32;

        SpawnTriggerBuffer *new_list = realloc(spawn_trigger_buffer, new_capacity * sizeof(SpawnTriggerBuffer));
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

PulseTriggerBuffer *get_new_pulse_trigger() {
    if (pulse_trigger_count >= pulse_trigger_capacity) {
        size_t new_capacity = pulse_trigger_capacity == 0 ? 32 : pulse_trigger_capacity + 32;

        PulseTriggerBuffer *new_list = realloc(pulse_trigger_buffer, new_capacity * sizeof(PulseTriggerBuffer));
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

void free_trigger_buffers() {
    free(alpha_trigger_buffer);
    free(move_trigger_buffer);
    free(spawn_trigger_buffer);
    free(pulse_trigger_buffer);

    alpha_trigger_buffer = NULL;
    move_trigger_buffer = NULL;
    spawn_trigger_buffer = NULL;
    pulse_trigger_buffer = NULL;

    alpha_trigger_count = 0;
    alpha_trigger_capacity = 0;

    move_trigger_count = 0;
    move_trigger_capacity = 0;

    spawn_trigger_count = 0;
    spawn_trigger_capacity = 0;

    pulse_trigger_count = 0;
    pulse_trigger_capacity = 0;
}

static unsigned int pulse_activation_counter = 0;
float move_lock_player_x_delta = 0.0f;
float move_lock_player_y_delta = 0.0f;

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
        int new_capacity = pool->capacity ? pool->capacity + 8 : 8;

        pool->data = realloc(pool->data, new_capacity * element_size);
        if (!pool->data) return -1;

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

        buffer->time_run += g_trigger_dt;

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

void handle_col_triggers() {
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
        for (GroupNode *cur = p; cur; cur = cur->next) {
            objects.alpha_trigger_opacity[cur->obj] = get_group_opacities(cur->obj);
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

    AlphaTriggerBuffer *buffer;

    if (slot >= 0) {
        buffer = &alpha_trigger_buffer[slot];
    } else {
        buffer = get_new_alpha_trigger();
    }
    if (!buffer) return;

    //if (buffer->restored_from_checkpoint) {
    //    buffer->restored_from_checkpoint = false;
    //    return;
    //}
    buffer->target_group = target_group;
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
            for (GroupNode *cur = p; cur; cur = cur->next) {
                objects.alpha_trigger_opacity[cur->obj] = get_group_opacities(cur->obj);
            }
        }

        buffer->time_run += g_trigger_dt;
        if (buffer->time_run >= buffer->seconds) {
            if (p) {
                p->alpha = buffer->new_alpha;
                for (GroupNode *cur = p; cur; cur = cur->next) {
                    objects.alpha_trigger_opacity[cur->obj] = get_group_opacities(cur->obj);
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

bool object_can_be_x_moved(int obj) {
    int obj_id = objects.id[obj];
    if (is_trigger_object(obj_id) && !objects.touch_triggered[obj]) {
        return false;
    }

    switch (obj_id) {
        // Speed portals
        case 200:
        case 201:
        case 202:
        case 203:
            return false;
    }

    return true;
}


inline float objTop(float y, int object)  { 
    return y + objects.height[object] / 2; 
}
inline float objBot(float y, int object)  { 
    return y - objects.height[object] / 2; 
}
inline float objgravBot(Player *player, float y, int object) { return player->upside_down ? -objTop(y, object) : objBot(y, object); }
inline float objgravTop(Player *player, float y, int object) { return player->upside_down ? -objBot(y, object) : objTop(y, object); }


static bool check_rising_platform(Player *player, int object, float delta_y, float offset_y) {
    if (grav(player, delta_y) <= 0.f) return false;
    
    float player_bottom = gravBottom(player);

    float object_x = objects.x[object];
    float object_y = objects.y[object];
    float object_width = objects.width[object];
    float object_height = objects.height[object];

    float old_top = grav(player, object_y - delta_y + object_height / 2.f);
    float new_top = grav(player, object_y + object_height / 2.f);

    float tolerance = 10.f + fabsf(player->vel_y) * STEPS_DT;

    bool horizontal_overlap = fabsf(player->x - object_x) <= (player->width + object_width) / 2.f;

    bool vertical_overlap = player_bottom >= old_top - tolerance && player_bottom <= new_top + tolerance;

    return horizontal_overlap && vertical_overlap;
}

static bool check_moving_ceiling(Player *player, int object, float delta_y, float offset_y) {
    if (grav(player, delta_y) >= 0.f) return false;
    if (player->gamemode == GAMEMODE_PLAYER || player->gamemode == GAMEMODE_ROBOT) return false;

    float player_top = gravTop(player);

    float object_x = objects.x[object];
    float object_y = objects.y[object];
    float object_width = objects.width[object];

    float old_bottom = objgravBot(player, object_y - delta_y, object);
    float new_bottom = objgravBot(player, object_y, object);

    bool horizontal_overlap = fabsf(player->x - object_x) <= (player->width + object_width) / 2.f;
    
    bool vertical_overlap = player_top >= new_bottom && player_top <= old_bottom;

    return horizontal_overlap && vertical_overlap;
}

static bool has_other_vertical_trigger(MoveTriggerBuffer *buffer) {
    for (int i = 0; i < move_trigger_count; i++) {
        MoveTriggerBuffer *other = &move_trigger_buffer[i];

        if (other == buffer || !other->active || other->target_group != buffer->target_group || other->offset_y == 0.f)
            continue;

        return true;
    }

    return false;
}

static void set_player_to_obj_bottom(Player *player, int obj) {
    player->y = grav(player, objgravTop(player, objects.y[obj], obj)) + grav(player, player->height / 2);
}

static void set_player_to_obj_top(Player *player, int obj) {
    player->y = grav(player, objgravBot(player, objects.y[obj], obj)) - grav(player, player->height / 2);
}
void handle_move_triggers(void) {
    for (int slot = 0; slot < move_trigger_count; slot++) {
        MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];
        if (!buffer->active) continue;

        buffer->time_run += g_trigger_dt;

        float t = easeTime(convert_ease(buffer->easing),
                           buffer->time_run, buffer->seconds, 2.0f);
        float delta_x, delta_y;
        if (buffer->lock_to_player_x) {
            delta_x = move_lock_player_x_delta;
        } else {
            float current_x = buffer->offset_x * t;
            delta_x = current_x - buffer->move_last_x;
            buffer->move_last_x = current_x;
        }
        if (buffer->lock_to_player_y) {
            delta_y = move_lock_player_y_delta;
        } else {
            float current_y = buffer->offset_y * t;
            delta_y = current_y - buffer->move_last_y;
            buffer->move_last_y = current_y;
        }

        bool zero_delta = (delta_x == 0.f && delta_y == 0.f);

        float rise_vel = (buffer->seconds > 0.f && g_trigger_dt != 0.f) ? delta_y / g_trigger_dt : 0.f;
        
        bool player1_can_be_carried = buffer->seconds > 0.f && grav(&state.player, rise_vel) > MINIMUM_OBJECT_SPEED;
        bool player2_can_be_carried = buffer->seconds > 0.f && state.dual && grav(&state.player2, rise_vel) > MINIMUM_OBJECT_SPEED;
        
        float player1_carry = 0.f, player2_carry = 0.f;
        bool player1_support = false, player2_support = false;
        
        bool player1_moving_with_gravity = grav(&state.player, delta_y) < 0.f;
        bool player2_moving_with_gravity = state.dual && grav(&state.player2, delta_y) < 0.f;

        float player1_grav_delta = grav(&state.player, delta_y);
        float player2_grav_delta = grav(&state.player2, delta_y);

        GroupNode *p = get_group(buffer->target_group);
        if (p && !zero_delta) {
            for (GroupNode *cur = p; cur; cur = cur->next) {
                int group_obj = cur->obj;
                int old_sx = objects.section_x[group_obj];
                int old_sy = objects.section_y[group_obj];
                if (objects.flags[group_obj] & FLAG_CAN_BE_X_MOVED) {
                    objects.last_x[group_obj] = objects.x[group_obj];
                    objects.x[group_obj] += delta_x;   
                }

                objects.last_y[group_obj] = objects.y[group_obj];
                objects.y[group_obj] += delta_y;

                // Dirty part
                objects.flags[group_obj] |= FLAG_DIRTY;

                int new_sx = (int)(objects.x[group_obj] / SECTION_SIZE);
                int new_sy = (int)(objects.y[group_obj] / SECTION_SIZE);

                if (new_sx != old_sx || new_sy != old_sy)
                    update_object_section(group_obj);

                int object_id = objects.id[group_obj];

                if (!is_valid_object(object_id)) continue;

                const GameObject *game_object = &game_objects[object_id];
                
                // Ignore no solid hitbox
                if (!game_object->hitbox || game_object->hitbox->type != HITBOX_SOLID) continue;

                if (buffer->seconds <= 0.f) {
                    float launch_vel = 0.f;

                    if (g_trigger_dt != 0.f) {
                        launch_vel = fminf(fabsf(delta_y) / STEPS_DT_UNMOD, LAUNCH_CAP);
                    }

                    if (state.player.collided_block == group_obj && player1_grav_delta > 0.f) {
                        state.player.y += delta_y;
                        state.player.vel_y = launch_vel;
                    }

                    if (state.dual && state.player2.collided_block == group_obj && player2_grav_delta > 0.f) {
                        state.player2.y += delta_y;
                        state.player2.vel_y = launch_vel;
                    }
                }
                // Downwards movement
                else {
                    if (player1_moving_with_gravity || player2_moving_with_gravity) {
                        float cont_vel = (g_trigger_dt != 0.f) ? delta_y / g_trigger_dt : 0.f;

                        if (state.player.collided_block == group_obj && player1_moving_with_gravity && grav(&state.player, cont_vel) >= -MINIMUM_OBJECT_SPEED) {
                            set_player_to_obj_bottom(&state.player, group_obj);
                        }

                        if (state.dual && state.player2.collided_block == group_obj && player2_moving_with_gravity && grav(&state.player2, cont_vel) >= -MINIMUM_OBJECT_SPEED) {
                            set_player_to_obj_bottom(&state.player2, group_obj);
                        }
                    }
                }

                // Players riding object
                if (
                    player1_can_be_carried && player1_carry == 0.f &&
                    check_rising_platform(&state.player, group_obj, delta_y, buffer->offset_y)
                ) {
                    player1_carry = delta_y;
                }

                if (
                    state.dual && player2_can_be_carried && player2_carry == 0.f &&
                    check_rising_platform(&state.player2, group_obj, delta_y, buffer->offset_y)
                ) {
                    player2_carry = delta_y;
                }

                if (check_moving_ceiling(&state.player, group_obj, delta_y, buffer->offset_y)) {
                    set_player_to_obj_top(&state.player, group_obj);
                }
                
                if (state.dual && check_moving_ceiling(&state.player2, group_obj, delta_y, buffer->offset_y)) {
                    set_player_to_obj_top(&state.player2, group_obj);
                }
                
                // Support state
                if (group_obj == state.player.collided_block) player1_support = true;
                if (state.dual && group_obj == state.player2.collided_block) player2_support = true;
            }
            state.player.y += player1_carry;
            if (state.dual) state.player2.y += player2_carry;
        }

        if (buffer->time_run >= buffer->seconds) {
            if (p) {
                for (GroupNode *cur = p; cur; cur = cur->next) {
                    objects.last_x[cur->obj] = objects.x[cur->obj];
                    objects.last_y[cur->obj] = objects.y[cur->obj];
                }
            }
            if (buffer->seconds > 0.f && g_trigger_dt != 0.f && p && (player1_support || player2_support)) {
                float launch;

                if (buffer->time_run - 2.f * g_trigger_dt < 0.f) {
                    launch = fabsf(delta_y) / g_trigger_dt;
                } else {
                    float e1 = easeTime(convert_ease(buffer->easing), buffer->time_run - g_trigger_dt, buffer->seconds, 2.0f);
                    float e2 = easeTime(convert_ease(buffer->easing), buffer->time_run - 2.f * g_trigger_dt, buffer->seconds, 2.0f);

                    launch = fabsf(buffer->offset_y * (e1 - e2)) / g_trigger_dt;
                }

                bool player1_should_launch = player1_support && grav(&state.player, launch) > MINIMUM_OBJECT_SPEED;
                bool player2_should_launch = state.dual && player2_support && grav(&state.player2, launch) > MINIMUM_OBJECT_SPEED;

                if ((player1_should_launch  || player2_should_launch) && !has_other_vertical_trigger(buffer)) {
                    if (player1_should_launch) state.player.vel_y = grav(&state.player, launch);
                    if (player2_should_launch) state.player2.vel_y = grav(&state.player2, launch);
                }
            }

            buffer->active = false;
            for (int j = slot; j < move_trigger_count - 1; j++) {
                // Shift buffer array
                move_trigger_buffer[j] = move_trigger_buffer[j + 1];
            }
            
            // Make the game run the pulse moved into the current slot
            slot--;
            move_trigger_count--;
        }
    }
}

void upload_to_spawn_buffer(int obj, bool from_spawn) {
    for (int i = 0; i < spawn_trigger_count; i++) {
        if (spawn_trigger_buffer[i].active &&
            spawn_trigger_buffer[i].source_obj == obj &&
            spawn_trigger_buffer[i].restored_from_checkpoint) {
            spawn_trigger_buffer[i].restored_from_checkpoint = false;
            return;
        }
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
    for (int slot = 0; slot < spawn_trigger_count; slot++) {
        spawn_trigger_buffer[slot].queued = false;
    }
    for (int slot = 0; slot < spawn_trigger_count; slot++) {
        SpawnTriggerBuffer *buffer = &spawn_trigger_buffer[slot];
        if (!buffer->active || buffer->queued) continue;

        buffer->time_run += g_trigger_dt;
        if (buffer->time_run > buffer->seconds) {
            buffer->active = false;

            for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
                int obj_idx = p->obj;
                if (trigger_is_spawn_triggered(objects.id[obj_idx], obj_idx)
                    && is_trigger_object(objects.id[obj_idx])
                    && (trigger_is_multi_triggered(objects.id[obj_idx], obj_idx) || !GET_ACTIVATED(obj_idx))
                    && !(objects.flags[obj_idx] & FLAG_TOGGLED)) {
                    run_trigger(obj_idx, true);
                }
            }

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


void upload_to_pulse_buffer(int obj) {
    PulseTrigger *trigger = get_pulse_trigger(obj);
    int channel = trigger->target_group;
    if (channel == 0) return;

    PulseTriggerBuffer *buffer = get_new_pulse_trigger();
    if (!buffer) return;

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

    buffer->started_fade_out = false;

    buffer->target_color_id = channel;

    if (buffer->pulse_target_type == PULSE_TARGET_CHANNEL) {
        ColorChannel *chan = &channels[get_col_channel_index(buffer->target_color_id)];
        buffer->pulse_index = chan->num_pulses;
        chan->num_pulses++;
    } else if (buffer->pulse_target_type == PULSE_TARGET_GROUP) {
        bool both = !buffer->main_only && !buffer->detail_only;
        int count = 0;
        for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
            count++;
        }

        buffer->main_pulse_index = malloc(sizeof(int) * count);
        buffer->detail_pulse_index = malloc(sizeof(int) * count);

        if (!buffer->main_pulse_index || !buffer->detail_pulse_index) return;

        int index = 0;
        for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
            int obj_idx = p->obj;
            if (both || buffer->main_only) {
                buffer->main_pulse_index[index] = objects.num_main_pulses[obj_idx]++;
                objects.main_being_pulsed[obj_idx] = true;
            }

            if (both || buffer->detail_only) {
                buffer->detail_pulse_index[index] = objects.num_detail_pulses[obj_idx]++;
                objects.detail_being_pulsed[obj_idx] = true;
            }
            index++;
        }
    }

    buffer->time_run = 0;
    buffer->seconds = trigger->pulse_fade_in + trigger->pulse_hold + trigger->pulse_fade_out;
    buffer->activation_order = pulse_activation_counter++;
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

    // process group pulses in ascending activation_order
    int group_order[pulse_trigger_count];
    int group_order_count = 0;
    for (int i = 0; i < pulse_trigger_count; i++) {
        PulseTriggerBuffer *b = &pulse_trigger_buffer[i];
        if (!b->active || b->pulse_target_type != PULSE_TARGET_GROUP) continue;
        group_order[group_order_count++] = i;
    }
    // insertion sort by activation_order ascending
    for (int a = 1; a < group_order_count; a++) {
        int key = group_order[a];
        unsigned int key_order = pulse_trigger_buffer[key].activation_order;
        int b = a - 1;
        while (b >= 0 && pulse_trigger_buffer[group_order[b]].activation_order > key_order) {
            group_order[b + 1] = group_order[b];
            b--;
        }
        group_order[b + 1] = key;
    }

    for (int gi = 0; gi < group_order_count; gi++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[group_order[gi]];
        bool both = !buffer->main_only && !buffer->detail_only;
        for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
            int obj_idx = p->obj;
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

    for (int gi = 0; gi < group_order_count; gi++) {
        PulseTriggerBuffer *buffer = &pulse_trigger_buffer[group_order[gi]];
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

        for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
            int obj_idx = p->obj;
            if (obj_idx < 0 || obj_idx >= objects.count) continue;
            if (both || buffer->main_only) {
                Color target = buffer->color;
                if (objects.main_col_HSV_enabled[obj_idx]) target = HSV_combine(target, objects.main_col_HSV[obj_idx]);
                Color base = objects.main_color[obj_idx];
                objects.main_color[obj_idx].r = (unsigned char)(base.r + (target.r - base.r) * alpha);
                objects.main_color[obj_idx].g = (unsigned char)(base.g + (target.g - base.g) * alpha);
                objects.main_color[obj_idx].b = (unsigned char)(base.b + (target.b - base.b) * alpha);
                objects.main_being_pulsed[obj_idx] = true;
            }
            if (both || buffer->detail_only) {
                Color target = buffer->color;
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

        buffer->time_run += g_trigger_dt;
        if (buffer->time_run > buffer->seconds) {
            if (buffer->pulse_target_type == PULSE_TARGET_GROUP) {
                bool both = !buffer->main_only && !buffer->detail_only;
                for (GroupNode *p = get_group(buffer->target_group); p; p = p->next) {
                    int obj_idx = p->obj;
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
                
                free(buffer->main_pulse_index);
                free(buffer->detail_pulse_index);
            
                buffer->main_pulse_index = NULL;
                buffer->detail_pulse_index = NULL;
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
    buffer->active = true;
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
            GroupNode *p = get_group(trigger->target_group);
            if (p) {
                p->toggled = !trigger->activate_group;

                for (; p; p = p->next) {
                    objects.flags[p->obj] &= ~FLAG_TOGGLED;
                    objects.flags[p->obj] |= get_group_toggles(p->obj) ? FLAG_TOGGLED : 0;
                }
            }
            break;
        }
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

void handle_triggers() {
    trigger_count = 0;
    int cam_sx = (int)((state.player.x) / SECTION_SIZE);
    
    for (int sx = -1; sx < 1; sx++) {
        for (int sy = -(400 / SECTION_SIZE); sy <= MAX_LEVEL_HEIGHT / SECTION_SIZE; sy++) {
            int sec_x = cam_sx + sx;
            int sec_y = sy;
            if (sec_x < 0) continue;

            Section *sec = get_section(sec_x, sec_y);
            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];
                
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

    for (size_t i = 0; i < trigger_count; i++) {
        run_trigger(triggers_buffer[i], false);
    }
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