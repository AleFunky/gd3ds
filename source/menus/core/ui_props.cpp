#include "ui_props.hpp"
#include <stdlib.h>
#include "main.hpp"
#include "text.hpp"
#include "utils/string_helpers.hpp"
#include "ui_actions.hpp"

namespace gd3ds
{

static char **splitStringNobrackets(const char *str, char delimiter, size_t *outCount) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    int insideBrackets = 0;

    while (*ptr) {
        if (*ptr == '[') {
            insideBrackets++;
        } else if(*ptr == ']'){
            insideBrackets--;
        }

        if(insideBrackets < 0){
            insideBrackets = 0;
        }

        if(!insideBrackets){
            if (*ptr == delimiter) {
                int len = ptr - start;
                if (len >= 0) {
                    char *token = (char *)malloc(len + 1);
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
            char *token = (char *)malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            result = (char **)realloc(result, sizeof(char*) * (count + 1));
            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

UIPropertyList uiCreateProplist(size_t capacity, bool duplicate) {
    UIPropertyList props;
    props.properties = (UIProperty *)malloc(sizeof(UIProperty) * capacity);
    props.count = 0;
    props.duplicate = duplicate;
    return props;
}

void uiProplistAdd(UIPropertyList *props, char* key, char* value){
    if(props->duplicate){
        key = strdup(key);
        value = strdup(value);
    }

    props->properties[props->count++] = (UIProperty){
        .key = key,
        .value = value
    };
}

void uiDestroyProplist(UIPropertyList *props) {
    //if the UIPropertyList's properties are duplicated, they must be freed explicitly
    if(props->duplicate){
        for(size_t i = 0; i < props->count; i++) {
            free((char *) props->properties[i].key);
            free((char *) props->properties[i].value);
        }
    }

    free(props->properties);

    props->properties = NULL;
    props->count = 0;
}

const char *uiPropString(const UIPropertyList *props, const char *key, const char *defaultValue) {
    if(!props) return defaultValue ? defaultValue : "null";

    for (size_t i = 0; i < props->count; i++) {
        if (strcmp(props->properties[i].key, key) == 0)
            return props->properties[i].value;
    }

    return defaultValue;
}

int uiPropInt(const UIPropertyList *props, const char *key, int defaultValue) {
    const char *prop = uiPropString(props, key, NULL);
    return prop ? atoi(prop) : defaultValue;
}

float uiPropFloat(const UIPropertyList *props, const char *key, float defaultValue) {
    const char *prop = uiPropString(props, key, NULL);
    return prop ? atoff(prop) : defaultValue;
}

bool uiPropBool(const UIPropertyList *props, const char *key, bool defaultValue) {
    const char *prop = uiPropString(props, key, NULL);
    return prop ? parseBool(prop) : defaultValue;
}

u32 uiPropBitfield(const UIPropertyList *props, const char *key, const UIBitfieldEntry *table, size_t count) {
    const char *prop = uiPropString(props, key, NULL);
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

int uiPropIntEnum(const UIPropertyList *props, const char *key, const UIIntEnumEntry *table, size_t count, int defaultValue) {
    const char *value = uiPropString(props, key, NULL);
    if (!value) return defaultValue;

    for (size_t i = 0; i < count; i++) {
        if (strcmp(value, table[i].name) == 0)
            return table[i].value;
    }

    return defaultValue;
}

float uiPropFloatEnum(const UIPropertyList *props, const char *key, const UIFloatEnumEntry *table, size_t count, float defaultValue) {
    const char *value = uiPropString(props, key, NULL);
    if (!value) return defaultValue;

    for (size_t i = 0; i < count; i++) {
        if (strcmp(value, table[i].name) == 0)
            return table[i].value;
    }

    return defaultValue;
}

u32 uiPropColor(const UIPropertyList *props, const char *key, u32 defaultValue) {
    const char *value = uiPropString(props, key, NULL);
    if (!value) return defaultValue;

    u32 color;

    bool parsed = parseHexColor(value, &color);

    return parsed ? color : defaultValue;
}

UIPropertyList uiParsePropList(char *value){
    char *copy = strdup(value);

    char* cursor = copy;
    char* token = NULL;
    size_t count = 0;

    while ((token = nextToken(&cursor)) != NULL)
        count++;

    free(copy);

    UIPropertyList propertyList = uiCreateProplist(count, true);

    cursor = value;
    collectProperties(&propertyList, token, &cursor);

    return propertyList;
}

UIPropertyList uiPropList(const UIPropertyList *props, const char *key){
    char *value = (char *) uiPropString(props, key, NULL);

    if(!value) return (UIPropertyList){ 0 };

    return uiParsePropList(value);
}

UIAction *parseActions(const char *value, const UIActionDef *actionDefs, const size_t actionsCount, size_t *outEntryCount) {
    char **entries = splitStringNobrackets(value, ',', outEntryCount);

    if(!entries || *outEntryCount == 0){
        return NULL;
    }

    UIAction *actions = (UIAction *)calloc(*outEntryCount, sizeof(UIAction));

    if(!actions){
        freeStringArray(entries, *outEntryCount);
        *outEntryCount = 0;
        return NULL;
    }

    for(size_t i = 0; i < *outEntryCount; i++){
        char *entry = entries[i];

        if(!entry || !*entry) continue;

        actions[i].args = (UIPropertyList){ 0 };
        //remove brackets
        char *bracket = strchr(entry, '[');
        char *props_start = NULL;
        char *props_end = NULL;
        int inBrackets = 0;

        while(bracket && *bracket){
            if(*bracket == '['){
                if(!inBrackets){
                    props_start = bracket;
                }
                inBrackets++;
            }
            if(*bracket == ']'){
                inBrackets--;
                if(!inBrackets){
                    props_end = bracket;
                }
            }

            bracket++;
        }

        if (props_start && props_end) {
            *props_start = '\0';
            *props_end = '\0';
            actions[i].args = uiParsePropList(++props_start);
        }

        actions[i].action = uiFindAction(baseActions, BASE_ACTION_COUNT, entry);

        //if not found in base actions, search in provided actions
        if(!actions[i].action){
            actions[i].action = uiFindAction(actionDefs, actionsCount, entry);
        }
    }

    freeStringArray(entries, *outEntryCount);

    return actions;
}

UIAction *uiPropActions(const UIPropertyList *props, const UIActionDef *actionDefs, const size_t actionsCount, const char *key, size_t *outEntryCount) {
    char *value = (char *) uiPropString(props, key, NULL);

    if(!value) return NULL;

    UIAction *actions = parseActions(value, actionDefs, actionsCount, outEntryCount);

    if(!actions) return NULL;
    
    return actions;
}

}