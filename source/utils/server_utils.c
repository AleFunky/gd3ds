#include <3ds.h>
#include <stdlib.h>
#include <citro2d.h>
#include <string.h>
#include "level_loading.h"
#include "menus/components/ui_window_button.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_rectangle.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"
#include "utils/folders.h"
#include "utils/server_utils.h"
#include "network.h"
#include "utils/string_helpers.h"

SearchEntry *search_entries = NULL;
CreatorEntry *creator_entries = NULL;
SongEntry *song_entries = NULL;

LevelEntry *level_entry = NULL;

int searchEntriesLength = 0;
int creatorEntriesLength = 0;
int songEntriesLength = 0;

int levelEntryLength = 0;

static void fill_creator_entries(char **creatorStrings, int creatorStringCount) {
    for (int i = 0; i < creatorStringCount; i++) {
        int stringCount;
        char **creatorString = split_string(creatorStrings[i], ':', &stringCount, true);
        creator_entries[i].accountId = atoi(creatorString[0]);
        strncpy(creator_entries[i].creatorName, creatorString[1], sizeof(creator_entries[i].creatorName) - 1);
        creator_entries[i].userId = atoi(creatorString[2]);
        free_string_array(creatorString, stringCount);
    }
}

static void fill_song_entries(char **songStrings, int songStringCount) {
    for (int i = 0; i < songStringCount; i++) {
        int songKeyCount = 0;

        char **songKeys = split_string_str_del(songStrings[i], "~|~", &songKeyCount, true);

        for (int j = 0; j + 1 < songKeyCount; j += 2) {
            int key = atoi(songKeys[j]);
            char *valStr = songKeys[j + 1];
            switch (key) {
                case 1:
                    // song id
                    song_entries[i].ngSongId = atoi(valStr);
                    break;
                case 2:
                    // song name
                    strncpy(song_entries[i].songTitle, valStr, sizeof(song_entries[i].songTitle) - 1);
                    break;
                case 4:
                    // artist name
                    strncpy(song_entries[i].artistName, valStr, sizeof(song_entries[i].artistName) - 1);
                    break;
                case 5:
                    // song size
                    song_entries[i].songSize = atoi(valStr);
                    break;
                case 10:
                    // song link
                    strncpy(song_entries[i].songLink, valStr, sizeof(song_entries[i].songLink) - 1);
                    break;
            }
        }
        free_string_array(songKeys, songKeyCount);
    }
}

static void fill_level_entries(char **levelsStrings, int songStringCount, int creatorStringCount, int levelStringCount) {
    for (int i = 0; i < levelStringCount; i++) {
        int levelKeyCount = 0;
        char **levelKeys = split_string(levelsStrings[i], ':', &levelKeyCount, true);

        for (int j = 0; j + 1 < levelKeyCount; j += 2) {
            int key = atoi(levelKeys[j]);
            char *valStr = levelKeys[j + 1];

            switch (key) {
                case 1:
                    // level id
                    search_entries[i].levelId = atoi(valStr);
                    break;
                case 2:
                    // level name
                    strncpy(search_entries[i].name, valStr, sizeof(search_entries[i].name) - 1);
                    break;
                case 3:
                    // level description
                    if (valStr[0] == '\0') {
                        search_entries[i].description = strdup("No description provided.");
                        break;
                    }

                    fix_base64_url(valStr);
                    search_entries[i].description = malloc(strlen(valStr) + 1);
                    int decoded_len = base64_decode(valStr, (unsigned char *)search_entries[i].description);
                    if (decoded_len > 0) {
                        search_entries[i].description[decoded_len] = '\0';
                    }
                    break;
                case 5:
                    // level version
                    search_entries[i].levelVersion = atoi(valStr);
                    break;
                case 6:
                    // creator player id
                    search_entries[i].creatorId = atoi(valStr);
                    break;
                case 9:
                    // level difficulty
                    search_entries[i].difficulty = atoi(valStr) / 10;
                    break;
                case 10:
                    // level downloads
                    search_entries[i].downloads = atoi(valStr);
                    break;
                case 12:
                    // main level song, 0 if custom song is present
                    search_entries[i].mainSongId = atoi(valStr);
                    break;
                case 13:
                    // game version the level was uploaded in 
                    search_entries[i].gameVersion = atoi (valStr);
                    break;
                case 14:
                    // level likes, formula is likes - dislikes
                    search_entries[i].likes = atoi(valStr);
                    break;
                case 15:
                    // level length
                    search_entries[i].lengthNum = atoi(valStr);
                    break;
                case 16:
                    // level dislikes, we have no use for this (formula is dislikes - likes)
                    break;
                case 17:
                    // demon status
                    search_entries[i].isDemon = parse_bool(valStr);
                    break;
                case 18: 
                    // stars
                    search_entries[i].stars = atoi(valStr);
                    break;
                case 19: 
                    // feature score
                    search_entries[i].featureScore = atoi(valStr);
                    break;
                case 25:
                    // auto status
                    search_entries[i].isAuto = parse_bool(valStr);
                    break;
                case 30:
                    // if level is a copy, id of original level
                    search_entries[i].originalId = atoi(valStr);
                    break;
                case 31:
                    // two player status
                    search_entries[i].isTwoPlayer = parse_bool(valStr);
                    break;
                case 35: 
                    // newgrounds song id
                    search_entries[i].songId = atoi(valStr);
                    break;
                case 39: 
                    // stars requested
                    search_entries[i].reqStars = atoi(valStr);
                    break;
                case 45:
                    // object count, caps at 65535
                    search_entries[i].objCount = atoi(valStr);
                    break;
            }
        }
        free_string_array(levelKeys, levelKeyCount);

        int song_index;
        int creator_index;
        
        // Find the song
        for (song_index = 0; song_index < songStringCount; song_index++) {
            if (search_entries[i].songId == song_entries[song_index].ngSongId) {
                break;
            }
        }

        // Find the creator
        for (creator_index = 0; creator_index < creatorStringCount; creator_index++) {
            if (search_entries[i].creatorId == creator_entries[creator_index].accountId) {
                break;
            }
        }

        search_entries[i].creatorIndex = creator_index;
        search_entries[i].songIndex = song_index;
    }
}

static void fill_level_entry(char **levelStrings, int levelStringsCount) {
    int levelKeyCount = 0;

    char **levelKeys = split_string(levelStrings[0], ':', &levelKeyCount, true);
    for (int j = 0; j + 1 < levelKeyCount; j += 2) {
        int key = atoi(levelKeys[j]);
        char *valStr = levelKeys[j + 1];;
        switch (key) {
            case 1:
                // level id
                level_entry->levelId = atoi(valStr);
                break;
            case 4:
                // base64 encoded probably compressed level string
                level_entry->levelString = strdup(valStr);
                break;
            case 28:
                // time since upload
                strncpy(level_entry->uploadDate, valStr, sizeof(level_entry->uploadDate) - 1);
                break;
            case 29:
                // time since last
                strncpy(level_entry->updateDate, valStr, sizeof(level_entry->updateDate) - 1);
                break;
        }
    }
    free_string_array(levelKeys, levelKeyCount);
}

int search_levels(char *query, int type, int page) {
    char *outdata;
    int result = get_search_results(&outdata, query, type, page);

    if (result != 0) return result;

    int initialStringCount = 0;

    int levelStringCount = 0;
    int creatorStringCount = 0;
    int songStringCount = 0;

    char **initialStrings = split_string(outdata, '#', &initialStringCount, true);
    if (!initialStrings) return -1;

    char **levelsStrings = split_string(initialStrings[0], '|', &levelStringCount, true);
    if (!levelsStrings) return -1;

    char **creatorStrings = split_string(initialStrings[1], '|', &creatorStringCount, true);
    if (!creatorStrings) return -1;
    
    char **songStrings = split_string_str_del(initialStrings[2], "~:~", &songStringCount, true);
    if (!songStrings) return -1;
    
    search_entries = malloc(levelStringCount * sizeof(SearchEntry));
    if (!search_entries) return -1;

    creator_entries = malloc(creatorStringCount * sizeof(CreatorEntry));
    if (!creator_entries) return -1;

    song_entries = malloc(songStringCount * sizeof(SongEntry));
    if (!song_entries) return -1;

    // Initialize
    memset(search_entries, 0, levelStringCount * sizeof(SearchEntry));
    memset(creator_entries, 0, creatorStringCount * sizeof(CreatorEntry));
    memset(song_entries, 0, songStringCount * sizeof(SongEntry));

    // Fill creators
    fill_creator_entries(creatorStrings, creatorStringCount);

    // Fill songs
    fill_song_entries(songStrings, songStringCount);
    
    // Fill levels
    fill_level_entries(levelsStrings, songStringCount, creatorStringCount, levelStringCount);
   
    free_string_array(levelsStrings, levelStringCount);
    free_string_array(creatorStrings, creatorStringCount);
    free_string_array(songStrings, songStringCount);    
    free_string_array(initialStrings, initialStringCount);

    creatorEntriesLength = creatorStringCount;
    songEntriesLength = songStringCount;
    searchEntriesLength = levelStringCount;
    return 0;
}

int get_level_data(int id) {
    char *outdata;
    int result = get_level_from_id(&outdata, id);

    if (result != 0) return result;

    int initialStringCount = 0;

    char **initialStrings = split_string(outdata, '#', &initialStringCount, true);
    if (!initialStrings) return -1;

    level_entry = malloc(initialStringCount * sizeof(LevelEntry));
    if (!level_entry) return -1;

    // Initialize
    memset(level_entry, 0, initialStringCount * sizeof(LevelEntry));

    fill_level_entry(initialStrings, 1);

    free_string_array(initialStrings, initialStringCount);

    levelEntryLength = initialStringCount;
    return 0;
}

float derive_gj_version(int version) {
    switch (version) {
        case 1:
            return 1.0;
        case 2:
            return 1.1;
        case 3:
            return 1.2;
        case 4:
            return 1.3;
        case 5:
            return 1.4;
        case 6:
            return 1.5;
        case 7:
            return 1.6;
        case 10:
            return 1.7;
        case 18:
            return 1.8;
        case 19:
            return 1.9;
        case 20:
            return 2.0;
        case 21:
            return 2.1;
        case 22:
            return 2.2;
    }
    return 0;
}