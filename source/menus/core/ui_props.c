#include "ui_props.h"
#include <stdlib.h>
#include "main.h"
#include "text.h"
#include "utils/string_helpers.h"
#include "ui_actions.h"

static char **split_string_nobrackets(const char *str, char delimiter, size_t *outCount) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    int inside_brackets = 0;

    while (*ptr) {
        if (*ptr == '[') {
            inside_brackets++;
        } else if(*ptr == ']'){
            inside_brackets--;
        }

        if(inside_brackets < 0){
            inside_brackets = 0;
        }

        if(!inside_brackets){
            if (*ptr == delimiter) {
                int len = ptr - start;
                if (len >= 0) {
                    char *token = malloc(len + 1);
                    strncpy(token, start, len);
                    token[len] = '\0';

                    result = (char **)realloc(result, sizeof(char*) * (count + 1));
                    result[count++] = token;
                }
                start = ptr + 1;
            }
        }
        ptr++;
    }
    if (ptr > start) {
        int len = ptr - start;
        if (len >= 0) {
            char *token = malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            result = realloc(result, sizeof(char*) * (count + 1));
            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

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
    if(!props) return default_value ? default_value : "null";

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

UIPropertyList ui_parse_prop_list(char *value){
    char *copy = strdup(value);

    char* cursor = copy;
    char* token = NULL;
    size_t count = 0;

    while ((token = next_token(&cursor)) != NULL)
        count++;

    free(copy);

    UIPropertyList property_list = ui_create_proplist(count, true);

    cursor = value;
    collect_properties(&property_list, token, &cursor);

    return property_list;
}

UIPropertyList ui_prop_list(const UIPropertyList *props, const char *key){
    char *value = (char *) ui_prop_string(props, key, NULL);

    if(!value) return (UIPropertyList){ 0 };

    return ui_parse_prop_list(value);
}

UIAction *parse_actions(char *value, const UIActionDef *action_defs, const size_t actions_count, size_t *out_entry_count) {
    char **entries = split_string_nobrackets(value, ',', out_entry_count);

    if(!entries || *out_entry_count == 0){
        return NULL;
    }

    UIAction *actions = calloc(*out_entry_count, sizeof(UIAction));

    if(!actions){
        free_string_array(entries, *out_entry_count);
        *out_entry_count = 0;
        return NULL;
    }

    for(size_t i = 0; i < *out_entry_count; i++){
        char *entry = entries[i];

        if(!entry || !*entry) continue;

        actions[i].args = (UIPropertyList){ 0 };
        //remove brackets
        char *bracket = strchr(entry, '[');
        char *props_start = NULL;
        char *props_end = NULL;
        int in_brackets = 0;

        while(bracket && *bracket){
            if(*bracket == '['){
                if(!in_brackets){
                    props_start = bracket;
                }
                in_brackets++;
            }
            if(*bracket == ']'){
                in_brackets--;
                if(!in_brackets){
                    props_end = bracket;
                }
            }

            bracket++;
        }

        if (props_start && props_end) {
            *props_start = '\0';
            *props_end = '\0';
            actions[i].args = ui_parse_prop_list(++props_start);
        }

        actions[i].action = ui_find_action(base_actions, BASE_ACTION_COUNT, entry);

        //if not found in base actions, search in provided actions
        if(!actions[i].action){
            actions[i].action = ui_find_action(action_defs, actions_count, entry);
        }
    }

    free_string_array(entries, *out_entry_count);

    return actions;
}

UIAction *ui_prop_actions(const UIPropertyList *props, const UIActionDef *action_defs, const size_t actions_count, const char *key, size_t *out_entry_count) {
    char *value = (char *) ui_prop_string(props, key, NULL);

    if(!value) return NULL;

    UIAction *actions = parse_actions(value, action_defs, actions_count, out_entry_count);

    if(!actions) return NULL;
    
    return actions;
}