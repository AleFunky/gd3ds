#pragma once
#include "network.h"

#include <3ds.h>

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
    int epic;
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
    float songSize;
    char songLink[512];
} SongEntry;

typedef struct PageEntry {
    int totalLevels;
    int currentOffset;
    int amount;
    int totalPages;
} PageEntry;

typedef struct LevelEntry {
    int levelId;
    char *levelString;
    char uploadDate[128];
    char updateDate[128];
} LevelEntry;

typedef struct CommentEntry {
    int levelId;
    char name[24];
    int authorAccountId;
    int authorPlayerId;
    char *content;
    int likes;
    int percent;
    bool isSpam;
    char commentAge[128];
    int modBadge;
    char modCommentColor[16];
    int playerIcon;
    int iconType;
    int col1;
    int col2;
    bool glow;
    int glowCol;
} CommentEntry;

typedef struct CommentAuthorEntry {
    
} CommentAuthorEntry;

int search_levels(NetworkTask *task);
int get_level(NetworkTask *task);
int get_comments(NetworkTask *task);
int get_song_data(NetworkTask *task);

float derive_gj_version(int version);

extern SearchEntry *search_entries;
extern CreatorEntry *creator_entries;
extern SongEntry *song_entries;
extern PageEntry *page_entry;

extern LevelEntry *level_entry;

extern CommentEntry *comment_entries;

extern SearchFilters filters;

extern int creatorEntriesLength;
extern int songEntriesLength;
extern int searchEntriesLength;

extern int levelEntryLength;

extern int commentEntriesLength;