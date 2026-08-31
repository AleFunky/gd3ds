#include <3ds.h>
#include <stdlib.h>
#include <ctype.h>
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
#include "menus/search_menu.h"
#include "menus/online_menu.h"
#include "menus/online_level_menu.h"
#include "menus/online_level_comments.h"

SearchEntry *search_entries = NULL;
CreatorEntry *creator_entries = NULL;
SongEntry *song_entries = NULL;
PageEntry *page_entry = NULL;

LevelEntry *level_entry = NULL;

CommentEntry *comment_entries = NULL;

SearchFilters filters = { 0 };

int searchEntriesLength = 0;
int creatorEntriesLength = 0;
int songEntriesLength = 0;

int levelEntryLength = 0;

int commentEntriesLength = 0;

static void fill_creator_entries(char **creatorStrings, int creatorStringCount) {
    for (int i = 0; i < creatorStringCount; i++) {
        int stringCount;
        char **creatorString = split_string(creatorStrings[i], ':', &stringCount, true);
        if (!creatorString) return;

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
        if (!songKeys) return;

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
                    song_entries[i].songSize = atof(valStr);
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

static void fill_song_entry(char *songString, int targetEntry) {
        int songKeyCount = 0;

        char **songKeys = split_string_str_del(songString, "~|~", &songKeyCount, true);
        if (!songKeys) return;

        for (int i = 0; i + 1 < songKeyCount; i += 2) {
            int key = atoi(songKeys[i]);
            char *valStr = songKeys[i + 1];
            switch (key) {
                case 10:
                    // song link
                    strncpy(song_entries[targetEntry].songLink, valStr, sizeof(song_entries[targetEntry].songLink) - 1);
                    break;
            }
        }
        free_string_array(songKeys, songKeyCount);
    
}

static void fill_level_entries(char **levelsStrings, int songStringCount, int creatorStringCount, int levelStringCount) {
    for (int i = 0; i < levelStringCount; i++) {
        int levelKeyCount = 0;
        char **levelKeys = split_string(levelsStrings[i], ':', &levelKeyCount, true);
        if (!levelKeys) return;

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
                case 42:
                    //epic, legendary, mythic
                    search_entries[i].epic = atoi(valStr);
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
        
        search_entries[i].songIndex = song_index;
        search_entries[i].creatorIndex = creator_index;
    }
}

static void fill_level_entry(char **levelStrings, int levelStringsCount, bool fillSearchEntry, int searchId) {
    int levelKeyCount = 0;

    char **levelKeys = split_string(levelStrings[0], ':', &levelKeyCount, true);
    if (!levelKeys) return;
    for (int j = 0; j + 1 < levelKeyCount; j += 2) {
        int key = atoi(levelKeys[j]);
        char *valStr = levelKeys[j + 1];;
        switch (key) {
        case 1:
            // level id
            if (fillSearchEntry)
                search_entries[searchId].levelId = atoi(valStr);
            break;
        case 2:
            // level name
            if (fillSearchEntry)
                strncpy(search_entries[searchId].name, valStr, sizeof(search_entries[searchId].name) - 1);
            break;
        case 3:
            // level description
            if (fillSearchEntry)
            {

                
                if (valStr[0] == '\0')
                {
                    search_entries[searchId].description = strdup("No description provided.");
                    break;
                }
                if (gdps) {
                    search_entries[searchId].description = strdup(valStr);
                    break;
                };
                fix_base64_url(valStr);
                search_entries[searchId].description = malloc(strlen(valStr) + 1);
                int decoded_len = base64_decode(valStr, (unsigned char *)search_entries[searchId].description);
                if (decoded_len > 0)
                {
                    search_entries[searchId].description[decoded_len] = '\0';
                }
            }
            break;
        case 5:
            // level version
            if (fillSearchEntry)
                search_entries[searchId].levelVersion = atoi(valStr);
            break;
        case 6:
            // creator player id
            if (fillSearchEntry)
                search_entries[searchId].creatorId = atoi(valStr);
            break;
        case 9:
            // level difficulty
            search_entries[searchId].difficulty = atoi(valStr) / 10;
            break;
        case 10:
            // level downloads
            if (fillSearchEntry)
                search_entries[searchId].downloads = atoi(valStr);
            break;
        case 12:
            // main level song, 0 if custom song is present
            if (fillSearchEntry)
                search_entries[searchId].mainSongId = atoi(valStr);
            break;
        case 13:
            // game version the level was uploaded in
            if (fillSearchEntry)
                search_entries[searchId].gameVersion = atoi(valStr);
            break;
        case 14:
            // level likes, formula is likes - dislikes
            if (fillSearchEntry)
                search_entries[searchId].likes = atoi(valStr);
            break;
        case 15:
            // level length
            if (fillSearchEntry)
                search_entries[searchId].lengthNum = atoi(valStr);
            break;
        case 17:
            // demon status
            if (fillSearchEntry)
                search_entries[searchId].isDemon = parse_bool(valStr);
            break;
        case 18:
            // stars
            if (fillSearchEntry)
                search_entries[searchId].stars = atoi(valStr);
            break;
        case 19:
            // feature score
            if (fillSearchEntry)
                search_entries[searchId].featureScore = atoi(valStr);
            break;
        case 25:
            // auto status
            if (fillSearchEntry)
                search_entries[searchId].isAuto = parse_bool(valStr);
            break;
        case 30:
            // if level is a copy, id of original level
            if (fillSearchEntry)
                search_entries[searchId].originalId = atoi(valStr);
            break;
        case 31:
            // two player status
            if (fillSearchEntry)
                search_entries[searchId].isTwoPlayer = parse_bool(valStr);
            break;
        case 35:
            // newgrounds song id
            if (fillSearchEntry)
                search_entries[searchId].songId = atoi(valStr);
            break;
        case 39:
            // stars requested
            if (fillSearchEntry)
                search_entries[searchId].reqStars = atoi(valStr);
            break;
        case 42:
            // epic, legendary, mythic
            if (fillSearchEntry)
                search_entries[searchId].epic = atoi(valStr);
        case 45:
            // object count, caps at 65535
            if (fillSearchEntry)
                search_entries[searchId].objCount = atoi(valStr);
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

void fill_page_entry(char *initialString) {
    int stringCount;
    char **pageStrings = split_string(initialString, ':', &stringCount, true);
    if (!pageStrings) return;

    page_entry->totalLevels = atoi(pageStrings[0]);
    page_entry->currentOffset = atoi(pageStrings[1]);
    page_entry->amount = atoi(pageStrings[2]);
    page_entry->totalPages = (((page_entry->totalLevels + (page_entry->amount - 1)) / page_entry->amount));
    free_string_array(pageStrings, stringCount);
};

void fill_comment_entries(char **commentStrings, int commentStringCount) {
    for (int i = 0; i < commentStringCount; i++) {
        int commentKeyCount = 0;

        char **commentKeys = split_string(commentStrings[i], ':', &commentKeyCount, true);
        if (!commentKeys) return;
        
        for (int j = 0; j + 1 < commentKeyCount; j += 2) {
            int commentDataKeyCount = 0;
            int authorDataKeyCount = 0;
            char **commentData = split_string(commentKeys[0], '~', &commentDataKeyCount, true);
            char **authorData = split_string(commentKeys[1], '~', &authorDataKeyCount, true);

            // comment data entries
            for (int k = 0; k + 1 < commentDataKeyCount; k += 2) {
                int key = atoi(commentData[k]);
                char *valStr = commentData[k + 1];
                switch (key) {
                    case 1:
                        // id of level the comment comes from
                        comment_entries[i].levelId = atoi(valStr);
                        break;
                    case 2:
                        // base64 encoded comment text content
                        if (valStr[0] == '\0')
                        {
                            comment_entries[i].content = strdup("No");
                            break;
                        }

                        fix_base64_url(valStr);
                        comment_entries[i].content = malloc(strlen(valStr) + 1);
                        int decoded_len = base64_decode(valStr, (unsigned char *)comment_entries[i].content);
                        if (decoded_len > 0)
                        {
                            comment_entries[i].content[decoded_len] = '\0';
                        }
                        break;
                    case 3:
                        // author's player id
                        comment_entries[i].authorPlayerId = atoi(valStr);
                        break;
                    case 4:
                        // likes
                        comment_entries[i].likes = atoi(valStr);
                        break;
                    case 7:
                        // spam flag status
                        comment_entries[i].isSpam = parse_bool(valStr);
                        break; 
                    case 9:
                        // time since comment was posted
                        strncpy(comment_entries[i].commentAge, valStr, sizeof(comment_entries[i].commentAge) - 1);
                        break;
                    case 10:
                        // percent on source level
                        comment_entries[i].percent = atoi(valStr);
                        break;
                    case 11:
                        // mod badge status
                        comment_entries[i].modBadge = atoi(valStr);
                        break;
                    case 12:
                        // color of username (if mod)
                        int rgbCount = 0;
                        char **rgbData = split_string(valStr, ',', &rgbCount, true);
                        if (!rgbData) break;

                        snprintf(comment_entries[i].modCommentColor, sizeof(comment_entries[i].modCommentColor) - 1, "#%02X%02X%02X", atoi(rgbData[0]), atoi(rgbData[1]), atoi(rgbData[2]));
                
                        free_string_array(rgbData, rgbCount);
                        break;
                
                }
            }
            // comment author entries
            for (int k = 0; k + 1 < authorDataKeyCount; k += 2) {
                int key = atoi(authorData[k]);
                char *valStr = authorData[k + 1];
                switch (key) {
                    case 1:
                        // author username
                        strncpy(comment_entries[i].name, valStr, sizeof(comment_entries[i].name) - 1);
                        break;
                    case 9:
                        // player icon index
                        comment_entries[i].playerIcon = atoi(valStr);
                        break;
                    case 10:
                        // author icon primary color
                        comment_entries[i].col1 = atoi(valStr);
                        break;
                    case 11:
                        // author icon secondary color
                        comment_entries[i].col2 = atoi(valStr);
                        break;
                    case 14:
                        // icon type (gamemode)
                        comment_entries[i].iconType = atoi(valStr);
                        break;
                    case 15:
                        // icon glow (why is 0 false but 2 true??????)
                        comment_entries[i].glow = (atoi(valStr) == 2);
                        break;
                    case 16:
                    // author account id
                        comment_entries[i].authorAccountId = atoi(valStr);
                        break;
                    case 51:
                        // glow color 
                        comment_entries[i].glowCol = atoi(valStr);
                        break;
                }
            }
            free_string_array(commentData, commentDataKeyCount);
            free_string_array(authorData, authorDataKeyCount);

        }
        free_string_array(commentKeys, commentKeyCount);
    }
}

void fill_gdps_comment_entries(char **commentStrings, int commentStringCount) {
    for (int i = 0; i < commentStringCount; i++) {
            int commentKeyCount = 0;
            char **comments = split_string(commentStrings[i], '~', &commentKeyCount, true);

            for (int k = 0; k + 1 < commentKeyCount; k += 2) {
                int key = atoi(comments[k]);
                char *valStr = comments[k + 1];
                switch (key) {
                    case 1:
                        // id of level the comment comes from
                        comment_entries[i].levelId = atoi(valStr);
                        break;
                    case 2:
                        // comment text content (not base64 encoded this time!!!)
                        if (valStr[0] == '\0') {
                            comment_entries[i].content = strdup("");
                            break;
                        }
                        comment_entries[i].content = malloc(strlen(valStr) + 1);
                        snprintf(comment_entries[i].content, strlen(valStr) + 1, valStr);
                        break;
                    case 3:
                        // author's player id
                        comment_entries[i].authorPlayerId = atoi(valStr);
                        break;
                    case 4:
                        // likes
                        comment_entries[i].likes = atoi(valStr);
                        break;
                    case 7:
                        // spam flag status
                        comment_entries[i].isSpam = parse_bool(valStr);
                        break; 
                    case 9:
                        // time since comment was posted
                        strncpy(comment_entries[i].commentAge, valStr, sizeof(comment_entries[i].commentAge) - 1);
                        break;
                    case 10:
                        // percent on source level
                        comment_entries[i].percent = atoi(valStr);
                        break;
                    case 11:
                        // mod badge status
                        comment_entries[i].modBadge = atoi(valStr);
                        break;
                    case 12:
                        // color of username (if mod)

                        int rgbCount = 0;
                        char **rgbData = split_string(valStr, ',', &rgbCount, true);
                        if (!rgbData) break;

                        snprintf(comment_entries[i].modCommentColor, sizeof(comment_entries[i].modCommentColor) - 1, "#%02X%02X%02X", atoi(rgbData[0]), atoi(rgbData[1]), atoi(rgbData[2]));
        
                        free_string_array(rgbData, rgbCount);
                        break;
                
                }
            }
        free_string_array(comments, commentKeyCount);
    }
}

void fill_gdps_comment_author_entries(char **authorStrings, int authorStringCount, int commentStringCount) {
    for (int i = 0; i < authorStringCount; i++) {
        int stringCount;
        char **authorString = split_string(authorStrings[i], ':', &stringCount, true);
        if (!authorString) return;

        char name[21];
        int playerId = atoi(authorString[0]);
        strncpy(name, authorString[1], sizeof(name) - 1);
        int userId = atoi(authorString[2]);

        int commentIndex;
        
        // Find the song
        for (commentIndex = 0; commentIndex < commentStringCount; commentIndex++) {
            if (playerId == comment_entries[commentIndex].authorPlayerId) {
                snprintf(comment_entries[commentIndex].name, sizeof(comment_entries[commentIndex].name), name);
                comment_entries[commentIndex].authorAccountId = userId;
                break;
            }
        }

        free_string_array(authorString, stringCount);
    }
}

int search_levels_internal(NetworkTask *task, bool useGdps) {
    char *outdata;
    int result = get_search_results(task, &outdata, 22, filters, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the search results string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == ':')) return -2;

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
    // if (!songStrings && !filters.mainSong) return -1; 
    
    search_entries = malloc(levelStringCount * sizeof(SearchEntry));
    if (!search_entries) return -1;

    creator_entries = malloc(creatorStringCount * sizeof(CreatorEntry));
    if (!creator_entries) return -1;

    if (songStringCount > 0) {
        song_entries = malloc(songStringCount * sizeof(SongEntry));
        if (!song_entries) return -1;
    }

    page_entry = malloc(sizeof(SongEntry));
    if (!page_entry) return -1;

    // Initialize
    memset(search_entries, 0, levelStringCount * sizeof(SearchEntry));
    memset(creator_entries, 0, creatorStringCount * sizeof(CreatorEntry));
    if (songStringCount > 0) memset(song_entries, 0, songStringCount * sizeof(SongEntry));
    memset(page_entry, 0, sizeof(PageEntry));

    // Fill creators
    fill_creator_entries(creatorStrings, creatorStringCount);

    // Fill songs
    if (songStringCount > 0) fill_song_entries(songStrings, songStringCount);
    
    // Fill levels
    fill_level_entries(levelsStrings, songStringCount, creatorStringCount, levelStringCount);

    // Fill pages
    fill_page_entry(initialStrings[3]);
   
    free_string_array(levelsStrings, levelStringCount);
    free_string_array(creatorStrings, creatorStringCount);
    if (songStringCount > 0) free_string_array(songStrings, songStringCount);    
    free_string_array(initialStrings, initialStringCount);

    free(outdata);

    creatorEntriesLength = creatorStringCount;
    songEntriesLength = songStringCount;
    searchEntriesLength = levelStringCount;
    return 0;
}

int get_level_data_internal(NetworkTask *task, int id, bool refresh, int currentId, bool useGdps) {
    char *outdata;
    int result = get_level_from_id(task, &outdata, id, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the level string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == ':')) return -2;

    int initialStringCount = 0;

    char **initialStrings = split_string(outdata, '#', &initialStringCount, true);
    if (!initialStrings) return -1;

    level_entry = malloc(initialStringCount * sizeof(LevelEntry));
    if (!level_entry) return -1;

    // Initialize
    memset(level_entry, 0, initialStringCount * sizeof(LevelEntry));

    fill_level_entry(initialStrings, 1, refresh, currentId);

    free_string_array(initialStrings, initialStringCount);

    free(outdata);

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

int get_comments_internal(NetworkTask *task, int id, int page, int sortType, bool useGdps) {
    char *outdata;
    int result = get_comments_from_id(task, &outdata, id, page, sortType, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the comments string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == '~')) return -2;

    // i dont know why but the 1.9 gdps handles its comments completely differently, very annoying
    if (!useGdps) {
        int commentStringCount = 0;

        char **commentStrings = split_string(outdata, '|', &commentStringCount, true);
        if (!commentStrings)
            return -1;

        comment_entries = malloc(commentStringCount * sizeof(CommentEntry));
        if (!comment_entries)
            return -1;

        // Initialize
        memset(comment_entries, 0, commentStringCount * sizeof(CommentEntry));

        fill_comment_entries(commentStrings, commentStringCount);

        free_string_array(commentStrings, commentStringCount);

        free(outdata);

        commentEntriesLength = commentStringCount;
    } else {
        // gdps exclusive slop
        int initialStringCount = 0;

        char **initialStrings = split_string(outdata, '#', &initialStringCount, true);
        if (!initialStrings) return -1;

        int commentStringCount = 0;
        int commentAuthorStringCount = 0;

        char **commentStrings = split_string(initialStrings[0], '|', &commentStringCount, true);
        if (!commentStrings) return -1;

        char **commentAuthorStrings = split_string(initialStrings[1], '|', &commentAuthorStringCount, true);
        if (!commentAuthorStrings) return -1;

        comment_entries = malloc(commentStringCount * sizeof(CommentEntry));
        if (!comment_entries) return -1;
        
        commentEntriesLength = commentStringCount;

        // Initialize
        memset(comment_entries, 0, commentStringCount * sizeof(CommentEntry));

        // Fill comment entries
        fill_gdps_comment_entries(commentStrings, commentStringCount);

        // Fill missing creator entries
        fill_gdps_comment_author_entries(commentAuthorStrings, commentAuthorStringCount, commentStringCount);

        free_string_array(commentStrings, commentStringCount);
        free_string_array(commentAuthorStrings, commentAuthorStringCount);
        free_string_array(initialStrings, initialStringCount);

    }

    return 0;
}

int get_song_data_internal(NetworkTask *task, int songId, int targetSongEntry, bool useGdps) {
    char *outdata;
    int result = get_song_info_from_id(task, &outdata, songId, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the level string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == '~')) return -2;

    int initialStringCount = 0;

    fill_song_entry(outdata, targetSongEntry);

    free(outdata);

    levelEntryLength = initialStringCount;
    return 0;
}

//intermediate functions

int search_levels(NetworkTask *task) {
    int result = search_levels_internal(task, gdps);
    return result;
}

int get_level(NetworkTask *task) {
    int result = get_level_data_internal(task, search_entries[curr_search_id].levelId, refresh, curr_search_id, gdps);
    return result;
}

int get_comments(NetworkTask *task) {
    int result = get_comments_internal(task, search_entries[curr_search_id].levelId, current_comments_page, comments_sort_type, gdps);
    return result;
}

int get_song_data(NetworkTask *task) {
    int result = get_song_data_internal(task, search_entries[curr_search_id].songId, search_entries[curr_search_id].songIndex, gdps);
    return result;
}