#include "ui_props.h"
#include <stdlib.h>
#include "main.h"
#include "menus/core/ui_screen.h"
#include "text.h"
#include "utils/string_helpers.h"

UIPropertyList ui_create_proplist(size_t capacity, bool duplicate) {
    UIPropertyList props;
    props.properties = malloc(sizeof(UIProperty) * capacity);
    props.count = 0;
    props.duplicate = duplicate;
    return props;
}

void ui_proplist_add(UIPropertyList *props, char* key, char* value){
    if(props->duplicate){
        key = strdup(key);
        value = strdup(value);
    }

    props->properties[props->count++] = (UIProperty){
        .key = key,
        .value = value
    };
}

void ui_destroy_proplist(UIPropertyList *props) {
    //if the UIPropertyList's properties are duplicated, they must be freed explicitly
    if(props->duplicate){
        for(int i = 0; i < props->count; i++) {
            free((char *) props->properties[i].key);
            free((char *) props->properties[i].value);
        }
    }

    free(props->properties);

    props->properties = NULL;
    props->count = 0;
}

const char *ui_prop_string(const UIPropertyList *props, const char *key, const char *default_value) {
    for (int i = 0; i < props->count; i++) {
        if (strcmp(props->properties[i].key, key) == 0)
            return props->properties[i].value;
    }

    return default_value;
}

int ui_prop_int(const UIPropertyList *props, const char *key, int default_value) {
    const char *prop = ui_prop_string(props, key, NULL);
    return prop ? atoi(prop) : default_value;
}

float ui_prop_float(const UIPropertyList *props, const char *key, float default_value) {
    const char *prop = ui_prop_string(props, key, NULL);
    return prop ? atoff(prop) : default_value;
}

bool ui_prop_bool(const UIPropertyList *props, const char *key, bool default_value) {
    const char *prop = ui_prop_string(props, key, NULL);
    return prop ? parse_bool(prop) : default_value;
}

u32 ui_prop_bitfield(const UIPropertyList *props, const char *key, const UIBitfieldEntry *table, size_t count) {
    const char *prop = ui_prop_string(props, key, NULL);
    if (!prop) return 0;

    char buffer[128];
    strncpy(buffer, prop, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    u32 result = 0;

    for (char *entry = strtok(buffer, ","); entry; entry = strtok(NULL, ",")) {
        // Skip spaces
        while (*entry == ' ') entry++;

        // Find in the table
        for (size_t i = 0; i < count; i++) {
            if (strcmp(entry, table[i].name) == 0) {
                result |= table[i].value;
                break;
            }
        }
    }

    return result;
}

int ui_prop_int_enum(const UIPropertyList *props, const char *key, const UIIntEnumEntry *table, size_t count, int default_value) {
    const char *value = ui_prop_string(props, key, NULL);
    if (!value) return default_value;

    for (size_t i = 0; i < count; i++) {
        if (strcmp(value, table[i].name) == 0)
            return table[i].value;
    }

    return default_value;
}

float ui_prop_float_enum(const UIPropertyList *props, const char *key, const UIFloatEnumEntry *table, size_t count, float default_value) {
    const char *value = ui_prop_string(props, key, NULL);
    if (!value) return default_value;

    for (size_t i = 0; i < count; i++) {
        if (strcmp(value, table[i].name) == 0)
            return table[i].value;
    }

    return default_value;
}

u32 ui_prop_color(const UIPropertyList *props, const char *key, u32 default_value) {
    const char *value = ui_prop_string(props, key, NULL);
    if (!value) return default_value;

    u32 color;

    bool parsed = parse_hex_color(value, &color);

    return parsed ? color : default_value;
}

UIPropertyList ui_prop_list(const UIPropertyList *props, const char *key){
    char *value = (char *) ui_prop_string(props, key, NULL);

    if(!value) return (UIPropertyList){ 0 };

    char *copy = strdup(value);

    char* cursor = copy;
    char* token = NULL;
    size_t count = 0;

    while ((token = next_token(&cursor)) != NULL)
        count++;

    free(copy);

    UIPropertyList property_list = ui_create_proplist(count, true);

    cursor = value;
    collect_properties(&property_list, token, &cursor, true);

    return property_list;
}