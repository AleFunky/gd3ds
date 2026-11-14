#pragma once

#include <3ds.h>

namespace gd3ds
{

enum class Difficulty {
    Easy,
    Normal,
    Hard,
    Harder,
    Insane,
    Demon
};

struct SongEntries {
    const char *title;
    const char *artist;
};

struct MainLevelDefinition {
    int levelID;
    
    const char *levelName;
    const char *gmdPath;
    const char *songPath;

    Difficulty difficulty;
    int stars;
    SongEntries songData;
};

struct MainLevelPack {
    MainLevelDefinition *levels;
    size_t count;
};

extern const MainLevelPack *currentMainLevelPack;

extern const MainLevelPack robtopLevels;
extern const MainLevelPack gdpsLevels;

}