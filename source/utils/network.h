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
    int mainSong;
    char customSongQuery[15];
    char searchQuery[20];
} SearchFilters;

typedef struct NetworkTask NetworkTask;

typedef int (*NetworkFunc)(NetworkTask *);

typedef struct NetworkTask {
    NetworkFunc func;

    volatile int result;
    
    volatile bool running;
    volatile bool finished;
    volatile bool cancelled;
} NetworkTask;

typedef struct {
    volatile float progress;
    volatile int speed;

    volatile int result;
    
    volatile bool finished;
    volatile bool running;
    volatile bool cancelled;

    u64 last_time;
    curl_off_t last_bytes;

    char *path;
    char *url;
    char *song_id;
} DownloadTask;

Thread create_download_song_thread(DownloadTask *task);
Thread create_network_thread(NetworkTask *task);

int soc_init();

int get_level_from_id(NetworkTask *task, char **out_data, int id, bool useGdps);

int get_search_results(NetworkTask *task, char **out_data, int gameVer, SearchFilters f, bool useGdps);

int get_comments_from_id(NetworkTask *task, char **out_data, int id, int page, int mode, bool useGdps);

int get_song_info_from_id(NetworkTask *task, char **out_data, int songId, bool useGdps);

void soc_exit();
