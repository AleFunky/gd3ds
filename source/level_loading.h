#pragma once
#include "objects.h"
#include "utils/server_utils.h"
#include <3ds.h>

#define MAX_GROUPS_PER_OBJECT 20

#define SECTION_HASH_SIZE 1024

#define SECTION_SIZE 128

#define MAX_PULSES_PER_GROUP 5

typedef struct {
    unsigned char r,g,b;
} Color;

typedef struct {
    float h;
    float s;
    float v;
    bool sChecked;
    bool vChecked;
} HSV;

typedef enum {
    GD_VAL_INT,
    GD_VAL_FLOAT,
    GD_VAL_BOOL,
    GD_VAL_HSV,
    GD_VAL_INT_ARRAY,
    GD_VAL_UNKNOWN
} GDValueType;

typedef union {
    int i;
    float f;
    bool b;
    HSV hsv;
    short int_array[MAX_GROUPS_PER_OBJECT];
} GDValue;

typedef struct {
    int count;

    int *random;

    int *id;
    float *x, *y;
    float *rotation;
    int *zlayer, *zorder;
    float *trig_duration;
    float *opacity;

    float *width, *height;

    unsigned short *v1p9_col_channel;
    unsigned short *col_channel;
    unsigned short *detail_col_channel;
    unsigned short *target_color_id;

    bool *spawn_triggered;
    bool *multi_triggered;
    float *spawn_delay;
    int *target_group;
    bool *activate_group;
    float *alpha_trigger_opacity;
    float *trigger_opacity;

    float *move_offset_x;
    float *move_offset_y;
    int *move_easing;
    bool *lock_to_player_x;
    bool *lock_to_player_y;

    float *original_x;
    float *original_y;

    Color (*main_pulses)[MAX_PULSES_PER_GROUP];
    Color (*detail_pulses)[MAX_PULSES_PER_GROUP];
    u8 *num_main_pulses;
    u8 *num_detail_pulses;
    bool *main_being_pulsed;
    bool *detail_being_pulsed;
    Color *main_non_pulse_color;
    Color *detail_non_pulse_color;
    Color *main_color;
    Color *detail_color;

    float *pulse_fade_in;
    float *pulse_hold;
    float *pulse_fade_out;
    int *pulse_mode;
    int *copied_color_id;
    HSV *copied_hsv;
    bool *main_col_HSV_enabled;
    bool *detail_col_HSV_enabled;
    HSV *main_col_HSV;
    HSV *detail_col_HSV;
    int *pulse_target_type;
    bool *pulse_main_only;
    bool *pulse_detail_only;

    Color *cached_main_hsv_src_color;
    Color *cached_detail_hsv_src_color;
    Color *cached_main_hsv_color;
    Color *cached_detail_hsv_color;
    bool *cached_main_hsv_valid;
    bool *cached_detail_hsv_valid;

    float *scale_x, *scale_y;
    float *original_scale_x, *original_scale_y;

    int *child_object;
    float *tp_y_offset;
    int *section_x;
    int *section_y;

    unsigned char *transition_applied;
    unsigned char *trig_colorR, *trig_colorG, *trig_colorB;
    unsigned char *orientation;
    unsigned char *hitbox_counter;
    bool *tintGround;
    bool *p1_color, *p2_color;
    bool *blending;
    union {
        bool *touch_triggered;
        u8 *coin_id;
    };
    bool *flippedH, *flippedV;
    bool *toggled;

    short (*groups)[MAX_GROUPS_PER_OBJECT];
    u8 *group_count;
    bool *dirty;
    bool *render_visible;
    bool *render_seen;

    u8 *activated;
    u8 *collided;
} ObjectsArray;

typedef struct {
    int fromRed;
    int fromGreen;
    int fromBlue;
    int playerColor;
    bool blending;
    int channelID;
    float fromOpacity;
    bool toggleOpacity;
    int inheritedChannelID;
    HSV hsv;
    int toRed;
    int toGreen;
    int toBlue;
    float deltaTime;
    float toOpacity;
    float duration;
    bool copyOpacity;
} GDColorChannel;


typedef struct Section {
    int *objects;
    int object_count;
    int object_capacity;

    int x, y; // Section coordinates
    struct Section *next; // For chaining in hash map
} Section;

typedef struct {
    float last_obj_x;
    float wall_x;
    float wall_y;

    int pulsing_type;
    int song_id;
    int custom_song_id;
    float song_offset;
    bool completing;
    int background_id;
    int ground_id;
    int initial_gamemode;
    bool initial_mini;
    unsigned char initial_speed;
    bool initial_dual;
    bool initial_upsidedown;

    bool two_player_mode;

    char level_name[256];
    char creator_name[256];
} LoadedLevelInfo;


typedef enum {
    LOAD_NO_ERROR,
    LOAD_INVALID_GMD,
    LOAD_INVALID_COMPRESSED_DATA,
    LOAD_LEVEL_STRING_MISSING_SECTIONS,
    LOAD_OUT_OF_MEMORY,
    LOAD_COULDNT_PARSE_OBJECTS,
    LOAD_INVALID_BASE64,
    LOAD_ERROR_COUNT,
} LevelLoadError;

extern const char *error_strings[LOAD_ERROR_COUNT - 1];

extern LoadedLevelInfo level_info;

extern const char *default_name;

extern const char *level_lengths[5];

#define BG_COUNT 7
#define G_COUNT 7

extern char *curr_level_string;

extern ObjectsArray objects;

extern int channelCount;
extern GDColorChannel *colorChannels;

char *read_file(const char *filepath, size_t *out_size);
char *decompress_level(char *data, int *out_code);

int load_level(char *path);
int load_online_level(char *level_string);
void reload_level();
void unload_level();

void fix_base64_url(char *b64);
int base64_decode(const char *in, unsigned char *out);

Section *get_section(int x, int y);
Section *get_or_create_section(int x, int y);
void assign_object_to_section(int obj);
void update_object_section(int obj);
bool obj_has_main(const GameObject *obj);
bool obj_has_detail(const GameObject *obj);

bool is_valid_object(int id);
bool is_trigger_object(int id);

char *get_level_name(char *data_ptr);
char *load_user_song(int id, size_t *out_size); 
bool check_song(int id);
char *extract_gmd_key(const char *data, const char *key, const char *type);

char **split_string(const char *str, char delimiter, int *outCount, bool ignoreZeroLength);
char **split_string_str_del(const char *str, const char *delimiter, int *outCount, bool ignoreZeroLength);
void free_string_array(char **arr, int count);