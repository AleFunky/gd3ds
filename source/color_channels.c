#include <citro2d.h>
#include "color_channels.h"
#include "color.h"
#include "math_helpers.h"
#include <math.h>
#include "level_loading.h"
#include "main.h"
#include "graphics.h"

Color p1_color;
Color p2_color;

ColorChannel channels[COL_CHANNEL_NUM];

ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];

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
    p1_color.r = 0;
    p1_color.g = 255;
    p1_color.b = 0;

    p2_color.r = 0;
    p2_color.g = 255;
    p2_color.b = 255;
    
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

    channels[CHANNEL_BG].color.r = 0;
    channels[CHANNEL_BG].color.g = 5;
    channels[CHANNEL_BG].color.b = 100;
    
    channels[CHANNEL_GROUND].color.r = 40;
    channels[CHANNEL_GROUND].color.g = 125;
    channels[CHANNEL_GROUND].color.b = 255;
       
    channels[CHANNEL_LINE].color.r = 255;
    channels[CHANNEL_LINE].color.g = 255;
    channels[CHANNEL_LINE].color.b = 255;
    channels[CHANNEL_LINE].blending = true;
    
    channels[CHANNEL_OBJ].color.r = 255;
    channels[CHANNEL_OBJ].color.g = 255;
    channels[CHANNEL_OBJ].color.b = 255;
    
    channels[CHANNEL_3DL].color.r = 255;
    channels[CHANNEL_3DL].color.g = 255;
    channels[CHANNEL_3DL].color.b = 255;
    
    channels[CHANNEL_P1].color = p1_color;
    channels[CHANNEL_P1].blending = true;
        
    channels[CHANNEL_P2].color = p2_color;
    channels[CHANNEL_P2].blending = true;
    
    channels[CHANNEL_LBG].color.r = 255;
    channels[CHANNEL_LBG].color.g = 255;
    channels[CHANNEL_LBG].color.b = 255;
    channels[CHANNEL_LBG].blending = true;
}

void handle_col_channel(int chan) {
    ColTriggerBuffer *buffer = &col_trigger_buffer[chan];

    if (buffer->active) {
        Color lerped_color;
        Color color_to_lerp = buffer->new_color;

        if (buffer->seconds > 0) {
            float multiplier = buffer->time_run / buffer->seconds;
            lerped_color = color_lerp(buffer->old_color, color_to_lerp, multiplier);
        } else {
            lerped_color = color_to_lerp;
        }

        channels[chan].color = lerped_color;

        buffer->time_run += 1/60.f;

        if (buffer->time_run > buffer->seconds) {
            buffer->active = false;
            channels[chan].color = color_to_lerp;
        }
    }
}

void handle_col_triggers() {
    for (int chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        handle_col_channel(chan);
    }
}


void upload_to_buffer(Object *obj, int channel) {
    if (channel == 0) channel = 1;
    ColTriggerBuffer *buffer = &col_trigger_buffer[channel];
    buffer->old_color = channels[channel].color;
    if (obj->p1_color) {
        buffer->new_color.r = p1_color.r;
        buffer->new_color.g = p1_color.g;
        buffer->new_color.b = p1_color.b;
    } else if (obj->p2_color) {
        buffer->new_color.r = p2_color.r;
        buffer->new_color.g = p2_color.g;
        buffer->new_color.b = p2_color.b;
    } else {
        buffer->new_color.r = obj->trig_colorR;
        buffer->new_color.g = obj->trig_colorG;
        buffer->new_color.b = obj->trig_colorB;
    }

    if (channel < CHANNEL_BG) {
        channels[channel].blending = obj->blending;
    }
    
    
    if (obj->trig_duration == 0) {
        Color color_to_lerp = buffer->new_color;

        channels[channel].color = color_to_lerp;
        return;
    }
    
    buffer->seconds = obj->trig_duration;
    buffer->time_run = 0;
    buffer->active = true;
}

void upload_color_to_buffer(int channel, u32 color, float seconds) {
    ColTriggerBuffer *buffer = &col_trigger_buffer[channel];
    buffer->old_color = channels[channel].color;
    buffer->new_color.r = GET_R(color);
    buffer->new_color.g = GET_G(color);
    buffer->new_color.b = GET_B(color);
    buffer->seconds = seconds;
    buffer->time_run = 0;
    buffer->active = true;
}

void run_trigger(Object *obj) {
    switch (obj->id) {
        case TRIGGER_FADE_NONE:
            current_fading_effect = FADE_NONE;
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
            if (!obj->tintGround) break;
        
        case GROUND_TRIGGER:
            upload_to_buffer(obj, CHANNEL_GROUND);
            break;
                    
        case LINE_TRIGGER:
        case V2_0_LINE_TRIGGER: // gd converts 1.4 line trigger to 2.0 one for some reason
            upload_to_buffer(obj, CHANNEL_LINE);
            break;
        
        case OBJ_TRIGGER:
            upload_to_buffer(obj, CHANNEL_OBJ);
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

//        case ENABLE_TRAIL:
//            p1_trail = TRUE;
//            break;
//        
//        case DISABLE_TRAIL:
//            p1_trail = FALSE;
//            break;

        case COL_TRIGGER: // 2.0 color trigger
            upload_to_buffer(obj, obj->target_color_id);
            break;
    }
    obj->activated = true;
}

void handle_triggers() {
    int cam_sx = (int)((cam_x + SCREEN_WIDTH / 2) / SECTION_SIZE);
    
    int section = cam_sx;
    if (section < 0) return;

    Section *sec = get_or_create_section(section);
    for (int i = 0; i < sec->object_count; i++) {
        Object *obj = sec->objects[i];
        
        if (!obj->activated && !obj->touch_triggered) {
            //if (obj->touch_triggered) {
            //    // Try p1
            //    if (intersect(
            //        player->x, player->y, player->width, player->height, 0, 
            //        *soa_x(obj), *soa_y(obj), obj->width, obj->height, obj->rotation
            //    )) {
            //        run_trigger(obj);
            //    } else
            //    // Try now p2
            //    if (intersect(
            //        player_2->x, player_2->y, player_2->width, player_2->height, 0, 
            //        *soa_x(obj), *soa_y(obj), obj->width, obj->height, obj->rotation
            //    )) {
            //        run_trigger(obj);
            //    }
            if (obj->x < cam_x + SCREEN_WIDTH / 2) {
                run_trigger(obj);
            }
        }
    }
}

// https://github.com/gd-programming/gd.docs/issues/87
void calculate_lbg() {
    ColorChannel channel = channels[CHANNEL_BG];
    float h,s,v;
    
    convertRGBtoHSV(channel.color.r, channel.color.g, channel.color.b, &h, &s, &v);

    s -= 0.20f;
    s = clampf(s, 0.f, 1.f);
    v += 0.20f;
    v = clampf(v, 0.f, 1.f);

    unsigned char r,g,b;

    convertHSVtoRGB(h, s, v, &r, &g, &b);

    float factor = (channel.color.r + channel.color.g + channel.color.b) / 150.f;

    if (factor < 1.f) {
        r = r * factor + p1_color.r * (1 - factor);
        g = g * factor + p1_color.g * (1 - factor);
        b = b * factor + p1_color.b * (1 - factor);
    }

    // Set here lerped LBG
    channels[CHANNEL_LBG].color.r = r;
    channels[CHANNEL_LBG].color.g = g;
    channels[CHANNEL_LBG].color.b = b;
    channels[CHANNEL_LBG].blending = true;
}