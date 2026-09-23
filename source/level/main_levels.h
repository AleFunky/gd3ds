#pragma once

#include <3ds.h>

enum MainLevelDifficulties {
    MAIN_DIFF_EASY,
    MAIN_DIFF_NORMAL,
    MAIN_DIFF_HARD,
    MAIN_DIFF_HARDER,
    MAIN_DIFF_INSANE,
    MAIN_DIFF_DEMON
};

typedef struct SongEntries {
    char *title;
    char *artist;
} SongEntries;

typedef struct {
    char *level_name;
    char *gmd_path;
    char *song_path;

    int difficulty;
    int stars;
    SongEntries song_data;
} MainLevelDefinition;

typedef struct {
    MainLevelDefinition *levels;
    size_t count;
} MainLevelPack;

extern const MainLevelPack *current_main_level_pack;

extern const MainLevelPack robtop_levels;
extern const MainLevelPack gdps_levels;