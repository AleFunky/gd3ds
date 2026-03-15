#pragma once
#include "objects.h"
#include <3ds.h>

#define MAX_GROUPS_PER_OBJECT 20

#define SECTION_HASH_SIZE 600

#define SECTION_SIZE 100

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
    bool *tintGround;
    bool *p1_color, *p2_color;
    bool *blending;
    bool *touch_triggered;
    bool *flippedH, *flippedV;

    u8 *activated;
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

    int x; // Section coordinates
    struct Section *next; // For chaining in hash map
} Section;

extern ObjectsArray objects;

char *read_file(const char *filepath, size_t *out_size);
char *decompress_level(char *data);
int load_level(char *path);
void unload_level();
Section *get_or_create_section(int x);
bool obj_has_main(const GameObject *obj);
bool obj_has_detail(const GameObject *obj);