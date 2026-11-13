#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <citro2d.h>
#include <string.h>
#include <sys/_intsup.h>

#include "github_info.hpp"
#include "json-c/json_object.h"
#include "json-c/json_tokener.h"
#include "json-c/json_types.h"

#include "level_loading.hpp"
#include "main.hpp"
#include "save/saving.hpp"
#include "utils/folders.hpp"
#include "utils/server_utils.hpp"
#include "network.hpp"
#include "utils/json_config.hpp"
#include "utils/string_helpers.hpp"

#include "menus/components/ui_list.hpp"
#include "menus/updater_pop_up.hpp"
#include "menus/updater_settings.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/creator_menu/online/online_level_menu.hpp"
#include "menus/creator_menu/online/online_level_comments.hpp"
#include "utils/utils.hpp"

SearchEntry *searchEntries = NULL;
CreatorEntry *creatorEntries = NULL;
SongEntry *songEntries = NULL;
PageEntry *pageEntry = NULL;

LevelEntry *levelEntry = NULL;

CommentEntry *commentEntries = NULL;

SearchFilters filters = { 0 };

UpdateData *updateData = NULL;

int searchEntriesLength = 0;
int creatorEntriesLength = 0;
int songEntriesLength = 0;

int levelEntryLength = 0;

int commentEntriesLength = 0;

char **latestCommits;
int latestCommitCount = 0;

static void fillCreatorEntries(char **creatorStrings, int creatorStringCount) {
    for (int i = 0; i < creatorStringCount; i++) {
        int stringCount;
        char **creatorString = splitString(creatorStrings[i], ':', &stringCount, true);
        if (!creatorString) return;

        creatorEntries[i].accountId = atoi(creatorString[0]);
        strncpy(creatorEntries[i].creatorName, creatorString[1], sizeof(creatorEntries[i].creatorName) - 1);
        creatorEntries[i].userId = atoi(creatorString[2]);
        freeStringArray(creatorString, stringCount);
    }
}

static void fillSongEntries(char **songStrings, int songStringCount) {
    for (int i = 0; i < songStringCount; i++) {
        int songKeyCount = 0;

        char **songKeys = splitStringStrDel(songStrings[i], "~|~", &songKeyCount, true);
        if (!songKeys) return;

        for (int j = 0; j + 1 < songKeyCount; j += 2) {
            int key = atoi(songKeys[j]);
            char *valStr = songKeys[j + 1];
            switch (key) {
                case 1:
                    // song id
                    songEntries[i].ngSongId = atoi(valStr);
                    break;
                case 2:
                    // song name
                    strncpy(songEntries[i].songTitle, valStr, sizeof(songEntries[i].songTitle) - 1);
                    break;
                case 4:
                    // artist name
                    strncpy(songEntries[i].artistName, valStr, sizeof(songEntries[i].artistName) - 1);
                    break;
                case 5:
                    // song size
                    songEntries[i].songSize = atof(valStr);
                    break;
                case 10:
                    // song link
                    strncpy(songEntries[i].songLink, valStr, sizeof(songEntries[i].songLink) - 1);
                    break;
            }
        }
        freeStringArray(songKeys, songKeyCount);
    }
}

static void fillSongEntry(char *songString, SongEntry *targetEntry) {
        int songKeyCount = 0;

        char **songKeys = splitStringStrDel(songString, "~|~", &songKeyCount, true);
        if (!songKeys) return;

        for (int i = 0; i + 1 < songKeyCount; i += 2) {
            int key = atoi(songKeys[i]);
            char *valStr = songKeys[i + 1];
            switch (key) {
                case 10:
                    // song link
                    strncpy(targetEntry->songLink, valStr, sizeof(targetEntry->songLink) - 1);
                    break;
            }
        }
        freeStringArray(songKeys, songKeyCount);
    
}

static void fillLevelEntries(char **levelsStrings, int songStringCount, int creatorStringCount, int levelStringCount) {
    for (int i = 0; i < levelStringCount; i++) {
        searchEntries[i].coins = 0;
        int levelKeyCount = 0;
        char **levelKeys = splitString(levelsStrings[i], ':', &levelKeyCount, true);
        if (!levelKeys) return;

        for (int j = 0; j + 1 < levelKeyCount; j += 2) {
            int key = atoi(levelKeys[j]);
            char *valStr = levelKeys[j + 1];

            switch (key) {
                case 1: 
                    // level id
                    searchEntries[i].levelId = atoi(valStr);
                    break;
                case 2: 
                    // level name
                    strncpy(searchEntries[i].name, valStr, sizeof(searchEntries[i].name) - 1);
                    break;
                case 3: {
                    // level description
                    if (valStr[0] == '\0') {
                        searchEntries[i].description = strdup("No description provided.");
                        break;
                    }

                    fixBase64URL(valStr);
                    searchEntries[i].description = (char *)malloc(strlen(valStr) + 1);
                    int decodedLen = base64Decode(valStr, (unsigned char *)searchEntries[i].description);
                    if (decodedLen > 0) {
                        searchEntries[i].description[decodedLen] = '\0';
                    }
                    break;
                }
                case 5: 
                    // level version
                    searchEntries[i].levelVersion = atoi(valStr);
                    break;
                case 6:
                    // creator player id
                    searchEntries[i].creatorId = atoi(valStr);
                    break;
                case 9:
                    // level difficulty
                    searchEntries[i].difficulty = atoi(valStr) / 10;
                    break;
                case 10:
                    // level downloads
                    searchEntries[i].downloads = atoi(valStr);
                    break;
                case 12:
                    // main level song, 0 if custom song is present
                    searchEntries[i].mainSongId = atoi(valStr);
                    break;
                case 13:
                    // game version the level was uploaded in 
                    searchEntries[i].gameVersion = atoi (valStr);
                    break;
                case 14:
                    // level likes, formula is likes - dislikes
                    searchEntries[i].likes = atoi(valStr);
                    break;
                case 15:
                    // level length
                    searchEntries[i].lengthNum = atoi(valStr);
                    break;
                case 16:
                    // level dislikes, we have no use for this (formula is dislikes - likes)
                    break;
                case 17:
                    // demon status
                    searchEntries[i].isDemon = parseBool(valStr);
                    break;
                case 18: 
                    // stars
                    searchEntries[i].stars = atoi(valStr);
                    break;
                case 19: 
                    // feature score
                    searchEntries[i].featureScore = atoi(valStr);
                    break;
                case 25:
                    // auto status
                    searchEntries[i].isAuto = parseBool(valStr);
                    break;
                case 30:
                    // if level is a copy, id of original level
                    searchEntries[i].originalId = atoi(valStr);
                    break;
                case 31:
                    // two player status
                    searchEntries[i].isTwoPlayer = parseBool(valStr);
                    break;
                case 35:
                    // newgrounds song id
                    searchEntries[i].songId = atoi(valStr);
                    break;
                case 37:
                    // coins
                    {
                        int coins = atoi(valStr);
                        if (coins < 0) coins = 0;
                        if (coins > 3) coins = 3;
                        searchEntries[i].coins = coins;
                    }
                    break;
                case 38: 
                    // verified coins
                    searchEntries[i].verifiedCoins = parseBool(valStr);
                    break;
                case 39:
                    // stars requested
                    searchEntries[i].reqStars = atoi(valStr);
                    break;
                case 42:
                    //epic, legendary, mythic
                    searchEntries[i].epic = atoi(valStr);
                case 45:
                    // object count, caps at 65535
                    searchEntries[i].objCount = atoi(valStr);
                    break;
            }
        }
        freeStringArray(levelKeys, levelKeyCount);

        int songIndex;
        int creatorIndex;
        
        bool found = false;
        // Find the song
        for (songIndex = 0; songIndex < songStringCount; songIndex++) {
            if (searchEntries[i].songId == songEntries[songIndex].ngSongId) {
                found = true;
                break;
            }
        }
        
        searchEntries[i].songIndex = (found ? songIndex : -1);

        found = false;
        // Find the creator
        for (creatorIndex = 0; creatorIndex < creatorStringCount; creatorIndex++) {
            if (searchEntries[i].creatorId == creatorEntries[creatorIndex].accountId) {
                found = true;
                break;
            }
        }
        searchEntries[i].creatorIndex = (found ? creatorIndex : -1);
    }
}

static void fillLevelEntry(char **levelStrings, int levelStringsCount, bool fillSearchEntry, int searchId) {
    int levelKeyCount = 0;

    SavedLevelDataEntry *entry = getSavedLevelData(onlineMenuLevelID);

    char **levelKeys = splitString(levelStrings[0], ':', &levelKeyCount, true);
    if (!levelKeys) return;
    for (int j = 0; j + 1 < levelKeyCount; j += 2) {
        int key = atoi(levelKeys[j]);
        char *valStr = levelKeys[j + 1];;
        switch (key) {
        case 1:
            // level id
            if (fillSearchEntry)
                currentSearchEntry->levelId = atoi(valStr);
            break;
        case 2:
            // level name
            if (fillSearchEntry)
                strncpy(currentSearchEntry->name, valStr, sizeof(currentSearchEntry->name) - 1);
            break;
        case 3:
            // level description
            if (fillSearchEntry)
            {
                if (valStr[0] == '\0')
                {
                    currentSearchEntry->description = strdup("No description provided.");
                    break;
                }
                if (gdps) {
                    currentSearchEntry->description = strdup(valStr);
                    break;
                };
                fixBase64URL(valStr);
                currentSearchEntry->description = (char *)malloc(strlen(valStr) + 1);
                int decodedLen = base64Decode(valStr, (unsigned char *)currentSearchEntry->description);
                if (decodedLen > 0)
                {
                    currentSearchEntry->description[decodedLen] = '\0';
                }
            }
            break;
        case 5:
            // level version
            if (fillSearchEntry)
                currentSearchEntry->levelVersion = atoi(valStr);
            break;
        case 6:
            // creator player id
            if (fillSearchEntry)
                currentSearchEntry->creatorId = atoi(valStr);
            break;
        case 9:
            // level difficulty
            currentSearchEntry->difficulty = atoi(valStr) / 10;
            break;
        case 10:
            // level downloads
            if (fillSearchEntry)
                currentSearchEntry->downloads = atoi(valStr);
            break;
        case 12:
            // main level song, 0 if custom song is present
            if (fillSearchEntry)
                currentSearchEntry->mainSongId = atoi(valStr);
            break;
        case 13:
            // game version the level was uploaded in
            if (fillSearchEntry)
                currentSearchEntry->gameVersion = atoi(valStr);
            break;
        case 14:
            // level likes, formula is likes - dislikes
            if (fillSearchEntry)
                currentSearchEntry->likes = atoi(valStr);
            break;
        case 15:
            // level length
            if (fillSearchEntry)
                currentSearchEntry->lengthNum = atoi(valStr);
            break;
        case 17:
            // demon status
            if (fillSearchEntry)
                currentSearchEntry->isDemon = parseBool(valStr);
            break;
        case 18:
            // stars
            if (fillSearchEntry)
                currentSearchEntry->stars = atoi(valStr);
            break;
        case 19:
            // feature score
            if (fillSearchEntry)
                currentSearchEntry->featureScore = atoi(valStr);
            break;
        case 25:
            // auto status
            if (fillSearchEntry)
                currentSearchEntry->isAuto = parseBool(valStr);
            break;
        case 30:
            // if level is a copy, id of original level
            if (fillSearchEntry)
                currentSearchEntry->originalId = atoi(valStr);
            break;
        case 31:
            // two player status
            if (fillSearchEntry)
                currentSearchEntry->isTwoPlayer = parseBool(valStr);
            break;
        case 35:
            // newgrounds song id
            if (fillSearchEntry)
                currentSearchEntry->songId = atoi(valStr);
            break;
        case 37:
            // coins
            if (fillSearchEntry) {
                int coins = atoi(valStr);
                if (coins < 0) coins = 0;
                if (coins > 3) coins = 3;
                currentSearchEntry->coins = coins;
            }
            break;
        case 38:
            // verified coins
            if (fillSearchEntry)
                currentSearchEntry->verifiedCoins = parseBool(valStr);
            break;
        case 39:
            // stars requested
            if (fillSearchEntry)
                currentSearchEntry->reqStars = atoi(valStr);
            break;
        case 42:
            // epic, legendary, mythic
            if (fillSearchEntry)
                currentSearchEntry->epic = atoi(valStr);
        case 45:
            // object count, caps at 65535
            if (fillSearchEntry)
                currentSearchEntry->objCount = atoi(valStr);
            break;
        case 4:
            // base64 encoded probably compressed level string
            levelEntry->levelString = strdup(valStr);

            // Save level string
            saveSavedLevel(onlineMenuLevelID, gdps, levelEntry->levelString);
            break;
        case 28:
            // time since upload
            strncpy(levelEntry->uploadDate, valStr, sizeof(levelEntry->uploadDate) - 1);
            strncpy(entry->levelEntry.uploadDate, valStr, sizeof(entry->levelEntry.uploadDate) - 1);
            break;
        case 29:
            // time since last
            strncpy(levelEntry->updateDate, valStr, sizeof(levelEntry->updateDate) - 1);
            strncpy(entry->levelEntry.updateDate, valStr, sizeof(entry->levelEntry.updateDate) - 1);
            break;
        }
    }
    
    entry->version = SAVED_LEVELS_VERSION;
    freeStringArray(levelKeys, levelKeyCount);
}

void fillPageEntry(char *initialString) {
    int stringCount;
    char **pageStrings = splitString(initialString, ':', &stringCount, true);
    if (!pageStrings) return;

    pageEntry->totalLevels = atoi(pageStrings[0]);
    pageEntry->currentOffset = atoi(pageStrings[1]);
    pageEntry->amount = atoi(pageStrings[2]);
    pageEntry->totalPages = (((pageEntry->totalLevels + (pageEntry->amount - 1)) / pageEntry->amount));
    freeStringArray(pageStrings, stringCount);
};

void fillCommentEntries(char **commentStrings, int commentStringCount) {
    for (int i = 0; i < commentStringCount; i++) {
        int commentKeyCount = 0;

        char **commentKeys = splitString(commentStrings[i], ':', &commentKeyCount, true);
        if (!commentKeys) return;
        
        for (int j = 0; j + 1 < commentKeyCount; j += 2) {
            int commentDataKeyCount = 0;
            int authorDataKeyCount = 0;
            char **commentData = splitString(commentKeys[0], '~', &commentDataKeyCount, true);
            char **authorData = splitString(commentKeys[1], '~', &authorDataKeyCount, true);

            // comment data entries
            for (int k = 0; k + 1 < commentDataKeyCount; k += 2) {
                int key = atoi(commentData[k]);
                char *valStr = commentData[k + 1];
                switch (key) {
                    case 1:
                        // id of level the comment comes from
                        commentEntries[i].levelId = atoi(valStr);
                        break;
                    case 2: {
                        // base64 encoded comment text content
                        if (valStr[0] == '\0')
                        {
                            commentEntries[i].content = strdup("No");
                            break;
                        }

                        fixBase64URL(valStr);
                        commentEntries[i].content = (char *)malloc(strlen(valStr) + 1);
                        int decodedLen = base64Decode(valStr, (unsigned char *)commentEntries[i].content);
                        if (decodedLen > 0)
                        {
                            commentEntries[i].content[decodedLen] = '\0';
                        }
                        break;
                    }
                    case 3:
                        // author's player id
                        commentEntries[i].authorPlayerId = atoi(valStr);
                        break;
                    case 4:
                        // likes
                        commentEntries[i].likes = atoi(valStr);
                        break;
                    case 7:
                        // spam flag status
                        commentEntries[i].isSpam = parseBool(valStr);
                        break; 
                    case 9:
                        // time since comment was posted
                        strncpy(commentEntries[i].commentAge, valStr, sizeof(commentEntries[i].commentAge) - 1);
                        break;
                    case 10:
                        // percent on source level
                        commentEntries[i].percent = atoi(valStr);
                        break;
                    case 11:
                        // mod badge status
                        commentEntries[i].modBadge = atoi(valStr);
                        break;
                    case 12: {
                        // color of username (if mod)
                        int rgbCount = 0;
                        char **rgbData = splitString(valStr, ',', &rgbCount, true);
                        if (!rgbData) break;

                        snprintf(commentEntries[i].modCommentColor, sizeof(commentEntries[i].modCommentColor) - 1, "#%02X%02X%02X", atoi(rgbData[0]), atoi(rgbData[1]), atoi(rgbData[2]));
                
                        freeStringArray(rgbData, rgbCount);
                        break;
                    }
                }
            }
            // comment author entries
            for (int k = 0; k + 1 < authorDataKeyCount; k += 2) {
                int key = atoi(authorData[k]);
                char *valStr = authorData[k + 1];
                switch (key) {
                    case 1:
                        // author username
                        strncpy(commentEntries[i].name, valStr, sizeof(commentEntries[i].name) - 1);
                        break;
                    case 9:
                        // player icon index
                        commentEntries[i].playerIcon = atoi(valStr);
                        break;
                    case 10:
                        // author icon primary color
                        commentEntries[i].col1 = atoi(valStr);
                        break;
                    case 11:
                        // author icon secondary color
                        commentEntries[i].col2 = atoi(valStr);
                        break;
                    case 14:
                        // icon type (gamemode)
                        commentEntries[i].iconType = atoi(valStr);
                        break;
                    case 15:
                        // icon glow (why is 0 false but 2 true??????)
                        commentEntries[i].glow = (atoi(valStr) == 2);
                        break;
                    case 16:
                    // author account id
                        commentEntries[i].authorAccountId = atoi(valStr);
                        break;
                    case 51:
                        // glow color 
                        commentEntries[i].glowCol = atoi(valStr);
                        break;
                }
            }
            freeStringArray(commentData, commentDataKeyCount);
            freeStringArray(authorData, authorDataKeyCount);

        }
        freeStringArray(commentKeys, commentKeyCount);
    }
}

void fillGdpsCommentEntries(char **commentStrings, int commentStringCount) {
    for (int i = 0; i < commentStringCount; i++) {
            int commentKeyCount = 0;
            char **comments = splitString(commentStrings[i], '~', &commentKeyCount, true);

            for (int k = 0; k + 1 < commentKeyCount; k += 2) {
                int key = atoi(comments[k]);
                char *valStr = comments[k + 1];
                switch (key) {
                    case 1:
                        // id of level the comment comes from
                        commentEntries[i].levelId = atoi(valStr);
                        break;
                    case 2:
                        // comment text content (not base64 encoded this time!!!)
                        if (valStr[0] == '\0') {
                            commentEntries[i].content = strdup("");
                            break;
                        }
                        commentEntries[i].content = (char *)malloc(strlen(valStr) + 1);
                        snprintf(commentEntries[i].content, strlen(valStr) + 1, "%s", valStr);
                        break;
                    case 3:
                        // author's player id
                        commentEntries[i].authorPlayerId = atoi(valStr);
                        break;
                    case 4:
                        // likes
                        commentEntries[i].likes = atoi(valStr);
                        break;
                    case 7:
                        // spam flag status
                        commentEntries[i].isSpam = parseBool(valStr);
                        break; 
                    case 9:
                        // time since comment was posted
                        strncpy(commentEntries[i].commentAge, valStr, sizeof(commentEntries[i].commentAge) - 1);
                        break;
                    case 10:
                        // percent on source level
                        commentEntries[i].percent = atoi(valStr);
                        break;
                    case 11:
                        // mod badge status
                        commentEntries[i].modBadge = atoi(valStr);
                        break;
                    case 12:
                        // color of username (if mod)

                        int rgbCount = 0;
                        char **rgbData = splitString(valStr, ',', &rgbCount, true);
                        if (!rgbData) break;

                        snprintf(commentEntries[i].modCommentColor, sizeof(commentEntries[i].modCommentColor) - 1, "#%02X%02X%02X", atoi(rgbData[0]), atoi(rgbData[1]), atoi(rgbData[2]));
        
                        freeStringArray(rgbData, rgbCount);
                        break;
                
                }
            }
        freeStringArray(comments, commentKeyCount);
    }
}

void fillGdpsCommentAuthorEntries(char **authorStrings, int authorStringCount, int commentStringCount) {
    for (int i = 0; i < authorStringCount; i++) {
        int stringCount;
        char **authorString = splitString(authorStrings[i], ':', &stringCount, true);
        if (!authorString) return;

        char name[21];
        int playerId = atoi(authorString[0]);
        strncpy(name, authorString[1], sizeof(name) - 1);
        int userId = atoi(authorString[2]);

        int commentIndex;
        
        // Find the song
        for (commentIndex = 0; commentIndex < commentStringCount; commentIndex++) {
            if (playerId == commentEntries[commentIndex].authorPlayerId) {
                snprintf(commentEntries[commentIndex].name, sizeof(commentEntries[commentIndex].name), "%s", name);
                commentEntries[commentIndex].authorAccountId = userId;
                break;
            }
        }

        freeStringArray(authorString, stringCount);
    }
}

int parseLatestCommit(char *jsonString) {
    // parse response from github as json object 
    json_object *jsonObject = json_tokener_parse(jsonString);
    json_object *commitObject = NULL;
    json_object *hashObject = NULL;
    json_object *status = NULL;

    // check if we didnt get served garbage data
    if (!jsonObject) {
        outputLog("failure parsing commit data: response - %s", jsonString);
        return -1;
    }


    if (json_object_object_get_ex(jsonObject, "status", &status)) {
      // check what github wants to tell us
      outputLog("status code: %d\n%s\n", json_object_get_int(status), jsonString);
      json_object_put(jsonObject);
      return -4;
    }

    // first array entry is always the latest commit
    commitObject = json_object_array_get_idx(jsonObject, 0); 

    // attempt to populate latest commit hash, ofc gotta check if it actually exists first cuz who knows 
    if (json_object_object_get_ex(commitObject, "sha", &hashObject)) {
        snprintf(updateData->latestCommit, sizeof(updateData->latestCommit), "%s", json_object_get_string(hashObject));

        updateData->isAvailable = (strcmp(CURRENT_COMMIT_HASH, updateData->latestCommit) != 0); 

        json_object_put(jsonObject);
        if (!updateData->isAvailable) return -3;
    } else {
        json_object_put(jsonObject);
        return -1;
    }
    return 0;
}

int parseCommitsData(char *jsonString) {
    // parse response from github as json object 
    json_object *jsonObject = json_tokener_parse(jsonString);
    json_object *status = NULL;

    // check if we didnt get served garbage data
    if (!jsonObject) {
        outputLog("failure parsing commit data: response - %s", jsonString);
        return -1;
    }

    if (json_object_object_get_ex(jsonObject, "status", &status)) {
      // check what github wants to tell us
      outputLog("status code: %d\n%s\n", json_object_get_int(status), jsonString);
      json_object_put(jsonObject);
      return -4;
    }

    // iterate every commit in the response
    size_t count = json_object_array_length(jsonObject);
    latestCommits = (char **)malloc(count * sizeof(char *));
    for (size_t i = 0; i < count; i++) {
        const json_object *currentCommitObject = json_object_array_get_idx(jsonObject, i);
        json_object *currenthHashObject;
        json_object *commitDataObject;
        json_object *currentTitleObject;

        if(!currentCommitObject) return -2;

        // get commit hash, if equal to current we've hit the currently installed commit so we end parsing here
        if (json_object_object_get_ex(currentCommitObject, "sha", &currenthHashObject)) {
            const char *commitHash = json_object_get_string(currenthHashObject);

            if (commitHash && strcmp(commitHash, CURRENT_COMMIT_HASH) == 0) break;
        }

        // append commit title to the array
        if (json_object_object_get_ex(currentCommitObject, "commit", &commitDataObject) && json_object_object_get_ex(commitDataObject, "message", &currentTitleObject)) {

            const char *commitTitle = json_object_get_string(currentTitleObject);

            if (!commitTitle) return -1;

            // length of dash + space + title
            int len = 1 + 1 + strlen(commitTitle) + 1;

            latestCommits[i] = (char *)malloc(len);

            if (!latestCommits[i]) return -1;

            snprintf(latestCommits[i], len, "- %s", commitTitle);

            latestCommitCount++;
        }
    }
    json_object_put(jsonObject);
    return 0;
}

int parseReleasesData(char *jsonString) {
    // parse response as json string string
    json_object *jsonObject = json_tokener_parse(jsonString);
    json_object *targetRelease = NULL; 
    json_object *url = NULL;
    json_object *size = NULL;
    json_object *status = NULL;

    // check for garbage data
    if (!jsonObject) return -1;

    size_t count = json_object_object_length(jsonObject);

    if (json_object_object_get_ex(jsonObject, "status", &status)) {
      // check what github wants to tell us
      outputLog("status code: %d\n%s\n", json_object_get_int(status), jsonString);
      json_object_put(jsonObject);
      return -4;
    }

        for (size_t i = 0; i < count; i++) {
            json_object *currObject;
            json_object *prerelease = NULL;
            json_object *id = NULL;

            currObject = json_object_array_get_idx(jsonObject, i);

            bool isPrerelease = json_object_object_get_ex(currObject, "prerelease", &prerelease) && json_object_get_boolean(prerelease);
            bool idGreaterThanCurrent = (json_object_object_get_ex(currObject, "id", &id) && json_object_get_int(id) > CURRENT_LATEST_RELEASE_ID);

            if (!isPrerelease && idGreaterThanCurrent) {
                targetRelease = currObject;
                updateData->isAvailable = true;
                break;
            }
        }

    // if a release is found:
    if (targetRelease) {
        json_object *name = NULL;
        json_object *body = NULL;
        json_object *assets = NULL;

        // get release name
        if (json_object_object_get_ex(targetRelease, "name", &name)) snprintf(updateData->releaseTitle, sizeof(updateData->releaseTitle), "%s", json_object_get_string(name));

        // get release body
        if (json_object_object_get_ex(targetRelease, "body", &body)) {
            const char *tmp = json_object_get_string(body);
            updateData->releaseBody = (char *)malloc(strlen(tmp) + 1);
            strcpy(updateData->releaseBody, tmp);
        }

        // get assets object and iterate it looking for either the .3dsx build or .cia bundle depending on what the user is running
        if (json_object_object_get_ex(targetRelease, "assets", &assets)) {
            size_t assetCount = json_object_object_length(assets);
            for (size_t j = 0; j < assetCount; j++) {
                const json_object *currAssetObject = json_object_array_get_idx(assets, j);
                json_object *assetNameObject = NULL;

                if (json_object_object_get_ex(currAssetObject, "name", &assetNameObject)) {
                    const char *currAssetName = json_object_get_string(assetNameObject);
                    if (strstr(currAssetName, is3Dsx ? ".3dsx" : ".cia")) {
                        if (json_object_object_get_ex(currAssetObject, "browser_download_url", &url)) {
                            // we found the freaking download link lets go
                            const char *tempUrl = json_object_get_string(url);
                            snprintf(updateData->releaseDownloadUrl, sizeof(updateData->releaseDownloadUrl), "%s", tempUrl);
                        }
                        if (json_object_object_get_ex(currAssetObject, "size", &size)) {
                            // also get the file size
                            updateData->releaseFileSize = json_object_get_int(size);
                        }
                        json_object_put(jsonObject);
                        return 0;
                    }
                }
            }
            json_object_put(jsonObject);
            return -2;
        }
    } 
    json_object_put(jsonObject);
    return -3;
}

int parseActionsData(char *jsonString) {
    // parse response as json string string
    json_object *jsonObject = json_tokener_parse(jsonString);
    json_object *artifacts = NULL;
    json_object *currObject = NULL;
    json_object *workflow_data_object = NULL;
    json_object *id_object = NULL;
    json_object *status = NULL;
    // check for garbage data
    if (!jsonObject) return -1;
    
    if (json_object_object_get_ex(jsonObject, "status", &status)) {
      // check what github wants to tell us
      outputLog("status code: %d\n%s\n", json_object_get_int(status), jsonString);
      json_object_put(jsonObject);
      return -4;
    }

    if (!json_object_object_get_ex(jsonObject, "artifacts", &artifacts)) return -1; 

    size_t count = json_object_object_length(artifacts); 
    
    for (size_t i = 0; i < count; i++) {
        currObject = json_object_array_get_idx(artifacts, i);
        
        if (!json_object_object_get_ex(currObject, "workflow_run", &workflow_data_object)) continue;
        if (!json_object_object_get_ex(workflow_data_object, "head_sha", &id_object)) continue;
        
        const char *tmpString = json_object_get_string(id_object);

        if (strcmp(tmpString, updateData->latestCommit) != 0) continue;
        
        json_object *artifact_id_object = NULL;
        json_object *expired_object = NULL;

        if(!json_object_object_get_ex(currObject, "expired", &expired_object) && json_object_get_boolean(expired_object)) return -1;
        
        if(!json_object_object_get_ex(currObject, "id", &artifact_id_object)) return -1;
        
        snprintf(updateData->releaseDownloadUrl, sizeof(updateData->releaseDownloadUrl), "https://nightly.link/%s/%s/actions/artifacts/%s.zip", REPO_AUTHOR, REPO_NAME, json_object_get_string(artifact_id_object));
        json_object_put(jsonObject);
        return 0;
    }
    json_object_put(jsonObject);
    return -2;
}

int searchLevelsInternal(GenericTask *task, bool useGdps) {
    char *outdata;
    int result = getSearchResults(task, &outdata, 22, filters, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the search results string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == ':')) return -2;

    int initialStringCount = 0;

    int levelStringCount = 0;
    int creatorStringCount = 0;
    int songStringCount = 0;

    char **initialStrings = splitString(outdata, '#', &initialStringCount, true);
    if (!initialStrings) return -1;

    char **levelsStrings = splitString(initialStrings[0], '|', &levelStringCount, true);
    if (!levelsStrings) return -1;

    char **creatorStrings = splitString(initialStrings[1], '|', &creatorStringCount, true);
    if (!creatorStrings) return -1;
    
    char **songStrings = splitStringStrDel(initialStrings[2], "~:~", &songStringCount, true);
    // if (!songStrings && !filters.mainSong) return -1; 
    
    searchEntries = (SearchEntry *)malloc(levelStringCount * sizeof(SearchEntry));
    if (!searchEntries) return -1;

    creatorEntries = (CreatorEntry *)malloc(creatorStringCount * sizeof(CreatorEntry));
    if (!creatorEntries) return -1;

    if (songStringCount > 0) {
        songEntries = (SongEntry *)malloc(songStringCount * sizeof(SongEntry));
        if (!songEntries) return -1;
    }

    pageEntry = (PageEntry *)malloc(sizeof(SongEntry));
    if (!pageEntry) return -1;

    // Initialize
    memset(searchEntries, 0, levelStringCount * sizeof(SearchEntry));
    memset(creatorEntries, 0, creatorStringCount * sizeof(CreatorEntry));
    if (songStringCount > 0) memset(songEntries, 0, songStringCount * sizeof(SongEntry));
    memset(pageEntry, 0, sizeof(PageEntry));

    // Fill creators
    fillCreatorEntries(creatorStrings, creatorStringCount);

    // Fill songs
    if (songStringCount > 0) fillSongEntries(songStrings, songStringCount);
    
    // Fill levels
    fillLevelEntries(levelsStrings, songStringCount, creatorStringCount, levelStringCount);

    // Fill pages
    fillPageEntry(initialStrings[3]);
   
    freeStringArray(levelsStrings, levelStringCount);
    freeStringArray(creatorStrings, creatorStringCount);
    if (songStringCount > 0) freeStringArray(songStrings, songStringCount);    
    freeStringArray(initialStrings, initialStringCount);

    free(outdata);

    creatorEntriesLength = creatorStringCount;
    songEntriesLength = songStringCount;
    searchEntriesLength = levelStringCount;
    return 0;
}

int getLevelDataInternal(GenericTask *task, int id, bool refresh, int currentId, bool useGdps) {
    char *outdata;
    int result = getLevelFromID(task, &outdata, id, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the level string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == ':')) return -2;

    int initialStringCount = 0;

    char **initialStrings = splitString(outdata, '#', &initialStringCount, true);
    if (!initialStrings) return -1;

    levelEntry = (LevelEntry *)malloc(initialStringCount * sizeof(LevelEntry));
    if (!levelEntry) return -1;

    // Initialize
    memset(levelEntry, 0, initialStringCount * sizeof(LevelEntry));

    fillLevelEntry(initialStrings, 1, refresh, currentId);

    freeStringArray(initialStrings, initialStringCount);

    free(outdata);

    levelEntryLength = initialStringCount;
    return 0;
}

float deriveGJVersion(int version) {
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

int getCommentsInternal(GenericTask *task, int id, int page, int sortType, bool useGdps) {
    char *outdata;
    int result = getCommentsFromID(task, &outdata, id, page, sortType, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the comments string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == '~')) return -2;

    // i dont know why but the 1.9 gdps handles its comments completely differently, very annoying
    if (!useGdps) {
        int commentStringCount = 0;

        char **commentStrings = splitString(outdata, '|', &commentStringCount, true);
        if (!commentStrings)
            return -1;

        commentEntries = (CommentEntry *)malloc(commentStringCount * sizeof(CommentEntry));
        if (!commentEntries)
            return -1;

        // Initialize
        memset(commentEntries, 0, commentStringCount * sizeof(CommentEntry));

        fillCommentEntries(commentStrings, commentStringCount);

        freeStringArray(commentStrings, commentStringCount);

        free(outdata);

        commentEntriesLength = commentStringCount;
    } else {
        // gdps exclusive slop
        int initialStringCount = 0;

        char **initialStrings = splitString(outdata, '#', &initialStringCount, true);
        if (!initialStrings) return -1;

        int commentStringCount = 0;
        int commentAuthorStringCount = 0;

        char **commentStrings = splitString(initialStrings[0], '|', &commentStringCount, true);
        if (!commentStrings) return -1;

        char **commentAuthorStrings = splitString(initialStrings[1], '|', &commentAuthorStringCount, true);
        if (!commentAuthorStrings) return -1;

        commentEntries = (CommentEntry *)malloc(commentStringCount * sizeof(CommentEntry));
        if (!commentEntries) return -1;
        
        commentEntriesLength = commentStringCount;

        // Initialize
        memset(commentEntries, 0, commentStringCount * sizeof(CommentEntry));

        // Fill comment entries
        fillGdpsCommentEntries(commentStrings, commentStringCount);

        // Fill missing creator entries
        fillGdpsCommentAuthorEntries(commentAuthorStrings, commentAuthorStringCount, commentStringCount);

        freeStringArray(commentStrings, commentStringCount);
        freeStringArray(commentAuthorStrings, commentAuthorStringCount);
        freeStringArray(initialStrings, initialStringCount);

    }

    return 0;
}

int getSongDataInternal(GenericTask *task, int songId, SongEntry *targetSongEntry, bool useGdps) {
    char *outdata;
    int result = getSongInfoFromID(task, &outdata, songId, useGdps);

    if (result != 0) return result;
    // validate first two chars of response to make sure what we're parsing is the level string
    if (!(outdata[0] >= '0' && outdata[0] <= '9' && outdata[1] == '~')) return -2;

    int initialStringCount = 0;

    fillSongEntry(outdata, targetSongEntry);

    free(outdata);

    levelEntryLength = initialStringCount;
    return 0;
}

int getLatestNightlyInternal(GenericTask *task, const char *repoAuthor, const char *repoName, const char *repoBranch) {
    char *outdata;
    latestCommitCount = 0;
    int result = getCommits(task, &outdata, repoAuthor, repoName, repoBranch);
    if (result != 0) return result;

    // check for updates
    int parserResult = parseLatestCommit(outdata);
    if (parserResult != 0) {
        free(outdata);
        return parserResult;
    }
    // populate commits since current
    parserResult = parseCommitsData(outdata);
    if (parserResult != 0) {
        free(outdata);
        return parserResult;
    }
    // get repo actions
    result = getActions(task, &outdata, repoAuthor, repoName, is3Dsx ? "gd3ds-3dsx-zip" : "gd3ds-cia-zip");
    if (result != 0) return result;

    // parse actions and get download link
    parserResult = parseActionsData(outdata);
    if (parserResult != 0) {
        free(outdata);
        return parserResult;
    }

    free(outdata);
    return 0;
}

int getLatestReleaseInternal(GenericTask *task, const char *repoAuthor, const char *repoName) {
    char *outdata;
    int result = getReleases(task, &outdata, repoAuthor, repoName);
    if (result != 0) return result;

    int parserResult = parseReleasesData(outdata);
    if (parserResult != 0) return parserResult;

    free(outdata);
    return 0;
}

int checkForUpdatesInternal(GenericTask *task, const char *repoAuthor, const char *repoName, const char *repoBranch, bool useNightly) {
    updateData = (UpdateData *)malloc(sizeof(UpdateData));
    if (!updateData) return -1;

    // Initialize
    memset(updateData, 0, sizeof(UpdateData));

    int result;

    // nightly updates and regular updates behave very differently so we handle both in separate functions
    if (useNightly) {
        result = getLatestNightlyInternal(task, repoAuthor, repoName, repoBranch);
        if (result != 0) return result;
    } else {
        result = getLatestReleaseInternal(task, repoAuthor, repoName);
        if (result != 0) return result;
    }

    if (result != 0) return result; 

    return 0;
}

int extractUpdateInternal(const char *targetPath) {
    int result = extractZip(targetPath, CONFIG_ROOT);
    //clean up regardless of result
    remove(targetPath);
    return result;
}

int installUpdateInternal(const char *updatePath) {
    // 3dsx and cia rom installs are handled very differently 
    
    if (is3Dsx) {
        // ensure 3dsx path is present
        if (!threeDsxPath) return -1;
        
        Result res = 0;

        // unmount romfs so we can modify current .3dsx
        res = romfsExit();
        if(R_FAILED(res)) {
            outputLog("failed to exit romfs - code: %d\n", res);
            return -1;
        }

        // filesystem operations
        if (remove(threeDsxPath) != 0) {
            outputLog("failed to remove current .3dsx\n");
            return -1;
        }
        if (rename(updatePath, threeDsxPath) != 0) {
            outputLog("failed to move target .3dsx.\n");
            return -1;
        }
        // success
        return 0;
    }
    
    Result res = 0;
    FILE *cia = NULL;
    Handle ciaHandle = 0;
    
    // initialise application manager 
    res = amInit();
    if (R_FAILED(res)) {
        outputLog("failed to init application manager - code: %d\n", res);
        return 2;
    }
    
    // open target .cia
    cia = fopen(updatePath, "rb");
    if (!cia) {
        amExit();
        outputLog("failed to open target .cia\n");
        return 1;
    }
    
    // begin .cia install and get write handle
    res = AM_StartCiaInstall(MEDIATYPE_SD, &ciaHandle);
    if (R_FAILED(res)) {
        fclose(cia);
        amExit();
        outputLog("failed to start cia install: %d\n", res);
        return 3;
    }
    
    // write the .cia to handle in 64kb segments
    size_t bufferSize = 1024 * 64;
    u8* buffer = (u8 *)malloc(bufferSize);
    u32 bytesWritten = 0;
    u32 offset = 0;
    size_t bytesRead = 0;
    
    while ((bytesRead = fread(buffer, 1, bufferSize, cia)) > 0)
    {
        res = FSFILE_Write(ciaHandle, &bytesWritten, offset, buffer, bytesRead, FS_WRITE_FLUSH);
        
        if (R_FAILED(res)) {
            AM_CancelCIAInstall(ciaHandle);
            free(buffer);
            fclose(cia);
            amExit();
            outputLog("failed during cia install - code: %d\n", res);
            return 4;
        }
        
        offset += bytesWritten;
    }
    
    // install success, do cleanup
    res = AM_FinishCiaInstall(ciaHandle);
    free(buffer);
    fclose(cia);
    amExit();
    remove(updatePath);
    return 0;
}

//intermediate functions

int searchLevels(GenericTask *task) {
    int result = searchLevelsInternal(task, gdps);
    return result;
}

int getLevel(GenericTask *task) {
    int result = getLevelDataInternal(task, onlineMenuLevelID, refresh, currSearchID, gdps);
    return result;
}

int getComments(GenericTask *task) {
    int result = getCommentsInternal(task, onlineMenuLevelID, currentCommentsPage, commentsSortType, gdps);
    return result;
}

int getSongData(GenericTask *task) {
    int result = getSongDataInternal(task, currentSearchEntry->songId, currentSongEntry, gdps);
    return result;
}

int checkForUpdates(GenericTask *task) {
    int result = checkForUpdatesInternal(task, REPO_AUTHOR, REPO_NAME, REPO_BRANCH, useNightlyBranch);
    return result;
}

int installUpdate(GenericTask *){
    char updatePath[512];
    snprintf(updatePath, sizeof(updatePath), "%sGeometryDash3DS.%s", CONFIG_ROOT, is3Dsx ? "3dsx" : "cia");
    int result = installUpdateInternal(updatePath);
    return result;
}

int extractUpdate(GenericTask *){
    int result = extractUpdateInternal(downloadedUpdateFilePath);
    return result;
}