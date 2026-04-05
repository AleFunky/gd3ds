#pragma once
#include "objects.h"
#include <3ds.h>

#define MAX_GROUPS_PER_OBJECT 20

#define SECTION_HASH_SIZE 1024

#define SECTION_SIZE 128

typedef enum {
    GD_VAL_INT,
    GD_VAL_FLOAT,
    GD_VAL_BOOL,
    GD_VAL_UNKNOWN
} GDValueType;

typedef union {
    int i;
    float f;
    bool b;
} GDValue;

typedef struct {
    int count;

    int *random;

    int *id;
    float *x, *y;
    float *rotation;
    int *zlayer, *zorder;
    float *trig_duration;

    float *width, *height;

    unsigned short *v1p9_col_channel;
    unsigned short *col_channel;
    unsigned short *detail_col_channel;
    unsigned short *target_color_id;

    unsigned char *transition_applied;
    unsigned char *trig_colorR, *trig_colorG, *trig_colorB;
    unsigned char *orientation;
    unsigned char *hitbox_counter;
    bool *tintGround;
    bool *p1_color, *p2_color;
    bool *blending;
    bool *touch_triggered;
    bool *flippedH, *flippedV;

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
    int toRed;
    int toGreen;
    int toBlue;
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
    bool level_is_empty;
    bool level_is_custom;

    char *level_name;
} LoadedLevelInfo;

extern LoadedLevelInfo level_info;

#define BG_COUNT 7
#define G_COUNT 7

extern ObjectsArray objects;

char *read_file(const char *filepath, size_t *out_size);
char *decompress_level(char *data);

int load_level(char *path);
void reload_level();
void unload_level();

Section *get_or_create_section(int x, int y);
bool obj_has_main(const GameObject *obj);
bool obj_has_detail(const GameObject *obj);

char *get_level_name(char *data_ptr);
char *load_user_song(int id, size_t *out_size); 
bool check_song(int id);
char *extract_gmd_key(const char *data, const char *key, const char *type);