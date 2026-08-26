#pragma once
#include "3ds/thread.h"
#include "curl/system.h"
#include <stdbool.h>

typedef struct SearchFilters {
    bool uncompleted:1;
    bool completed:1;
    bool original:1;
    bool noStar:1;
    bool star:1;
    bool featured:1;
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

typedef int (*NetworkFunc)(void);

typedef struct {
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

int get_level_from_id(char **out_data, int id);

int get_search_results(char **out_data, int gameVer, SearchFilters f);

int get_comments_from_id(char **out_data, int id, int page, int mode);


void soc_exit();
