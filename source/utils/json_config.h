#pragma once
#include <3ds.h>
#include <json-c/json.h>

#define CONFIG_PARENT "/3ds/"
#define CONFIG_ROOT CONFIG_PARENT "gd3ds/"
#define CONFIG_FILENAME "config.json"
#define CONFIG_FILE CONFIG_ROOT CONFIG_FILENAME

#define CONFIG_FLAGS "flags."
#define CONFIG_GRAPHICS_PATH "graphics."
#define CONFIG_INPUT_PATH "input."
#define CONFIG_CUSTOMIZATION_PATH "custom."

#define CONFIG_OBJECT_DELIMITER "."

#define USER_LEVELS_NAME "external_levels"
#define USER_LEVELS_DIR CONFIG_ROOT USER_LEVELS_NAME
#define USER_SONGS_NAME "saved_songs"
#define USER_SONGS_DIR CONFIG_ROOT USER_SONGS_NAME

typedef struct {
    struct json_object* root;
    const char* path;
} Config;

int config_load(Config* cfg, const char* path);
void config_save(Config* cfg);
void config_free(Config* cfg);

const char* config_get_string(Config* cfg, const char* path, const char* def);
int config_get_int(Config* cfg, const char* path, int def);
int config_get_bool(Config* cfg, const char* path, int def);

void config_set_string(Config* cfg, const char* path, const char* value);
void config_set_int(Config* cfg, const char* path, int value);
void config_set_bool(Config* cfg, const char* path, int value);

void config_init_bool(Config* cfg, const char* path, bool def);
void config_init_int(Config* cfg, const char* path, int def);