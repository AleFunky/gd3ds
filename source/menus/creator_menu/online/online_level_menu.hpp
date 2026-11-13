#pragma once
#include "menus/core/ui_stack.hpp"
#include "menus/level_options.hpp"
#include "utils/server_utils.hpp"

void deleteLevel();
void checkWarningsAndPlay();

extern int onlineMenuLevelID;
extern SearchEntry *currentSearchEntry;
extern CreatorEntry *currentCreatorEntry;
extern SongEntry *currentSongEntry;

extern bool refresh;

extern const UIScreenDefPair onlineLevelMenuDef;