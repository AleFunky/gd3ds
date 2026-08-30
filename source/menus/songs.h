#pragma once
#include <3ds.h>

typedef struct SongEntries {
    char *title;
    char *artist;
} SongEntries;

extern const SongEntries main_songs[19];

void songs_init();
int songs_loop();
