#include <citro2d.h>
#include "color_channels.h"
#include "color.h"
#include "math_helpers.h"
#include <math.h>
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "player/collision.h"

#include <stdlib.h>

#include "state.h"

Color p1_color;
Color p2_color;
Color glow_color;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];

// Convert channel id to buffer index
int get_col_channel_index(int channel) {
    if (channel >= CHANNEL_SPECIAL_START) {
        return (channel - CHANNEL_SPECIAL_START) + CHANNEL_NORMAL_END;
    } else if (channel < 0 || channel >= CHANNEL_NORMAL_END) {
        return 0;
    }

    return channel;
}

// Convert from buffer index to channel id
int get_col_channel_from_index(int index) {
    if (index >= CHANNEL_NORMAL_END) {
        return (index - CHANNEL_NORMAL_END) + CHANNEL_SPECIAL_START;
    } else if (index < 0 || index >= COL_CHANNEL_NUM) {
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

void init_col_channels() {
    memset(col_trigger_buffer, 0, sizeof(col_trigger_buffer));

    channels[0].color.r = 0;
    channels[0].color.g = 0;
    channels[0].color.b = 0;
    channels[0].blending = false;

    for (size_t chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        channels[chan].color.r = 255;
        channels[chan].color.g = 255;
        channels[chan].color.b = 255;
        channels[chan].blending = false;
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

    int blue_glow = get_col_channel_index(CHANNEL_BLUE_GLOW);
    channels[blue_glow].color.r = 0;
    channels[blue_glow].color.g = 255;
    channels[blue_glow].color.b = 255;
    channels[blue_glow].blending = true;

    int yellow_glow = get_col_channel_index(CHANNEL_YELLOW_GLOW);
    channels[yellow_glow].color.r = 255;
    channels[yellow_glow].color.g = 255;
    channels[yellow_glow].color.b = 0;
    channels[yellow_glow].blending = true;

    int pink_glow = get_col_channel_index(CHANNEL_PINK_GLOW);
    channels[pink_glow].color.r = 255;
    channels[pink_glow].color.g = 0;
    channels[pink_glow].color.b = 255;
    channels[pink_glow].blending = true;

    int white_chn = get_col_channel_index(CHANNEL_WHITE);
    channels[white_chn].color.r = 255;
    channels[white_chn].color.g = 255;
    channels[white_chn].color.b = 255;
    channels[white_chn].blending = true;
    
    int invis_glow = get_col_channel_index(CHANNEL_INVISIBLE_GLOW);
    channels[invis_glow].color.r = 255;
    channels[invis_glow].color.g = 255;
    channels[invis_glow].color.b = 255;
    channels[invis_glow].blending = true;
}

void handle_col_channel(int chan) {
    int channel = get_col_channel_index(chan);

    ColTriggerBuffer *buffer = &col_trigger_buffer[channel];

    if (buffer->active) {
        Color lerped_color;
        Color color_to_lerp = buffer->new_color;

        if (buffer->seconds > 0) {
            float multiplier = buffer->time_run / buffer->seconds;
            lerped_color = color_lerp(buffer->old_color, color_to_lerp, multiplier);
        } else {
            lerped_color = color_to_lerp;
        }

        channels[channel].color = lerped_color;

        buffer->time_run += DT;

        if (buffer->time_run > buffer->seconds) {
            buffer->active = false;
            channels[channel].color = color_to_lerp;
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

void upload_to_buffer(int obj, int channel) {
    if (channel == 0) channel = 1;
    int buffer_channel = get_col_channel_index(channel);

    ColTriggerBuffer *buffer = &col_trigger_buffer[buffer_channel];
    buffer->old_color = channels[buffer_channel].color;
    if (objects.p1_color[obj]) {
        buffer->new_color = get_p2_if_black(p1_color);
    } else if (objects.p2_color[obj]) {
        buffer->new_color = get_p1_if_black(p2_color);
    } else {
        buffer->new_color.r = objects.trig_colorR[obj];
        buffer->new_color.g = objects.trig_colorG[obj];
        buffer->new_color.b = objects.trig_colorB[obj];
    }

    if (channel < CHANNEL_BG) {
        channels[buffer_channel].blending = objects.blending[obj];
    }
    
    
    if (objects.trig_duration[obj] == 0) {
        Color color_to_lerp = buffer->new_color;

        channels[buffer_channel].color = color_to_lerp;
        return;
    }
    
    buffer->seconds = objects.trig_duration[obj];
    buffer->time_run = 0;
    buffer->active = true;
}

void upload_color_to_buffer(int channel, u32 color, float seconds) {
    int buffer_channel = get_col_channel_index(channel);

    ColTriggerBuffer *buffer = &col_trigger_buffer[buffer_channel];
    buffer->old_color = channels[buffer_channel].color;
    buffer->new_color.r = GET_R(color);
    buffer->new_color.g = GET_G(color);
    buffer->new_color.b = GET_B(color);
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
            if (!objects.tintGround[obj]) break;
        
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

        case ENABLE_TRAIL:
            p1_trail = true;
            break;
        
        case DISABLE_TRAIL:
            p1_trail = false;
            break;

        case COL_TRIGGER: // 2.0 color trigger
            upload_to_buffer(obj, objects.target_color_id[obj]);
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