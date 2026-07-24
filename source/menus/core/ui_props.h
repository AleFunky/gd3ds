#pragma once

#include <3ds/types.h>

typedef struct {
    const char *key;
    const char *value;
} UIProperty;

typedef struct {
    UIProperty *properties;
    size_t count;
    //This is used for UIPropertyLists whose properties should survive outside of the .txt file
    bool duplicate;
} UIPropertyList;

typedef struct {
    const char *name;
    u32 value;
} UIBitfieldEntry;

typedef struct {
    const char *name;
    float value;
} UIFloatEnumEntry;

typedef struct {
    const char *name;
    int value;
} UIIntEnumEntry;

UIPropertyList ui_create_proplist(size_t capacity, bool duplicate);
void ui_proplist_add(UIPropertyList *props, char* key, char* value);
void ui_destroy_proplist(UIPropertyList *props);

const char *ui_prop_string(const UIPropertyList *props, const char *key, const char *default_value);
int ui_prop_int(const UIPropertyList *props, const char *key, int default_value);
float ui_prop_float(const UIPropertyList *props, const char *key, float default_value);
bool ui_prop_bool(const UIPropertyList *props, const char *key, bool default_value);
u32 ui_prop_bitfield(const UIPropertyList *props, const char *key, const UIBitfieldEntry *table, size_t count);

int ui_prop_int_enum(const UIPropertyList *props, const char *key, const UIIntEnumEntry *table, size_t count, int default_value);
float ui_prop_float_enum(const UIPropertyList *props, const char *key, const UIFloatEnumEntry *table, size_t count, float default_value);

u32 ui_prop_color(const UIPropertyList *props, const char *key, u32 default_value);

UIPropertyList ui_prop_list(const UIPropertyList *props, const char *key);