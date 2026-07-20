#pragma once
#include <3ds.h>
#include <json-c/json.h>

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
#define USER_SONGS_NAME "saved_songs"
#define USER_SONGS_DIR CONFIG_ROOT USER_SONGS_NAME

typedef struct {
    struct json_object* root;
    char path[256];
} Config;

int config_load(Config* cfg, const char* path);
void config_save(Config* cfg);
void config_free(Config* cfg);

const char* config_get_string(Config* cfg, const char* path, const char* def);
int config_get_int(Config* cfg, const char* path, int def);
float config_get_float(Config* cfg, const char* path, float def);
int config_get_bool(Config* cfg, const char* path, int def);

void config_set_string(Config* cfg, const char* path, const char* value);
void config_set_int(Config* cfg, const char* path, int value);
void config_set_float(Config* cfg, const char* path, float value);
void config_set_bool(Config* cfg, const char* path, int value);

void config_init_string(Config* cfg, const char* path, char* def);
void config_init_bool(Config* cfg, const char* path, bool def);
void config_init_int(Config* cfg, const char* path, int def);
void config_init_float(Config* cfg, const char* path, float def);