#pragma once
#include <3ds.h>
#include "menus/core/ui_stack.h"

typedef struct SongEntries {
    char *title;
    char *artist;
} SongEntries;

extern const SongEntries main_songs[19];

extern const UIScreenDefPair songs_def;