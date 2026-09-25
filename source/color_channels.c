#include <citro2d.h>
#include "color_channels.h"
#include "color.h"
#include "math_helpers.h"
#include <math.h>
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "groups.h"
#include "easing.h"
#include "player/collision.h"

#include <stdlib.h>
#include <string.h>

#include "state.h"

Color p1_color;
Color p2_color;
Color glow_color;

float g_trigger_dt = 0.f;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];
AlphaTriggerBuffer alpha_trigger_buffer[MAX_ALPHA_TRIGGERS];
MoveTriggerBuffer move_trigger_buffer[MAX_MOVE_TRIGGERS];
float move_lock_player_x_delta = 0.0f;
float move_lock_player_y_delta = 0.0f;

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
        memset(channels[chan].pulses, 0, sizeof(channels[chan].pulses));
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
        for (GroupNode *cur = p; cur; cur = cur->next) {
            objects.alpha_trigger_opacity[cur->obj] = alpha;
        }
        return;
    }

    int slot = -1;
    for (int i = 0; i < MAX_ALPHA_TRIGGERS; i++) {
        if (!alpha_trigger_buffer[i].active) {
            slot = i;
            break;
        }
    }

    for (int i = 0; i < MAX_ALPHA_TRIGGERS; i++) {
        if (alpha_trigger_buffer[i].active && alpha_trigger_buffer[i].target_group == target_group) {
            slot = i;
            break;
        }
    }

    if (slot < 0) return;

    AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[slot];
    buffer->target_group = target_group;
    buffer->new_alpha = trigger->trigger_opacity;
    buffer->old_alpha = objects.alpha_trigger_opacity[p->obj];
    buffer->seconds = trigger->trig_duration;
    buffer->time_run = 0;
    buffer->active = true;
}

void handle_alpha_triggers(void) {
    for (int slot = 0; slot < MAX_ALPHA_TRIGGERS; slot++) {
        AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[slot];
        if (!buffer->active) continue;

        float multiplier = buffer->time_run / buffer->seconds;
        float lerped = buffer->old_alpha + (buffer->new_alpha - buffer->old_alpha) * multiplier;

        GroupNode *p = get_group(buffer->target_group);
        if (p) {
            for (GroupNode *cur = p; cur; cur = cur->next) {
                objects.alpha_trigger_opacity[cur->obj] = lerped;
            }
        }

        buffer->time_run += g_trigger_dt;
        if (buffer->time_run >= buffer->seconds) {
            GroupNode *pg = get_group(buffer->target_group);
            if (pg) {
                for (GroupNode *cur = pg; cur; cur = cur->next) {
                    objects.alpha_trigger_opacity[cur->obj] = buffer->new_alpha;
                }
            }
            buffer->active = false;
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

    int slot = -1;
    for (int i = 0; i < MAX_MOVE_TRIGGERS; i++) {
        if (!move_trigger_buffer[i].active) { slot = i; break; }
    }

    if (slot < 0) return;

    MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];
    buffer->target_group = target_group;
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

void handle_move_triggers(void) {
    for (int slot = 0; slot < MAX_MOVE_TRIGGERS; slot++) {
        MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];
        if (!buffer->active) continue;

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

        GroupNode *p = get_group(buffer->target_group);
        if (p && !zero_delta) {
            for (GroupNode *cur = p; cur; cur = cur->next) {
                int group_obj = cur->obj;
                int old_sx = objects.section_x[group_obj];
                int old_sy = objects.section_y[group_obj];
                objects.x[group_obj] += delta_x;
                objects.y[group_obj] += delta_y;
                objects.dirty[group_obj] = true;
                int new_sx = (int)(objects.x[group_obj] / SECTION_SIZE);
                int new_sy = (int)(objects.y[group_obj] / SECTION_SIZE);
                if (new_sx != old_sx || new_sy != old_sy) {
                    update_object_section(group_obj);
                }
            }
        }

        buffer->time_run += g_trigger_dt;
        if (buffer->time_run >= buffer->seconds) {
            buffer->active = false;
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

void run_trigger(int obj) {
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
            upload_to_buffer(obj, get_color_trigger(obj)->target_color_id);
            break;
        case ALPHA_TRIGGER:
            upload_to_alpha_buffer(obj);
            break;
        case MOVE_TRIGGER:
            upload_to_move_buffer(obj);
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
                            objects.x[obj], objects.y[obj], 30, 30, objects.rotation[obj]
                        )) {
                            run_trigger(obj);
                        } else
                        // Try now p2
                        if (intersect(
                            state.player2.x, state.player2.y, state.player2.width, state.player2.height, 0, 
                            objects.x[obj], objects.y[obj], 30, 30, objects.rotation[obj]
                        )) {
                            run_trigger(obj);
                        }
                    } else if (objects.x[obj] < state.player.x) {
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
        run_trigger(triggers_buffer[i]);
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