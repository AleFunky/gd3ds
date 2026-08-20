#pragma once

typedef struct SearchEntry {
    char name[20+1];
    char *description;
    int levelId;
    int creatorId;
    int songId;
    int mainSongId;
    int lengthNum;
    int downloads;
    int likes;
    int stars;
    int reqStars;
    int difficulty;
    int objCount;
    int levelVersion;
    int gameVersion;
    int featureScore;
    int originalId;
    bool isTwoPlayer;
    bool isDemon;
    bool isAuto;
    int creatorIndex;
    int songIndex;
} SearchEntry;

typedef struct CreatorEntry {
    int userId;
    char creatorName[20+1];
    int accountId;
} CreatorEntry;

typedef struct SongEntry {
    int ngSongId;
    char songTitle[128];
    char artistName[128];
    int songSize;
    char songLink[128];
} SongEntry;

typedef struct LevelEntry {
    int levelId;
    char *levelString;
    char uploadDate[128];
    char updateDate[128];
} LevelEntry;

int search_levels(char *query, int type, int page);

int get_level_data(int id);

float derive_gj_version(int version);

extern SearchEntry *search_entries;
extern CreatorEntry *creator_entries;
extern SongEntry *song_entries;

extern LevelEntry *level_entry;

extern int creatorEntriesLength;
extern int songEntriesLength;
extern int searchEntriesLength;

extern int levelEntryLength;