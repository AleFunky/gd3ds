#pragma once
#include "3ds/thread.h"
#include "curl/system.h"
#include <stdbool.h>

#define ROBTOP_SEARCH_API "http://www.boomlings.com/database/getGJLevels21.php"
#define ROBTOP_LEVEL_API "http://www.boomlings.com/database/downloadGJLevel22.php"
#define ROBTOP_COMMENTS_API "http://www.boomlings.com/database/getGJComments21.php"
#define ROBTOP_SONGS_API "http://www.boomlings.com/database/getGJSongInfo.php"

#define GDPS_SEARCH_API "https://19gdps.com/gdapi/getGJLevels21.php"
#define GDPS_LEVEL_API "https://19gdps.com/gdapi/downloadGJLevel22.php"
#define GDPS_COMMENTS_API "https://19gdps.com/gdapi/getGJComments21.php"
#define GDPS_SONGS_API "https://19gdps.com/gdapi/getGJSongInfo.php"

#define TIMEOUT_DURATION 15

typedef struct SearchFilters {
    bool uncompleted:1;
    bool completed:1;
    bool original:1;
    bool noStar:1;
    bool star:1;
    bool featured:1;
    bool super:1;
    bool isNA:1;
    bool isAuto:1;
    bool isDemon:1;
    //used for demons if isDemon is true
    unsigned int difficultyFilters:6;
    unsigned int lengthFilters:5;
    bool songFilter:1;
    bool customSong:1;

    int searchType;
    int currentPage;
    size_t mainSong;
    char customSongQuery[15];
    char searchQuery[20];
} SearchFilters;

typedef struct GenericTask GenericTask;

typedef struct {
    volatile float progress;
    volatile int speed;

    volatile int result;
    
    volatile bool finished;
    volatile bool running;
    volatile bool cancelled;

    u64 lastTime;
    curl_off_t lastBytes;

    const char *path;
    const char *url;
    char fileName[16];
    char extension[8];
} DownloadTask;

Thread createFileDownloadThread(DownloadTask *task);

int socInit();

int getLevelFromID(GenericTask *task, char **outData, int id, bool useGdps);

int getSearchResults(GenericTask *task, char **outData, int gameVer, SearchFilters f, bool useGdps);

int getCommentsFromID(GenericTask *task, char **outData, int id, int page, int mode, bool useGdps);

int getSongInfoFromID(GenericTask *task, char **outData, int songId, bool useGdps);

int getCommits(GenericTask *task, char **outData, const char *repoOwner, const char *repoName, const char *repoBranch);

int getReleases(GenericTask *task, char **outData, const char *repoOwner, const char *repoName);

int getActions(GenericTask *task, char **outData, const char *repoOwner, const char *repoName, const char *actionName);

void shutdownSoc();
