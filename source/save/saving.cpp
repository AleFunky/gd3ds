#include "saving.hpp"
#include "3ds/thread.h"
#include "json-c/json_object.h"
#include "json-c/json_tokener.h"
#include "json-c/json_types.h"
#include "json-c/json_util.h"
#include "level_loading.hpp"
#include "main.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "save/config.hpp"
#include "utils/folders.hpp"
#include "utils/json_config.hpp"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdint.h>
#include "level/main_levels.hpp"

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "math_helpers.hpp"
#include "utils/server_utils.hpp"
#include "utils/utils.hpp"

LevelDataEntry *currentLevelEntry;
LevelListType currentLevelList = LEVEL_LIST_MAIN_LEVELS;

static CreatorEntry emptyCreator = (CreatorEntry) {
    .userId = 0,
    .creatorName = "-"
};

static SavingTask tasks[SAVE_TYPE_COUNT] = { 0 };
static Thread savingThreads[SAVE_TYPE_COUNT] = { 0 };

int totalStars = 0;
int totalCoins = 0;
int totalUserCoins = 0;
int totalAttempts = 0;
int totalJumps = 0;
int totalDemons = 0;
int completedMainLevels = 0;
int completedExternalLevels = 0;
int playersDestroyed = 0;

uint64_t fnv1a64(const char* str) {
    uint64_t hash = 14695981039346656037ULL;

    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

LevelDataEntry *levelDataListFind(LevelDataList *levelData, const char *key) {
    for (size_t i = 0; i < levelData->count; i++) {
        if (strcmp(levelData->list[i].key, key) == 0) {
            return &levelData->list[i];
        }
    }

    return NULL;
}

SavedLevelDataEntry *savedLevelDataListFind(SavedLevelDataList *levelData, const char *key) {
    for (size_t i = 0; i < levelData->count; i++) {
        if (strcmp(levelData->list[i].key, key) == 0) {
            return &levelData->list[i];
        }
    }

    return NULL;
}

// Those functions check if the old save data is a main level or a gdps main level

bool isGdpsMainLevel(const char *filename) {
    // Thumper to Streetwise
    for (size_t i = 18; i < 23; i++) {
        char file[16];
        snprintf(file, sizeof(file), "main_%d", i);
        char tmp[17];
        snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
        if (strncmp(filename, tmp, 16) == 0) {
            return true;
        }
    }
    return false;
}

bool isMainLevel(const char *filename) {
    for (size_t i = 0; i < 18; i++) {
        char file[16];
        snprintf(file, sizeof(file), "main_%d", i);
        char tmp[17];
        snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
        if (strncmp(filename, tmp, 16) == 0) {
            return true;
        }
    }
    return false;
}

LevelDataEntry *getOnlineLevelData(int levelID) {
    char file[16];
    snprintf(file, sizeof(file), "%d", levelID);
    char tmp[17];
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return levelDataListFind(&currentServerFile->onlineLevels, tmp);
}

SavedLevelDataEntry *getSavedLevelData(int levelID) {
    char file[16];
    snprintf(file, sizeof(file), "%d", levelID);
    char tmp[17]; 
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return savedLevelDataListFind(&currentServerFile->savedLevels, tmp);
}

static void calculateStatsLevelList(LevelDataList *list, bool isMainLevel, const MainLevelPack *pack) {
    for (size_t i = 0; i < list->count; i++) {
        LevelData *data = &list->list[i].data;
        totalAttempts += data->attempts;
        totalJumps += data->jumps;

        if (data->normalProgress == 100) {
            if (isMainLevel) {
                completedMainLevels++;
                totalStars += data->stars;
                totalCoins += data->coin1;
                totalCoins += data->coin2;
                totalCoins += data->coin3;

                // Check for demon
                if (pack->levels[i].difficulty == MAIN_DIFF_DEMON) {
                    totalDemons++;
                }
            } else {
                completedExternalLevels++;
                totalStars += data->stars;

                // Check for demon
                if (data->stars == 10) {
                    totalDemons++;
                }
            }
        }
    }
}

bool levelHasRate(const LevelData *data) {
    return data->stars > 0;
}

static int countListUserCoins(LevelDataList *list) {
    int count = 0;
    for (size_t i = 0; i < list->count; i++) {
        LevelData *data = &list->list[i].data;
        if (data->normalProgress == 100 && levelHasRate(data)) {
            count += data->coin1;
            count += data->coin2;
            count += data->coin3;
        }
    }
    return count;
}

void calculateStats() {
    totalStars = 0;
    totalCoins = 0;
    totalUserCoins = 0;
    totalAttempts = 0;
    totalJumps = 0;
    totalDemons = 0;
    completedMainLevels = 0;
    completedExternalLevels = 0;

    // Calculate stats
    calculateStatsLevelList(&gdServerFile.mainLevels, true, &robtopLevels);
    calculateStatsLevelList(&gdServerFile.onlineLevels, false, NULL);
    
    calculateStatsLevelList(&gdpsFile.mainLevels, true, &gdpsLevels);
    calculateStatsLevelList(&gdpsFile.onlineLevels, false, NULL);

    calculateStatsLevelList(&externalFile.externalLevels, false, NULL);

    totalUserCoins += countListUserCoins(&gdServerFile.onlineLevels);
    totalUserCoins += countListUserCoins(&gdpsFile.onlineLevels);
}

// New save file format

static struct json_object *makeLevelDataListJSON(const LevelDataList *levelData) {
    struct json_object *object = json_object_new_object();
    if (!object) {
        return NULL;
    }

    for (size_t i = 0; i < levelData->count; i++) {
        const LevelDataEntry *entry = &levelData->list[i];

        struct json_object *data = json_object_new_object();
        if (!data) {
            json_object_put(object);
            return NULL;
        }
        
        json_object_object_add(data, "level_id",
            json_object_new_int(entry->data.levelID)
        );

        json_object_object_add(data, "attempts",
            json_object_new_int(entry->data.attempts)
        );

        json_object_object_add(data, "jumps",
            json_object_new_int(entry->data.jumps)
        );

        json_object_object_add(data, "normal_progress",
            json_object_new_int(entry->data.normalProgress)
        );

        json_object_object_add(data, "practice_progress",
            json_object_new_int(entry->data.practiceProgress)
        );

        json_object_object_add(data, "stars",
            json_object_new_int(entry->data.stars)
        );

        json_object_object_add(data, "coin1",
            json_object_new_boolean(entry->data.coin1)
        );

        json_object_object_add(data, "coin2",
            json_object_new_boolean(entry->data.coin2)
        );

        json_object_object_add(data, "coin3",
            json_object_new_boolean(entry->data.coin3)
        );

        json_object_object_add(data, "low_detail",
            json_object_new_boolean(entry->data.lowDetail)
        );

        json_object_object_add(object, entry->key, data);
    }

    return object;
}


static struct json_object *searchEntryToJSON(const SearchEntry *entry) {
    struct json_object *obj = json_object_new_object();
    if (!obj)
        return NULL;

    json_object_object_add(obj, "name",
        json_object_new_string(entry->name)
    );

    json_object_object_add(obj, "description",
        json_object_new_string(entry->description ? entry->description : "")
    );

    json_object_object_add(obj, "levelId",
        json_object_new_int(entry->levelId)
    );

    json_object_object_add(obj, "creatorId",
        json_object_new_int(entry->creatorId)
    );

    json_object_object_add(obj, "songId",
        json_object_new_int(entry->songId)
    );

    json_object_object_add(obj, "mainSongId",
        json_object_new_int(entry->mainSongId)
    );

    json_object_object_add(obj, "lengthNum",
        json_object_new_int(entry->lengthNum)
    );

    json_object_object_add(obj, "downloads",
        json_object_new_int(entry->downloads)
    );

    json_object_object_add(obj, "likes",
        json_object_new_int(entry->likes)
    );

    json_object_object_add(obj, "stars",
        json_object_new_int(entry->stars)
    );

    json_object_object_add(obj, "reqStars",
        json_object_new_int(entry->reqStars)
    );

    json_object_object_add(obj, "difficulty",
        json_object_new_int(entry->difficulty)
    );

    json_object_object_add(obj, "objCount",
        json_object_new_int(entry->objCount)
    );

    json_object_object_add(obj, "levelVersion",
        json_object_new_int(entry->levelVersion)
    );

    json_object_object_add(obj, "gameVersion",
        json_object_new_int(entry->gameVersion)
    );

    json_object_object_add(obj, "epic",
        json_object_new_int(entry->epic)
    );

    json_object_object_add(obj, "featureScore",
        json_object_new_int(entry->featureScore)
    );

    json_object_object_add(obj, "originalId",
        json_object_new_int(entry->originalId)
    );

    json_object_object_add(obj, "isTwoPlayer",
        json_object_new_boolean(entry->isTwoPlayer)
    );

    json_object_object_add(obj, "isDemon",
        json_object_new_boolean(entry->isDemon)
    );

    json_object_object_add(obj, "isAuto",
        json_object_new_boolean(entry->isAuto)
    );

    json_object_object_add(obj, "creatorIndex",
        json_object_new_int(entry->creatorIndex)
    );

    json_object_object_add(obj, "songIndex",
        json_object_new_int(entry->songIndex)
    );
    
    json_object_object_add(obj, "coins",
        json_object_new_int(entry->coins)
    );
    
    json_object_object_add(obj, "verifiedCoins",
        json_object_new_boolean(entry->verifiedCoins)
    );

    return obj;
}

static struct json_object *creatorEntryToJSON(const CreatorEntry *entry) {
    struct json_object *obj = json_object_new_object();
    if (!obj)
        return NULL;

    json_object_object_add(obj, "userId",
        json_object_new_int(entry->userId)
    );

    json_object_object_add(obj, "creatorName",
        json_object_new_string(entry->creatorName)
    );

    json_object_object_add(obj, "accountId",
        json_object_new_int(entry->accountId)
    );

    return obj;
}

static struct json_object *songEntryToJSON(const SongEntry *entry) {
    struct json_object *obj = json_object_new_object();
    if (!obj)
        return NULL;

    json_object_object_add(obj, "ngSongId",
        json_object_new_int(entry->ngSongId)
    );

    json_object_object_add(obj, "songTitle",
        json_object_new_string(entry->songTitle)
    );

    json_object_object_add(obj, "artistName",
        json_object_new_string(entry->artistName)
    );

    json_object_object_add(obj, "songSize",
        json_object_new_double(entry->songSize)
    );

    json_object_object_add(obj, "songLink",
        json_object_new_string(entry->songLink)
    );

    return obj;
}

static struct json_object *levelEntryToJSON(const LevelEntry *entry) {
    struct json_object *obj = json_object_new_object();
    if (!obj)
        return NULL;

    json_object_object_add(obj, "uploadDate",
        json_object_new_string(entry->uploadDate)
    );
    json_object_object_add(obj, "updateDate",
        json_object_new_string(entry->updateDate)
    );

    return obj;
}


static struct json_object *makeSavedLevelDataListJSON(const SavedLevelDataList *levelData) {
    struct json_object *object = json_object_new_object();
    if (!object) {
        return NULL;
    }

    for (size_t i = 0; i < levelData->count; i++) {
        const SavedLevelDataEntry *entry = &levelData->list[i];

        struct json_object *data = json_object_new_object();
        if (!data) {
            json_object_put(object);
            return NULL;
        }

        struct json_object *search = searchEntryToJSON(&entry->searchEntry);
        struct json_object *creator = creatorEntryToJSON(&entry->creatorEntry);
        
        struct json_object *song = NULL;
        struct json_object *level = NULL;
        if (entry->songEntry.ngSongId >= 0) song = songEntryToJSON(&entry->songEntry);
        if (true) level = levelEntryToJSON(&entry->levelEntry);

        if (!search || !creator) {
            if (search) json_object_put(search);
            if (creator) json_object_put(creator);
            if (song) json_object_put(song);
            json_object_put(data);
            json_object_put(object);
            return NULL;
        }  
        json_object_object_add(data, "version", json_object_new_int(entry->version));
        json_object_object_add(data, "search", search);
        json_object_object_add(data, "creator", creator);
        if (song) json_object_object_add(data, "song", song);
        if (level) json_object_object_add(data, "level", level);

        json_object_object_add(object, entry->key, data);
    }

    return object;
}

static void freeLevelDataList(LevelDataList *levelData) {
    if (levelData->list) {
        for (size_t i = 0; i < levelData->count; i++) {
            free(levelData->list[i].key);
        }
    }
    free(levelData->list);
    levelData->list = NULL;
    levelData->count = 0;
    levelData->capacity = 0;
}


static void freeSavedLevelDataList(SavedLevelDataList *levelData) {
    if (levelData->list) {
        for (size_t i = 0; i < levelData->count; i++) {
            free(levelData->list[i].key);
        }
    }
    free(levelData->list);
    levelData->list = NULL;
    levelData->count = 0;
    levelData->capacity = 0;
}

static bool parseLevelDataList(LevelDataList *levelData, struct json_object *object) {
    size_t count = json_object_object_length(object);

    levelData->list = (LevelDataEntry *)calloc(count, sizeof(LevelDataEntry));
    if (!levelData->list) {
        return false;
    }

    levelData->capacity = count;
    levelData->count = 0;

    json_object_object_foreach(object, key, value)
    {
        struct json_object *levelID = NULL;
        struct json_object *attempts = NULL;
        struct json_object *jumps = NULL;
        struct json_object *normalProgress = NULL;
        struct json_object *practiceProgress = NULL;
        struct json_object *stars = NULL;
        struct json_object *coin1 = NULL;
        struct json_object *coin2 = NULL;
        struct json_object *coin3 = NULL;
        struct json_object *lowDetail = NULL;

        LevelDataEntry *entry = &levelData->list[levelData->count];

        entry->key = strdup(key);
        if (!entry->key) {
            freeLevelDataList(levelData);
            return false;
        }

        entry->data.levelID = 0;
        entry->data.attempts = 0;
        entry->data.jumps = 0;
        entry->data.normalProgress = 0;
        entry->data.practiceProgress = 0;
        entry->data.stars = 0;
        entry->data.coin1 = false;
        entry->data.coin2 = false;
        entry->data.coin3 = false;
        entry->data.lowDetail = false;


        if (json_object_object_get_ex(value, "level_id", &levelID))
            entry->data.levelID = json_object_get_int(levelID);

        if (json_object_object_get_ex(value, "attempts", &attempts))
            entry->data.attempts = json_object_get_int(attempts);

        if (json_object_object_get_ex(value, "jumps", &jumps))
            entry->data.jumps = json_object_get_int(jumps);

        if (json_object_object_get_ex(value, "normal_progress", &normalProgress))
            entry->data.normalProgress = json_object_get_int(normalProgress);

        if (json_object_object_get_ex(value, "practice_progress", &practiceProgress))
            entry->data.practiceProgress = json_object_get_int(practiceProgress);

        if (json_object_object_get_ex(value, "stars", &stars))
            entry->data.stars = json_object_get_int(stars);

        if (json_object_object_get_ex(value, "coin1", &coin1))
            entry->data.coin1 = json_object_get_boolean(coin1);

        if (json_object_object_get_ex(value, "coin2", &coin2))
            entry->data.coin2 = json_object_get_boolean(coin2);

        if (json_object_object_get_ex(value, "coin3", &coin3))
            entry->data.coin3 = json_object_get_boolean(coin3);

        if (json_object_object_get_ex(value, "low_detail", &lowDetail))
            entry->data.lowDetail = json_object_get_boolean(lowDetail);

        levelData->count++;

    }

    return true;
}

static bool parseSavedLevelDataList(SavedLevelDataList *levelData, struct json_object *object) {
    size_t count = json_object_object_length(object);

    levelData->list = (SavedLevelDataEntry *)calloc(count, sizeof(SavedLevelDataEntry));
    if (!levelData->list) {
        return false;
    }

    levelData->capacity = count;
    levelData->count = 0;

    json_object_object_foreach(object, key, value)
    {
        struct json_object *version = NULL;
        struct json_object *search = NULL;
        struct json_object *creator = NULL;
        struct json_object *song = NULL;
        struct json_object *level = NULL;

        SavedLevelDataEntry *entry = &levelData->list[levelData->count];

        entry->key = strdup(key);
        if (!entry->key) {
            freeSavedLevelDataList(levelData);
            return false;
        }

        memset(&entry->searchEntry, 0, sizeof(entry->searchEntry));
        memset(&entry->creatorEntry, 0, sizeof(entry->creatorEntry));
        memset(&entry->songEntry, 0, sizeof(entry->songEntry));
        memset(&entry->levelEntry, 0, sizeof(entry->levelEntry));

        if (json_object_object_get_ex(value, "version", &version)) {
            entry->version = json_object_get_int(version);
        } else {
            entry->version = 0;
        }

        // Level entry
        if (json_object_object_get_ex(value, "search", &search)) {
            struct json_object *name = NULL;
            struct json_object *description = NULL;
            struct json_object *levelID = NULL;
            struct json_object *creator_id = NULL;
            struct json_object *songID = NULL;
            struct json_object *main_song_id = NULL;
            struct json_object *length_num = NULL;
            struct json_object *downloads = NULL;
            struct json_object *likes = NULL;
            struct json_object *stars = NULL;
            struct json_object *req_stars = NULL;
            struct json_object *difficulty = NULL;
            struct json_object *objCount = NULL;
            struct json_object *level_version = NULL;
            struct json_object *game_version = NULL;
            struct json_object *epic = NULL;
            struct json_object *feature_score = NULL;
            struct json_object *original_id = NULL;
            struct json_object *is_two_player = NULL;
            struct json_object *is_demon = NULL;
            struct json_object *is_auto = NULL;
            struct json_object *creatorIndex = NULL;
            struct json_object *songIndex = NULL;
            struct json_object *coins = NULL;
            struct json_object *verified_coins = NULL;

            if (json_object_object_get_ex(search, "name", &name))
                snprintf(entry->searchEntry.name, sizeof(entry->searchEntry.name), "%s", json_object_get_string(name));

            if (json_object_object_get_ex(search, "description", &description)) {
                const char *str = json_object_get_string(description);

                entry->searchEntry.description = strdup(str);
                if (!entry->searchEntry.description) {
                    freeSavedLevelDataList(levelData);
                    return false;
                }
            }

            if (json_object_object_get_ex(search, "levelId", &levelID))
                entry->searchEntry.levelId = json_object_get_int(levelID);

            if (json_object_object_get_ex(search, "creatorId", &creator_id))
                entry->searchEntry.creatorId = json_object_get_int(creator_id);

            if (json_object_object_get_ex(search, "songId", &songID))
                entry->searchEntry.songId = json_object_get_int(songID);

            if (json_object_object_get_ex(search, "mainSongId", &main_song_id))
                entry->searchEntry.mainSongId = json_object_get_int(main_song_id);

            if (json_object_object_get_ex(search, "lengthNum", &length_num))
                entry->searchEntry.lengthNum = json_object_get_int(length_num);

            if (json_object_object_get_ex(search, "downloads", &downloads))
                entry->searchEntry.downloads = json_object_get_int(downloads);

            if (json_object_object_get_ex(search, "likes", &likes))
                entry->searchEntry.likes = json_object_get_int(likes);

            if (json_object_object_get_ex(search, "stars", &stars))
                entry->searchEntry.stars = json_object_get_int(stars);

            if (json_object_object_get_ex(search, "reqStars", &req_stars))
                entry->searchEntry.reqStars = json_object_get_int(req_stars);

            if (json_object_object_get_ex(search, "difficulty", &difficulty))
                entry->searchEntry.difficulty = json_object_get_int(difficulty);

            if (json_object_object_get_ex(search, "objCount", &objCount))
                entry->searchEntry.objCount = json_object_get_int(objCount);

            if (json_object_object_get_ex(search, "levelVersion", &level_version))
                entry->searchEntry.levelVersion = json_object_get_int(level_version);

            if (json_object_object_get_ex(search, "gameVersion", &game_version))
                entry->searchEntry.gameVersion = json_object_get_int(game_version);

            if (json_object_object_get_ex(search, "epic", &epic))
                entry->searchEntry.epic = json_object_get_int(epic);

            if (json_object_object_get_ex(search, "featureScore", &feature_score))
                entry->searchEntry.featureScore = json_object_get_int(feature_score);

            if (json_object_object_get_ex(search, "originalId", &original_id))
                entry->searchEntry.originalId = json_object_get_int(original_id);

            if (json_object_object_get_ex(search, "isTwoPlayer", &is_two_player))
                entry->searchEntry.isTwoPlayer = json_object_get_boolean(is_two_player);

            if (json_object_object_get_ex(search, "isDemon", &is_demon))
                entry->searchEntry.isDemon = json_object_get_boolean(is_demon);

            if (json_object_object_get_ex(search, "isAuto", &is_auto))
                entry->searchEntry.isAuto = json_object_get_boolean(is_auto);

            if (json_object_object_get_ex(search, "creatorIndex", &creatorIndex))
                entry->searchEntry.creatorIndex = json_object_get_int(creatorIndex);

            if (json_object_object_get_ex(search, "songIndex", &songIndex))
                entry->searchEntry.songIndex = json_object_get_int(songIndex);

            if (json_object_object_get_ex(search, "coins", &coins))
                entry->searchEntry.coins = json_object_get_int(coins);

            if (json_object_object_get_ex(search, "verifiedCoins", &verified_coins))
                entry->searchEntry.verifiedCoins = json_object_get_boolean(verified_coins);
        }

        // Creator entry
        if (json_object_object_get_ex(value, "creator", &creator)) {
            struct json_object *user_id = NULL;
            struct json_object *creatorName = NULL;
            struct json_object *account_id = NULL;

            if (json_object_object_get_ex(creator, "userId", &user_id))
                entry->creatorEntry.userId = json_object_get_int(user_id);

            if (json_object_object_get_ex(creator, "creatorName", &creatorName))
                snprintf(entry->creatorEntry.creatorName, sizeof(entry->creatorEntry.creatorName), "%s", json_object_get_string(creatorName));

            if (json_object_object_get_ex(creator, "accountId", &account_id))
                entry->creatorEntry.accountId = json_object_get_int(account_id);
        }

        // Song entry
        if (json_object_object_get_ex(value, "song", &song)) {
            struct json_object *ng_song_id = NULL;
            struct json_object *song_title = NULL;
            struct json_object *artist_name = NULL;
            struct json_object *song_size = NULL;
            struct json_object *song_link = NULL;

            if (json_object_object_get_ex(song, "ngSongId", &ng_song_id))
                entry->songEntry.ngSongId = json_object_get_int(ng_song_id);

            if (json_object_object_get_ex(song, "songTitle", &song_title))
                snprintf(entry->songEntry.songTitle, sizeof(entry->songEntry.songTitle), "%s", json_object_get_string(song_title));

            if (json_object_object_get_ex(song, "artistName", &artist_name))
                snprintf(entry->songEntry.artistName, sizeof(entry->songEntry.artistName), "%s", json_object_get_string(artist_name));

            if (json_object_object_get_ex(song, "songSize", &song_size))
                entry->songEntry.songSize = (float)json_object_get_double(song_size);

            if (json_object_object_get_ex(song, "songLink", &song_link))
                snprintf(entry->songEntry.songLink, sizeof(entry->songEntry.songLink), "%s", json_object_get_string(song_link));
        } else {
            entry->songEntry.ngSongId = -1;
        }

        // Level entry
        if (json_object_object_get_ex(value, "level", &level)) {
            struct json_object *upload_date = NULL;
            struct json_object *update_date = NULL;
            if (json_object_object_get_ex(level, "uploadDate", &upload_date))
                snprintf(entry->levelEntry.uploadDate, sizeof(entry->levelEntry.uploadDate), "%s", json_object_get_string(upload_date));

            if (json_object_object_get_ex(level, "updateDate", &update_date))
                snprintf(entry->levelEntry.updateDate, sizeof(entry->levelEntry.updateDate), "%s", json_object_get_string(update_date));
        } else {
            snprintf(entry->levelEntry.uploadDate, sizeof(entry->levelEntry.uploadDate), "%s", "N/A");
            snprintf(entry->levelEntry.updateDate, sizeof(entry->levelEntry.updateDate), "%s", "N/A");
        }


        levelData->count++;
    }

    return true;
}

static bool levelDataListAdd(LevelDataList *levelData, const char *key, const LevelData data) {
    if (levelData->count >= levelData->capacity) {
        size_t newCapacity = levelData->capacity == 0 ? 8 : levelData->capacity + 4;

        LevelDataEntry *newList = (LevelDataEntry *)realloc(levelData->list, newCapacity * sizeof(LevelDataEntry));
        if (!newList) {
            return false;
        }

        levelData->list = newList;
        levelData->capacity = newCapacity;
    }

    LevelDataEntry *entry = &levelData->list[levelData->count];

    entry->key = strdup(key);
    if (!entry->key) {
        return false;
    }

    entry->data = data;

    levelData->count++;

    return true;
}

static void freeSavedLevelDataEntry(SavedLevelDataEntry *entry) {
    if (!entry) return;

    free(entry->searchEntry.description);
}

static bool savedLevelDataListRemove(SavedLevelDataList *levelData, const char *key) {
    if (!levelData->list) return false;

    SavedLevelDataEntry *search = savedLevelDataListFind(levelData, key);
    if (!search) return false;

    for (size_t i = 0; i < levelData->count; i++) {
        SavedLevelDataEntry *entry = &levelData->list[i];
        if (entry == search) {
            freeSavedLevelDataEntry(entry);
            if (i + 1 < levelData->count) {
                int count = levelData->count - i - 1;
                memmove(&levelData->list[i], &levelData->list[i + 1], sizeof(SavedLevelDataEntry) * count);
            }
            levelData->count--;
            return true;
        }
    }
    return false;
}

static bool savedLevelDataListAdd(SavedLevelDataList *levelData, const char *key, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    if (levelData->count >= levelData->capacity) {
        size_t newCapacity = levelData->capacity == 0 ? 8 : levelData->capacity + 4;

        SavedLevelDataEntry *newList = (SavedLevelDataEntry *)realloc(levelData->list, newCapacity * sizeof(SavedLevelDataEntry));
        if (!newList) {
            return false;
        }

        levelData->list = newList;
        levelData->capacity = newCapacity;
    }

    SavedLevelDataEntry *entry = &levelData->list[levelData->count];

    entry->key = strdup(key);
    if (!entry->key) {
        return false;
    }

    entry->version = SAVED_LEVELS_VERSION;
    entry->searchEntry = *search;
    entry->creatorEntry = *creator;
    if (song) {
        entry->songEntry = *song;
    } else {
        // Missing song entry
        entry->songEntry = (SongEntry) { 0 };
        entry->songEntry.ngSongId = -1;
    }

    // Copy here
    if (search->description) {
        entry->searchEntry.description = strdup(search->description);
        if (!entry->key) {
            return false;
        }
    }

    levelData->count++;

    return true;
}

static void serverFileInit(ServerFile *saveData) {
    saveData->onlineLevels.list = NULL;
    saveData->onlineLevels.capacity = 0;
    saveData->onlineLevels.count = 0;

    saveData->mainLevels.list = NULL;
    saveData->mainLevels.capacity = 0;
    saveData->mainLevels.count = 0;

    saveData->savedLevels.list = NULL;
    saveData->savedLevels.capacity = 0;
    saveData->savedLevels.count = 0;
}

static void externalFileInit(ExternalLevelFile *saveData) {
    saveData->externalLevels.list = NULL;
    saveData->externalLevels.capacity = 0;
    saveData->externalLevels.count = 0;
}

static LevelDataEntry *levelDataListGetOrAdd(LevelDataList *levelData, const char *key) {
    LevelDataEntry *entry = levelDataListFind(levelData, key);
    if (entry) {
        return entry;
    }

    LevelData data = {
        .levelID = 0,
        .attempts = 0,
        .jumps = 0,
        .normalProgress = 0,
        .practiceProgress = 0,
        .stars = false,
        .coin1 = false,
        .coin2 = false,
        .coin3 = false,
        .lowDetail = false
    };

    if (!levelDataListAdd(levelData, key, data)) {
        return NULL;
    }

    return &levelData->list[levelData->count - 1];
}

static bool savedDataListAdd(SavedLevelDataList *levelData, const char *key, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    SavedLevelDataEntry *entry = savedLevelDataListFind(levelData, key);

    const CreatorEntry *cEntry;
    if (creator) {
        cEntry = creator;
    } else {
        cEntry = &emptyCreator;
    }

    if (entry) {
        entry->searchEntry = *search;
        entry->creatorEntry = *cEntry;
        entry->songEntry = *song;
        return true;
    }
    
    if (!savedLevelDataListAdd(levelData, key, search, cEntry, song)) {
        return false;
    }

    return true;
}

bool saveLevelToServerFile(ServerFile *saveData, int levelID, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    char file[16];
    snprintf(file, sizeof(file), "%d", levelID);
    char tmp[17];
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return savedDataListAdd(&saveData->savedLevels, tmp, search, creator, song);
}

bool removeSavedLevel(int levelID, bool gdps) {
    char tmp[16];
    char biggerTmp[256];
    snprintf(tmp, sizeof(tmp), "%d", levelID);
    snprintf(biggerTmp, sizeof(biggerTmp), "%016llX", fnv1a64(tmp));
    savedLevelDataListRemove((gdps ? &gdpsFile.savedLevels : &gdServerFile.savedLevels), biggerTmp);


    snprintf(tmp, sizeof(tmp), "%d_%d", levelID, (int) gdps);
    snprintf(biggerTmp, sizeof(biggerTmp), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(tmp));

    char *path = biggerTmp;

    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    if (remove(path) != 0) {
        if (errno == ENOENT)
            return true;

        return false;
    }

    return true;
}

LevelDataEntry *getOrAddLevelToServerFile(ServerFile *saveData, const char *key, LevelListType type) {
    char hash[17];
    snprintf(hash, sizeof(hash), "%016llX", fnv1a64(key));

    switch (type) {
        case LEVEL_LIST_MAIN_LEVELS:
            return levelDataListGetOrAdd(&saveData->mainLevels, hash);
        case LEVEL_LIST_ONLINE:
            return levelDataListGetOrAdd(&saveData->onlineLevels, hash);
        default:
            break;
    }
    return NULL;
}

LevelDataEntry *getOrAddLevelToExternalFile(ExternalLevelFile *saveData, const char *key) {
    char hash[17];
    snprintf(hash, sizeof(hash), "%016llX", fnv1a64(key));

    return levelDataListGetOrAdd(&saveData->externalLevels, hash);
}

bool loadSaveFile(const char *path, ServerFile *saveData) {
    size_t outSize;
    
    // Load savefile
    char *file = readFile(path, &outSize);
    if (!file) {
        serverFileInit(saveData);
        return saveSaveFile(path, saveData);
    }

    size_t outLen;
    int outCode;
    char *decompressed = decompressData((unsigned char *) file, outSize, &outLen, &outCode);
    if (!decompressed) {
        free(file);
        return false;
    }

    free(file);

    struct json_object *root = json_tokener_parse(decompressed);
    if (!root) {
        free(decompressed);
        return false;
    }

    free(decompressed);

    struct json_object *online = NULL;

    if (!json_object_object_get_ex(root, SAVE_ONLINE_KEY, &online)) {
        json_object_put(root);
        return false;
    }

    if (!parseLevelDataList(&saveData->onlineLevels, online)) {
        json_object_put(root);
        return false;
    }

    struct json_object *mainLevels = NULL;

    if (!json_object_object_get_ex(root, SAVE_MAIN_LEVEL_KEY, &mainLevels)) {
        json_object_put(root);
        return false;
    }

    if (!parseLevelDataList(&saveData->mainLevels, mainLevels)) {
        json_object_put(root);
        return false;
    }

    struct json_object *savedLevels = NULL;

    if (json_object_object_get_ex(root, SAVE_SAVED_LEVEL_KEY, &savedLevels)) {
        if (!parseSavedLevelDataList(&saveData->savedLevels, savedLevels)) {
            json_object_put(root);
            return false;
        }
    }

    json_object_put(root);

    return true;
}

bool loadExternalFile(const char *path, ExternalLevelFile *saveData) {
    size_t outSize;
    
    // Load savefile
    char *file = readFile(path, &outSize);
    if (!file) {
        externalFileInit(saveData);
        return saveExternalFile(path, saveData);
    }

    size_t outLen;
    int outCode;
    char *decompressed = decompressData((unsigned char *) file, outSize, &outLen, &outCode);
    if (!decompressed) {
        free(file);
        return false;
    }

    free(file);
    
    struct json_object *root = json_tokener_parse(decompressed);
    if (!root) {
        free(decompressed);
        return false;
    }
    
    free(decompressed);

    struct json_object *external = NULL;

    if (!json_object_object_get_ex(root, SAVE_EXTERNAL_KEY, &external)) {
        json_object_put(root);
        return false;
    }

    if (!parseLevelDataList(&saveData->externalLevels, external)) {
        json_object_put(root);
        return false;
    }

    json_object_put(root);

    return true;
}

SavingError saveSaveFile(const char *path, const ServerFile *saveData) {
    struct json_object *root = json_object_new_object();
    if (!root) {
        return SAVE_ERROR_JSON_FAIL;
    }

    struct json_object *online = makeLevelDataListJSON(&saveData->onlineLevels);
    if (!online) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_ONLINE_KEY, online);

    struct json_object *mainLevels = makeLevelDataListJSON(&saveData->mainLevels);

    if (!mainLevels) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_MAIN_LEVEL_KEY, mainLevels);

    struct json_object *savedLevels = makeSavedLevelDataListJSON(&saveData->savedLevels);

    if (!savedLevels) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_SAVE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_SAVED_LEVEL_KEY, savedLevels);


    SaveType type = (saveData == &gdpsFile ? SAVE_1P9_GDPS : SAVE_ROBTOP);

    SavingTask *task = &tasks[type];

    char tmpPath[250];
    snprintf(tmpPath, sizeof(tmpPath), "%s", path);
    strncpy(task->file, tmpPath, sizeof(task->file));
    stripExtension(tmpPath);

    snprintf(task->tmpFile, sizeof(task->tmpFile), "%s.tmp", tmpPath);

    task->root = root;
    
    beginSaving(type);

    return SAVE_ERROR_NONE;
}

SavingError saveExternalFile(const char *path, const ExternalLevelFile *saveData) {
    struct json_object *root = json_object_new_object();
    if (!root) {
        return SAVE_ERROR_JSON_FAIL;
    }

    struct json_object *external = makeLevelDataListJSON(&saveData->externalLevels);
    if (!external) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_EXTERNAL_KEY, external);

    SavingTask *task = &tasks[SAVE_EXTERNAL];

    char tmpPath[250];
    snprintf(tmpPath, sizeof(tmpPath), "%s", path);
    strncpy(task->file, tmpPath, sizeof(task->file));
    stripExtension(tmpPath);

    snprintf(task->tmpFile, sizeof(task->tmpFile), "%s.tmp", tmpPath);

    task->root = root;
    
    beginSaving(SAVE_EXTERNAL);

    return SAVE_ERROR_NONE;
}

void saveCurrentSaveFile(LevelListType type) {
    const char *path = (gdps ? SAVE_1P9_SERVER_FILE : SAVE_ROBTOP_SERVER_FILE);
    switch (type) {
        case LEVEL_LIST_MAIN_LEVELS:
        case LEVEL_LIST_ONLINE:
            saveSaveFile(path, currentServerFile);
            break;
        case LEVEL_LIST_EXTERNAL:
            saveExternalFile(SAVE_EXTERNAL_LEVELS_FILE, &externalFile);
            break;
    }
}


// Migration

static bool loadOldLevelFile(const char *path, LevelData *data) {
    size_t size;

    char *file = readFile(path, &size);
    if (!file) {
        return false;
    }

    struct json_object *root = json_tokener_parse(file);

    free(file);

    if (!root) {
        return false;
    }

    struct json_object *attempts;
    struct json_object *jumps;
    struct json_object *normal;
    struct json_object *practice;
    struct json_object *stars;
    struct json_object *coin1;
    struct json_object *coin2;
    struct json_object *coin3;

    // Billion keys
    bool valid =
        json_object_object_get_ex(root, "attempts", &attempts) &&
        json_object_object_get_ex(root, "jumps", &jumps) &&
        json_object_object_get_ex(root, "normal", &normal) &&
        json_object_object_get_ex(root, "practice", &practice) &&
        json_object_object_get_ex(root, "stars", &stars) &&
        json_object_object_get_ex(root, "coin1", &coin1) &&
        json_object_object_get_ex(root, "coin2", &coin2) &&
        json_object_object_get_ex(root, "coin3", &coin3);

    if (!valid) {
        json_object_put(root);
        return false;
    }

    data->attempts = json_object_get_int(attempts);
    data->jumps = json_object_get_int(jumps);
    data->normalProgress = json_object_get_int(normal);
    data->practiceProgress = json_object_get_int(practice);
    data->stars = json_object_get_int(stars);
    data->coin1 = json_object_get_boolean(coin1);
    data->coin2 = json_object_get_boolean(coin2);
    data->coin3 = json_object_get_boolean(coin3);
    data->lowDetail = false;

    json_object_put(root);

    return true;
}

static bool oldDataExists() {
    DIR *dir = opendir(DATA_FOLDER);
    if (!dir) {
        return false;
    }

    closedir(dir);
    return true;
}

static bool removeOldData() {
    DIR *dir = opendir(DATA_FOLDER);
    if (!dir) {
        return false;
    }

    struct dirent *entry;

    // Remove data
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[PATH_MAX];

        int written = snprintf(path, sizeof(path), "%s%s", DATA_FOLDER, entry->d_name);

        if (written < 0 || (size_t)written >= sizeof(path)) {
            closedir(dir);
            return false;
        }

        if (unlink(path) != 0) {
            closedir(dir);
            return false;
        }
    }

    closedir(dir);

    // Should be empty now
    if (rmdir(DATA_FOLDER) != 0) {
        return false;
    }

    return true;
}

bool migrateOldData() {
    if (oldDataExists()) {
        DIR *dir = opendir(DATA_FOLDER);
        if (!dir) { // Shouldn't happen i think
            return false;
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            const char *filename = entry->d_name;

            if (strcmp(filename, ".") == 0 ||
                strcmp(filename, "..") == 0) {
                continue;
            }

            const char *extension = strrchr(filename, '.');
            if (!extension || strcmp(extension, ".d") != 0) {
                continue;
            }

            // Everything before .d is the hash
            size_t hashLength = (size_t)(extension - filename);

            char *hash = (char *)malloc(hashLength + 1);
            if (!hash) {
                closedir(dir);
                return false;
            }

            memcpy(hash, filename, hashLength);
            hash[hashLength] = '\0';

            char path[PATH_MAX];

            snprintf(path, sizeof(path), "%s%s", DATA_FOLDER, filename);

            LevelData data;

            if (!loadOldLevelFile(path, &data)) {
                free(hash);
                continue;
            }
            
            bool mainGdpsLevel = isGdpsMainLevel(filename);
            bool mainLevel = isMainLevel(filename);
            if (mainGdpsLevel) {
                // Main gdps levels go to 1p9 gdps server
                if (!levelDataListAdd(&gdpsFile.mainLevels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            } else if (mainLevel) {
                // Main levels go to robtop server
                if (!levelDataListAdd(&gdServerFile.mainLevels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            } else {
                if (!levelDataListAdd(&externalFile.externalLevels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            }

            free(hash);
        }

        closedir(dir);
        
        
        SavingError errorCode = saveSaveFile(SAVE_ROBTOP_SERVER_FILE, &gdServerFile);
        if (errorCode) {
            outputLog("Failed to migrate robtop: %x\n", errorCode);
            return false;
        }

        errorCode = saveSaveFile(SAVE_1P9_SERVER_FILE, &gdpsFile);
        if (errorCode) {
            outputLog("Failed to migrate gdps: %x\n", errorCode);
            return false;
        }
        
        errorCode = saveExternalFile(SAVE_EXTERNAL_LEVELS_FILE, &externalFile);
        if (errorCode) {
            outputLog("Failed to migrate external: %x\n", errorCode);
            return false;
        }

        // Data has been succesfully migrated!
        removeOldData();
        
        return true;
    }

    // Nothing to migrate
    return true;
}

static SavingError compressSave(SavingTask *task) {
    const char *json = json_object_to_json_string_ext(task->root, JSON_C_TO_STRING_PLAIN);
    if (!json) {
        return SAVE_ERROR_OUTOFMEM;
    }

    int outCode;
    task->data = compressData((const unsigned char *) json, strlen(json), &task->dataLen, &outCode);
    if (!task->data) {
        return SAVE_ERROR_COMPRESS;
    }

    return SAVE_ERROR_NONE;
}

static SavingError writeSave(SavingTask *task) {
    FILE *file = fopen(task->tmpFile, "wb");
    if (!file) {
        return SAVE_ERROR_OPEN_FILE;
    }

    bool success = true;

    if (fwrite(task->data, 1, task->dataLen, file) != task->dataLen) {
        success = false;
    }

    if (fclose(file) != 0) {
        success = false;
    }

    if (!success) {
        return SAVE_ERROR_WRITING_FILE;
    }

    if (remove(task->file) != 0) {
        if (errno != ENOENT) {
            outputLog(
                "Failed to remove %s: errno=%d (%s)\n",
                task->file,
                errno,
                strerror(errno)
            );
            remove(task->tmpFile);
            return SAVE_ERROR_REMOVE_FILE;
        }
    }

    int result = rename(task->tmpFile, task->file);
    if (result != 0) {
        outputLog(
            "Failed to rename %s -> %s: result=%d, errno=%d\n",
            task->tmpFile,
            task->file,
            result,
            errno
        );
        remove(task->tmpFile);
        return SAVE_ERROR_RENAME_FILE;
    }

    return SAVE_ERROR_NONE;
}

static void savingThread(void *arg) {
    SavingTask *task = (SavingTask *)arg;

    task->error = compressSave(task);
    json_object_put(task->root);
    task->root = NULL;
    task->ready = true;
}

void processSaving() {
    for (int i = 0; i < (int)SAVE_TYPE_COUNT; i++) {
        SavingTask *task = &tasks[i];
        if (!task->ready) continue;

        SavingError error = task->error ? task->error : writeSave(task);

        free(task->data);
        task->data = NULL;
        task->ready = false;
        task->running = false;

        if (error) {
            outputLog("An error has occured while saving: %d\n", error);
        }
    }
}

bool isSaving() {
    for (int i = 0; i < (int)SAVE_TYPE_COUNT; i++) {
        if (tasks[i].running) return true;
    }
    return false;
}

void waitForSaving() {
    for (int i = 0; i < (int)SAVE_TYPE_COUNT; i++) {
        if (!savingThreads[i]) continue;

        threadJoin(savingThreads[i], U64_MAX);
        savingThreads[i] = NULL;
    }

    processSaving();
}

void beginSaving(SaveType type) {
    if (type == SAVE_CONFIG) {
        configSave(&cfg);
        return;
    }

    if (!tasks[type].running) {   
        int32_t priority = 0x30;
        svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
        priority += 1;
        priority = priority < 0x18 ? 0x18 : priority;
        priority = priority > 0x3F ? 0x3F : priority;

        tasks[type].data = NULL;
        tasks[type].error = SAVE_ERROR_NONE;
        tasks[type].ready = false;
        tasks[type].running = true;
    
        savingThreads[type] = threadCreate(
            savingThread,
            &tasks[type],
            32 * 1024,
            priority,
            (isN3Ds ? 2 : 0),
            false
        );
    }
}

// Offline levels

char *loadSavedLevel(int levelID, bool gdps, size_t *outSize) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", levelID, (int) gdps);
    char path[256];
    snprintf(path, sizeof(path), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(key));

    return readFile(path, outSize);
}

bool saveSavedLevel(int levelID, bool gdps, const char *data) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", levelID, (int) gdps);
    char path[256];
    snprintf(path, sizeof(path), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(key));

    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    size_t len = strlen(data);

    if (fwrite(data, 1, len, file) != len) {
        fclose(file);
        return false;
    }

    if (fclose(file) != 0) {
        return false;
    }

    return true;
}

bool savedLevelExists(int levelID, bool gdps) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", levelID, (int) gdps);
    char path[256];
    snprintf(path, sizeof(path), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(key));

    FILE *file = fopen(path, "rb");

    if (!file) {
        return false;
    }

    fclose(file);
    return true;
}
