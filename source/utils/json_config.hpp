#pragma once
#include <3ds.h>
#include "json-c/json.h"

#define CONFIG_PARENT "/3ds/"
#define CONFIG_ROOT CONFIG_PARENT "gd3ds/"
#define CONFIG_FILENAME "config.json"
#define CONFIG_FILE CONFIG_ROOT CONFIG_FILENAME
#define DATA_FOLDER CONFIG_ROOT "data/"

#define CONFIG_FLAGS "flags."
#define CONFIG_VALUES "values."
#define CONFIG_GRAPHICS_PATH "graphics."
#define CONFIG_INPUT_PATH "input."
#define CONFIG_CUSTOMIZATION_PATH "custom."
#define CONFIG_MISC_PATH "misc."
#define CONFIG_GAMEPLAY_PATH "gameplay."
#define CONFIG_COSMETIC_PATH "cosmetic."
#define CONFIG_FILTERS_PATH "filters."


#define CONFIG_OBJECT_DELIMITER "."

#define USER_LEVELS_NAME "external_levels"
#define USER_LEVELS_DIR CONFIG_ROOT USER_LEVELS_NAME
#define USER_SONGS_NAME "saved_songs/"
#define USER_SONGS_DIR CONFIG_ROOT USER_SONGS_NAME
#define SAVED_LEVELS_NAME "saved_levels"
#define SAVED_LEVELS_DIR CONFIG_ROOT SAVED_LEVELS_NAME

namespace gd3ds
{

struct Config {
    struct json_object* root;
    char path[256];
};

int configLoad(Config* cfg, const char* path);
void configSave(Config* cfg);
void configFree(Config* cfg);

const char* configGetString(Config* cfg, const char* path, const char* def);
int configGetInt(Config* cfg, const char* path, int def);
float configGetFloat(Config* cfg, const char* path, float def);
int configGetBool(Config* cfg, const char* path, int def);

void configSetString(Config* cfg, const char* path, const char* value);
void configSetInt(Config* cfg, const char* path, int value);
void configSetFloat(Config* cfg, const char* path, float value);
void configSetBool(Config* cfg, const char* path, int value);

void configInitString(Config* cfg, const char* path, const char* def);
void configInitBool(Config* cfg, const char* path, bool def);
void configInitInt(Config* cfg, const char* path, int def);
void configInitFloat(Config* cfg, const char* path, float def);

}
