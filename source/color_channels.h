#pragma once
#include <3ds.h>
#include <stdbool.h>
#include "level_loading.h"

#define TRIGGER_BUFFER_SIZE 2048

enum ColorChannelIDs {
    NONE,
    COL_1,
    COL_2,
    COL_3,
    COL_4,
    CHANNEL_BG = 1000,
    CHANNEL_GROUND,
    CHANNEL_LINE,
    CHANNEL_3DL,
    CHANNEL_OBJ,
    CHANNEL_P1,
    CHANNEL_P2,
    CHANNEL_LBG,
    CHANNEL_LBG_NOLERP,
    CHANNEL_OBJ_BLENDING,
    COL_CHANNEL_NUM,
};

typedef struct {
    unsigned char r,g,b;
} Color;

typedef struct {
    Color color;
    bool blending;
} ColorChannel;

typedef struct {
    bool active;
    Color old_color;
    Color new_color;
    float seconds;
    float time_run;
} ColTriggerBuffer;

extern ColorChannel channels[COL_CHANNEL_NUM];

extern Color p1_color;
extern Color p2_color;
extern Color glow_color;

#define BG_TRIGGER 29
#define GROUND_TRIGGER 30
#define LINE_TRIGGER 104
#define V2_0_LINE_TRIGGER 915
#define OBJ_TRIGGER 105
#define OBJ_2_TRIGGER 221
#define COL2_TRIGGER 717
#define COL3_TRIGGER 718
#define COL4_TRIGGER 743
#define THREEDL_TRIGGER 744
#define COL_TRIGGER 899

#define TRIGGER_FADE_NONE 22
#define TRIGGER_FADE_UP 23
#define TRIGGER_FADE_DOWN 24
#define TRIGGER_FADE_RIGHT 26
#define TRIGGER_FADE_LEFT 25
#define TRIGGER_FADE_SCALE_IN 27
#define TRIGGER_FADE_SCALE_OUT 28
#define TRIGGER_FADE_INWARDS 58
#define TRIGGER_FADE_OUTWARDS 59 
#define TRIGGER_FADE_LEFT_SEMICIRCLE 56
#define TRIGGER_FADE_RIGHT_SEMICIRCLE 57

#define GET_R(color) (color & 0xff)
#define GET_G(color) ((color >> 8) & 0xff)
#define GET_B(color) ((color >> 16) & 0xff)

void calculate_lbg();
void init_col_channels();
void handle_col_channel(int chan);
void handle_col_triggers();
void handle_triggers();
void upload_color_to_buffer(int channel, u32 color, float seconds);
void upload_to_buffer(int obj, int channel);
int convert_one_point_nine_channel(int channel);
ColTriggerBuffer *get_buffer(int chan);