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
    CHANNEL_NORMAL_END,
    CHANNEL_SPECIAL_START = 1000,
    CHANNEL_BG = CHANNEL_SPECIAL_START,
    CHANNEL_GROUND,
    CHANNEL_LINE,
    CHANNEL_3DL,
    CHANNEL_OBJ,
    CHANNEL_P1,
    CHANNEL_P2,
    CHANNEL_LBG,
    CHANNEL_LBG_NOLERP,
    CHANNEL_GROUND_2,
    CHANNEL_BLACK,
    CHANNEL_WHITE,
    CHANNEL_LIGHTER,
    CHANNEL_BLUE_GLOW,
    CHANNEL_PINK_GLOW,
    CHANNEL_INVISIBLE_GLOW,
    CHANNEL_WHITE_GLOW,
    CHANNEL_OBJ_BLENDING,
    CHANNEL_YELLOW_GLOW_INTERNAL,
    COL_CHANNEL_LAST,
    COL_CHANNEL_NUM = 1024,
};

#define MAX_PULSES_PER_CHANNEL 10

typedef struct {
    Color color;
    Color non_pulse_color;
    float alpha;
    bool blending;
    HSV hsv;
    int copy_color_id;
    Color pulses[MAX_PULSES_PER_CHANNEL];
    int num_pulses;
} ColorChannel;

typedef struct {
    bool active;
    Color old_color;
    Color new_color;
    float old_alpha;
    float new_alpha;
    float seconds;
    float time_run;
    int copied_color_id;
    HSV copied_hsv;
} ColTriggerBuffer;

#define MAX_ALPHA_TRIGGERS 50

typedef struct {
    bool active;
    int target_group;
    float old_alpha;
    float new_alpha;
    float seconds;
    float time_run;
} AlphaTriggerBuffer;

#define MAX_MOVE_TRIGGERS 50

typedef struct {
    bool active;
    int target_group;
    float offset_x;
    float offset_y;
    int easing;
    bool lock_to_player_x;
    bool lock_to_player_y;
    float move_last_x;
    float move_last_y;
    float seconds;
    float time_run;
} MoveTriggerBuffer;

extern ColorChannel channels[COL_CHANNEL_NUM];
extern ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_NUM];
extern AlphaTriggerBuffer alpha_trigger_buffer[MAX_ALPHA_TRIGGERS];
extern MoveTriggerBuffer move_trigger_buffer[MAX_MOVE_TRIGGERS];
extern float move_lock_player_x_delta;
extern float move_lock_player_y_delta;

extern Color p1_color;
extern Color p2_color;
extern Color glow_color;

extern float g_trigger_dt;

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
#define GROUND_2_TRIGGER 900
#define MOVE_TRIGGER 901
#define ALPHA_TRIGGER 1007
#define TOGGLE_TRIGGER 1049
#define PULSE_TRIGGER 1006
#define SPAWN_TRIGGER 1268

#define TRIGGER_FADE_SIMPLE 22
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

Color HSV_combine(Color base, HSV hsv);

#define TRIGGER_AT(pool, type, id) (&((type *)(pool).data)[id])


int trigger_pool_add(TriggerPool *pool, size_t element_size);

void calculate_lbg();
int get_col_channel_index(int channel);
int get_col_channel_from_index(int index);
void init_col_channels();
void handle_col_channel(int chan);
void handle_col_triggers();
void handle_copy_channels();
void handle_triggers();
void upload_to_alpha_buffer(int obj);
void handle_alpha_triggers(void);
void upload_to_move_buffer(int obj);
void handle_move_triggers(void);
void upload_color_to_buffer(int channel, u32 color, float seconds);
void upload_to_buffer(int obj, int channel);
int convert_one_point_nine_channel(int channel);
ColTriggerBuffer *get_buffer(int chan);