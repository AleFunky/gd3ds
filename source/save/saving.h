#pragma once

#include <stdbool.h>
#include "level/main_levels.h"

#define DATA_ATTEMPTS "attempts"
#define DATA_JUMPS "jumps"
#define DATA_NORMAL "normal"
#define DATA_PRACTICE "practice"
#define DATA_COIN1 "coin1"
#define DATA_COIN2 "coin2"
#define DATA_COIN3 "coin3"
#define DATA_STARS "stars"

#define SAVE_ROBTOP_SERVER_FILE (CONFIG_ROOT "gdservers.dat")
#define SAVE_1P9_SERVER_FILE (CONFIG_ROOT "1p9gdps.dat")
#define SAVE_EXTERNAL_LEVELS_FILE (CONFIG_ROOT "external.dat")

#define SAVE_ONLINE_KEY "online"
#define SAVE_MAIN_LEVEL_KEY "main_levels"
#define SAVE_EXTERNAL_KEY "external_levels"

typedef struct LevelData {
    int level_id;
    int attempts;
    int jumps;
    int normal_progress;
    int practice_progress;
    int stars;
    bool coin1;
    bool coin2;
    bool coin3;
} LevelData;

typedef struct LevelDataEntry {
    char *key;
    LevelData data;
} LevelDataEntry;

typedef struct LevelDataList {
    LevelDataEntry *list;
    size_t capacity;
    size_t count;
} LevelDataList;

typedef struct ServerFile {
    LevelDataList online_levels;
    LevelDataList main_levels;
} ServerFile;

typedef struct ExternalLevelFile {
    LevelDataList external_levels;  
} ExternalLevelFile;

typedef enum {
    LEVEL_LIST_MAIN_LEVELS,
    LEVEL_LIST_EXTERNAL,
    LEVEL_LIST_ONLINE,
} LevelListType;

extern int total_stars;
extern int total_coins;
extern int total_attempts;
extern int total_jumps;
extern int total_demons;
extern int completed_main_levels;
extern int completed_external_levels;
extern int players_destroyed;

bool load_external_file(const char *path, ExternalLevelFile *save_data);
bool save_external_file(const char *path, const ExternalLevelFile *save_data);

bool load_save_file(const char *path, ServerFile *save_data);
bool save_save_file(const char *path, const ServerFile *save_data);

LevelDataEntry *get_or_add_level_to_external_file(ExternalLevelFile *save_data, const char *key);
LevelDataEntry *get_or_add_level_to_server_file(ServerFile *save_data, const char *key, LevelListType type);

extern LevelDataEntry *current_level_entry;

bool migrate_old_data();

void save_current_save_file(LevelListType type);

void calculate_stats();