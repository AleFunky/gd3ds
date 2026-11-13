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
    const char *title;
    const char *artist;
} SongEntries;

typedef struct {
    int levelID;
    
    const char *levelName;
    const char *gmdPath;
    const char *songPath;

    int difficulty;
    int stars;
    SongEntries songData;
} MainLevelDefinition;

typedef struct {
    MainLevelDefinition *levels;
    size_t count;
} MainLevelPack;

extern const MainLevelPack *currentMainLevelPack;

extern const MainLevelPack robtopLevels;
extern const MainLevelPack gdpsLevels;