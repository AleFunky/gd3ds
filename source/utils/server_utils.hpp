#pragma once
#include "network.hpp"

#include <3ds.h>

namespace gd3ds
{

typedef struct SearchEntry {
    char name[20+1];
    char *description;
    int levelId;
    int creatorId;
    int songId;
    size_t mainSongId;
    int lengthNum;
    int downloads;
    int likes;
    int stars;
    int coins;
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
    bool verifiedCoins;
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
    char songTitle[64];
    char artistName[32];
    float songSize;
    char songLink[256];
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
    char uploadDate[32];
    char updateDate[32];
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
    size_t col1;
    size_t col2;
    bool glow;
    size_t glowCol;
} CommentEntry;

typedef struct CommentAuthorEntry {
    
} CommentAuthorEntry;

typedef struct UpdateData {
    bool isAvailable;
    char latestCommit[41];
    char releaseTitle[64];
    char *releaseBody;
    char releaseDownloadUrl[128];
    int releaseFileSize;
} UpdateData;

int searchLevels(GenericTask *task);
int getLevel(GenericTask *task);
int getComments(GenericTask *task);
int getSongData(GenericTask *task);
int checkForUpdates(GenericTask *task);
int installUpdate(GenericTask *);
int extractUpdate(GenericTask *);

float deriveGJVersion(int version);

extern SearchEntry *searchEntries;
extern CreatorEntry *creatorEntries;
extern SongEntry *songEntries;
extern PageEntry *pageEntry;

extern LevelEntry *levelEntry;

extern CommentEntry *commentEntries;

extern SearchFilters filters;

extern UpdateData *updateData;

extern int creatorEntriesLength;
extern int songEntriesLength;
extern int searchEntriesLength;

extern int levelEntryLength;

extern int commentEntriesLength;

extern char **latestCommits;
extern int latestCommitCount;

}
