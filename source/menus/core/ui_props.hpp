#pragma once
#include <3ds/types.h>

namespace gd3ds
{

struct UIAction;
struct UIActionDef;

struct UIProperty {
    const char *key;
    const char *value;
};

struct UIPropertyList {
    UIProperty *properties;
    size_t count;
    //This is used for UIPropertyLists whose properties should survive outside of the .txt file
    bool duplicate;
};

struct UIBitfieldEntry {
    const char *name;
    u32 value;
};

struct UIFloatEnumEntry {
    const char *name;
    float value;
};

struct UIIntEnumEntry {
    const char *name;
    int value;
};

UIPropertyList uiCreateProplist(size_t capacity, bool duplicate);
void uiProplistAdd(UIPropertyList *props, char* key, char* value);
void uiDestroyProplist(UIPropertyList *props);

const char *uiPropString(const UIPropertyList *props, const char *key, const char *defaultValue);
int uiPropInt(const UIPropertyList *props, const char *key, int defaultValue);
float uiPropFloat(const UIPropertyList *props, const char *key, float defaultValue);
bool uiPropBool(const UIPropertyList *props, const char *key, bool defaultValue);
u32 uiPropBitfield(const UIPropertyList *props, const char *key, const UIBitfieldEntry *table, size_t count);

int uiPropIntEnum(const UIPropertyList *props, const char *key, const UIIntEnumEntry *table, size_t count, int defaultValue);
float uiPropFloatEnum(const UIPropertyList *props, const char *key, const UIFloatEnumEntry *table, size_t count, float defaultValue);

u32 uiPropColor(const UIPropertyList *props, const char *key, u32 defaultValue);

UIPropertyList uiPropList(const UIPropertyList *props, const char *key);

UIAction *parseActions(const char *value, const UIActionDef *actionDefs, const size_t actionsCount, size_t *outEntryCount);
UIAction *uiPropActions(const UIPropertyList *props, const UIActionDef *actionDefs, const size_t actionsCount, const char *key, size_t *outEntryCount);

}