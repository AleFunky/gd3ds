#pragma once

#include <3ds.h>

#define MAIN_LEVELS_NUM 23

enum MainLevelDifficulties {
    MAIN_DIFF_EASY,
    MAIN_DIFF_NORMAL,
    MAIN_DIFF_HARD,
    MAIN_DIFF_HARDER,
    MAIN_DIFF_INSANE,
    MAIN_DIFF_DEMON
};

typedef struct {
    char *level_name;
    char *gmd_path;
    char *song_path;

    int difficulty;
    int stars;
} MainLevelDefinition;

extern MainLevelDefinition main_levels[];