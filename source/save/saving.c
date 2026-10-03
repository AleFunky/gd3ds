#include "saving.h"
#include "json-c/json_object.h"
#include "json-c/json_object_iterator.h"
#include "json-c/json_tokener.h"
#include "json-c/json_types.h"
#include "json-c/json_util.h"
#include "level_loading.h"
#include "main.h"
#include "menus/creator_menu/search_menu.h"
#include "save/config.h"
#include "utils/folders.h"
#include "utils/json_config.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdint.h>
#include "level/main_levels.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "math_helpers.h"
#include "utils/server_utils.h"
#include "utils/utils.h"

LevelDataEntry *current_level_entry;

static SavingTask tasks[SAVE_TYPE_COUNT] = { 0 };

int total_stars = 0;
int total_coins = 0;
int total_attempts = 0;
int total_jumps = 0;
int total_demons = 0;
int completed_main_levels = 0;
int completed_external_levels = 0;
int players_destroyed = 0;

uint64_t fnv1a64(const char* str) {
    uint64_t hash = 14695981039346656037ULL;

    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

LevelDataEntry *level_data_list_find(LevelDataList *level_data, const char *key) {
    for (size_t i = 0; i < level_data->count; i++) {
        if (strcmp(level_data->list[i].key, key) == 0) {
            return &level_data->list[i];
        }
    }

    return NULL;
}

SavedLevelDataEntry *saved_level_data_list_find(SavedLevelDataList *level_data, const char *key) {
    for (size_t i = 0; i < level_data->count; i++) {
        if (strcmp(level_data->list[i].key, key) == 0) {
            return &level_data->list[i];
        }
    }

    return NULL;
}

// Those functions check if the old save data is a main level or a gdps main level

bool is_gdps_main_level(const char *filename) {
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

bool is_main_level(const char *filename) {
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

LevelDataEntry *get_online_level_data(int level_id) {
    char file[16];
    snprintf(file, sizeof(file), "%d", level_id);
    char tmp[17];
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return level_data_list_find(&current_server_file->online_levels, tmp);
}

SavedLevelDataEntry *get_saved_level_data(int level_id) {
    char file[16];
    snprintf(file, sizeof(file), "%d", level_id);
    char tmp[17];
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return saved_level_data_list_find(&current_server_file->saved_levels, tmp);
}

static void calculate_stats_level_list(LevelDataList *list, bool is_main_level, const MainLevelPack *pack) {
    for (int i = 0; i < list->count; i++) {
        LevelData *data = &list->list[i].data;
        total_attempts += data->attempts;
        total_jumps += data->jumps;

        if (data->normal_progress == 100) {
            if (is_main_level) {
                completed_main_levels++;
                total_stars += data->stars;
                total_coins += data->coin1;
                total_coins += data->coin2;
                total_coins += data->coin3;

                // Check for demon
                if (pack->levels[i].difficulty == MAIN_DIFF_DEMON) {
                    total_demons++;
                }
            } else {
                completed_external_levels++;
                total_stars += data->stars;

                // Check for demon
                if (data->stars == 10) {
                    total_demons++;
                }
            }
        }
    }
}

void calculate_stats() {
    total_stars = 0;
    total_coins = 0;
    total_attempts = 0;
    total_jumps = 0;
    total_demons = 0;
    completed_main_levels = 0;
    completed_external_levels = 0;

    // Calculate stats
    calculate_stats_level_list(&gd_server_file.main_levels, true, &robtop_levels);
    calculate_stats_level_list(&gd_server_file.online_levels, false, NULL);
    
    calculate_stats_level_list(&gdps_file.main_levels, true, &gdps_levels);
    calculate_stats_level_list(&gdps_file.online_levels, false, NULL);

    calculate_stats_level_list(&external_file.external_levels, false, NULL);
}

// New save file format

static struct json_object *make_level_data_list_json(const LevelDataList *level_data) {
    struct json_object *object = json_object_new_object();
    if (!object) {
        return NULL;
    }

    for (size_t i = 0; i < level_data->count; i++) {
        const LevelDataEntry *entry = &level_data->list[i];

        struct json_object *data = json_object_new_object();
        if (!data) {
            json_object_put(object);
            return NULL;
        }
        
        json_object_object_add(data, "level_id",
            json_object_new_int(entry->data.level_id)
        );

        json_object_object_add(data, "attempts",
            json_object_new_int(entry->data.attempts)
        );

        json_object_object_add(data, "jumps",
            json_object_new_int(entry->data.jumps)
        );

        json_object_object_add(data, "normal_progress",
            json_object_new_int(entry->data.normal_progress)
        );

        json_object_object_add(data, "practice_progress",
            json_object_new_int(entry->data.practice_progress)
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

        json_object_object_add(object, entry->key, data);
    }

    return object;
}


static struct json_object *search_entry_to_json(const SearchEntry *entry) {
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

    return obj;
}

static struct json_object *creator_entry_to_json(const CreatorEntry *entry) {
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

static struct json_object *song_entry_to_json(const SongEntry *entry) {
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

static struct json_object *level_entry_to_json(const LevelEntry *entry) {
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


static struct json_object *make_saved_level_data_list_json(const SavedLevelDataList *level_data) {
    struct json_object *object = json_object_new_object();
    if (!object) {
        return NULL;
    }

    for (size_t i = 0; i < level_data->count; i++) {
        const SavedLevelDataEntry *entry = &level_data->list[i];

        struct json_object *data = json_object_new_object();
        if (!data) {
            json_object_put(object);
            return NULL;
        }

        struct json_object *search = search_entry_to_json(&entry->search_entry);
        struct json_object *creator = creator_entry_to_json(&entry->creator_entry);
        
        struct json_object *song = NULL;
        struct json_object *level = NULL;
        if (entry->song_entry.ngSongId >= 0) song = song_entry_to_json(&entry->song_entry);
        if (true) level = level_entry_to_json(&entry->level_entry);

        if (!search || !creator) {
            if (search) json_object_put(search);
            if (creator) json_object_put(creator);
            if (song) json_object_put(song);
            json_object_put(data);
            json_object_put(object);
            return NULL;
        }

        json_object_object_add(data, "search", search);
        json_object_object_add(data, "creator", creator);
        if (song) json_object_object_add(data, "song", song);
        if (level) json_object_object_add(data, "level", level);

        json_object_object_add(object, entry->key, data);
    }

    return object;
}

static void free_level_data_list(LevelDataList *level_data) {
    if (level_data->list) {
        for (int i = 0; i < level_data->count; i++) {
            free(level_data->list[i].key);
        }
    }
    free(level_data->list);
    level_data->list = NULL;
    level_data->count = 0;
    level_data->capacity = 0;
}


static void free_saved_level_data_list(SavedLevelDataList *level_data) {
    if (level_data->list) {
        for (int i = 0; i < level_data->count; i++) {
            free(level_data->list[i].key);
        }
    }
    free(level_data->list);
    level_data->list = NULL;
    level_data->count = 0;
    level_data->capacity = 0;
}

static bool parse_level_data_list(LevelDataList *level_data, struct json_object *object) {
    size_t count = json_object_object_length(object);

    level_data->list = calloc(count, sizeof(LevelDataEntry));
    if (!level_data->list) {
        return false;
    }

    level_data->capacity = count;
    level_data->count = 0;

    json_object_object_foreach(object, key, value)
    {
        struct json_object *level_id = NULL;
        struct json_object *attempts = NULL;
        struct json_object *jumps = NULL;
        struct json_object *normal_progress = NULL;
        struct json_object *practice_progress = NULL;
        struct json_object *stars = NULL;
        struct json_object *coin1 = NULL;
        struct json_object *coin2 = NULL;
        struct json_object *coin3 = NULL;

        LevelDataEntry *entry = &level_data->list[level_data->count];

        entry->key = strdup(key);
        if (!entry->key) {
            free_level_data_list(level_data);
            return false;
        }

        entry->data.level_id = 0;
        entry->data.attempts = 0;
        entry->data.jumps = 0;
        entry->data.normal_progress = 0;
        entry->data.practice_progress = 0;
        entry->data.stars = 0;
        entry->data.coin1 = false;
        entry->data.coin2 = false;
        entry->data.coin3 = false;


        if (json_object_object_get_ex(value, "level_id", &level_id))
            entry->data.level_id = json_object_get_int(level_id);

        if (json_object_object_get_ex(value, "attempts", &attempts))
            entry->data.attempts = json_object_get_int(attempts);

        if (json_object_object_get_ex(value, "jumps", &jumps))
            entry->data.jumps = json_object_get_int(jumps);

        if (json_object_object_get_ex(value, "normal_progress", &normal_progress))
            entry->data.normal_progress = json_object_get_int(normal_progress);

        if (json_object_object_get_ex(value, "practice_progress", &practice_progress))
            entry->data.practice_progress = json_object_get_int(practice_progress);

        if (json_object_object_get_ex(value, "stars", &stars))
            entry->data.stars = json_object_get_int(stars);

        if (json_object_object_get_ex(value, "coin1", &coin1))
            entry->data.coin1 = json_object_get_boolean(coin1);

        if (json_object_object_get_ex(value, "coin2", &coin2))
            entry->data.coin2 = json_object_get_boolean(coin2);

        if (json_object_object_get_ex(value, "coin3", &coin3))
            entry->data.coin3 = json_object_get_boolean(coin3);

        level_data->count++;

    }

    return true;
}

static bool parse_saved_level_data_list(SavedLevelDataList *level_data, struct json_object *object) {
    size_t count = json_object_object_length(object);

    level_data->list = calloc(count, sizeof(SavedLevelDataEntry));
    if (!level_data->list) {
        return false;
    }

    level_data->capacity = count;
    level_data->count = 0;

    json_object_object_foreach(object, key, value)
    {
        struct json_object *search = NULL;
        struct json_object *creator = NULL;
        struct json_object *song = NULL;
        struct json_object *level = NULL;

        SavedLevelDataEntry *entry = &level_data->list[level_data->count];

        entry->key = strdup(key);
        if (!entry->key) {
            free_saved_level_data_list(level_data);
            return false;
        }

        memset(&entry->search_entry, 0, sizeof(entry->search_entry));
        memset(&entry->creator_entry, 0, sizeof(entry->creator_entry));
        memset(&entry->song_entry, 0, sizeof(entry->song_entry));
        memset(&entry->level_entry, 0, sizeof(entry->level_entry));

        // Level entry
        if (json_object_object_get_ex(value, "search", &search)) {
            struct json_object *name = NULL;
            struct json_object *description = NULL;
            struct json_object *level_id = NULL;
            struct json_object *creator_id = NULL;
            struct json_object *song_id = NULL;
            struct json_object *main_song_id = NULL;
            struct json_object *length_num = NULL;
            struct json_object *downloads = NULL;
            struct json_object *likes = NULL;
            struct json_object *stars = NULL;
            struct json_object *req_stars = NULL;
            struct json_object *difficulty = NULL;
            struct json_object *obj_count = NULL;
            struct json_object *level_version = NULL;
            struct json_object *game_version = NULL;
            struct json_object *epic = NULL;
            struct json_object *feature_score = NULL;
            struct json_object *original_id = NULL;
            struct json_object *is_two_player = NULL;
            struct json_object *is_demon = NULL;
            struct json_object *is_auto = NULL;
            struct json_object *creator_index = NULL;
            struct json_object *song_index = NULL;

            if (json_object_object_get_ex(search, "name", &name))
                snprintf(entry->search_entry.name, sizeof(entry->search_entry.name), "%s", json_object_get_string(name));

            if (json_object_object_get_ex(search, "description", &description)) {
                const char *str = json_object_get_string(description);

                entry->search_entry.description = strdup(str);
                if (!entry->search_entry.description) {
                    free_saved_level_data_list(level_data);
                    return false;
                }
            }

            if (json_object_object_get_ex(search, "levelId", &level_id))
                entry->search_entry.levelId = json_object_get_int(level_id);

            if (json_object_object_get_ex(search, "creatorId", &creator_id))
                entry->search_entry.creatorId = json_object_get_int(creator_id);

            if (json_object_object_get_ex(search, "songId", &song_id))
                entry->search_entry.songId = json_object_get_int(song_id);

            if (json_object_object_get_ex(search, "mainSongId", &main_song_id))
                entry->search_entry.mainSongId = json_object_get_int(main_song_id);

            if (json_object_object_get_ex(search, "lengthNum", &length_num))
                entry->search_entry.lengthNum = json_object_get_int(length_num);

            if (json_object_object_get_ex(search, "downloads", &downloads))
                entry->search_entry.downloads = json_object_get_int(downloads);

            if (json_object_object_get_ex(search, "likes", &likes))
                entry->search_entry.likes = json_object_get_int(likes);

            if (json_object_object_get_ex(search, "stars", &stars))
                entry->search_entry.stars = json_object_get_int(stars);

            if (json_object_object_get_ex(search, "reqStars", &req_stars))
                entry->search_entry.reqStars = json_object_get_int(req_stars);

            if (json_object_object_get_ex(search, "difficulty", &difficulty))
                entry->search_entry.difficulty = json_object_get_int(difficulty);

            if (json_object_object_get_ex(search, "objCount", &obj_count))
                entry->search_entry.objCount = json_object_get_int(obj_count);

            if (json_object_object_get_ex(search, "levelVersion", &level_version))
                entry->search_entry.levelVersion = json_object_get_int(level_version);

            if (json_object_object_get_ex(search, "gameVersion", &game_version))
                entry->search_entry.gameVersion = json_object_get_int(game_version);

            if (json_object_object_get_ex(search, "epic", &epic))
                entry->search_entry.epic = json_object_get_int(epic);

            if (json_object_object_get_ex(search, "featureScore", &feature_score))
                entry->search_entry.featureScore = json_object_get_int(feature_score);

            if (json_object_object_get_ex(search, "originalId", &original_id))
                entry->search_entry.originalId = json_object_get_int(original_id);

            if (json_object_object_get_ex(search, "isTwoPlayer", &is_two_player))
                entry->search_entry.isTwoPlayer = json_object_get_boolean(is_two_player);

            if (json_object_object_get_ex(search, "isDemon", &is_demon))
                entry->search_entry.isDemon = json_object_get_boolean(is_demon);

            if (json_object_object_get_ex(search, "isAuto", &is_auto))
                entry->search_entry.isAuto = json_object_get_boolean(is_auto);

            if (json_object_object_get_ex(search, "creatorIndex", &creator_index))
                entry->search_entry.creatorIndex = json_object_get_int(creator_index);

            if (json_object_object_get_ex(search, "songIndex", &song_index))
                entry->search_entry.songIndex = json_object_get_int(song_index);
        }

        // Creator entry
        if (json_object_object_get_ex(value, "creator", &creator)) {
            struct json_object *user_id = NULL;
            struct json_object *creator_name = NULL;
            struct json_object *account_id = NULL;

            if (json_object_object_get_ex(creator, "userId", &user_id))
                entry->creator_entry.userId = json_object_get_int(user_id);

            if (json_object_object_get_ex(creator, "creatorName", &creator_name))
                snprintf(entry->creator_entry.creatorName, sizeof(entry->creator_entry.creatorName), "%s", json_object_get_string(creator_name));

            if (json_object_object_get_ex(creator, "accountId", &account_id))
                entry->creator_entry.accountId = json_object_get_int(account_id);
        }

        // Song entry
        if (json_object_object_get_ex(value, "song", &song)) {
            struct json_object *ng_song_id = NULL;
            struct json_object *song_title = NULL;
            struct json_object *artist_name = NULL;
            struct json_object *song_size = NULL;
            struct json_object *song_link = NULL;

            if (json_object_object_get_ex(song, "ngSongId", &ng_song_id))
                entry->song_entry.ngSongId = json_object_get_int(ng_song_id);

            if (json_object_object_get_ex(song, "songTitle", &song_title))
                snprintf(entry->song_entry.songTitle, sizeof(entry->song_entry.songTitle), "%s", json_object_get_string(song_title));

            if (json_object_object_get_ex(song, "artistName", &artist_name))
                snprintf(entry->song_entry.artistName, sizeof(entry->song_entry.artistName), "%s", json_object_get_string(artist_name));

            if (json_object_object_get_ex(song, "songSize", &song_size))
                entry->song_entry.songSize = (float)json_object_get_double(song_size);

            if (json_object_object_get_ex(song, "songLink", &song_link))
                snprintf(entry->song_entry.songLink, sizeof(entry->song_entry.songLink), "%s", json_object_get_string(song_link));
        } else {
            entry->song_entry.ngSongId = -1;
        }

        // Level entry
        if (json_object_object_get_ex(value, "level", &level)) {
            struct json_object *upload_date = NULL;
            struct json_object *update_date = NULL;
            if (json_object_object_get_ex(level, "uploadDate", &upload_date))
                snprintf(entry->level_entry.uploadDate, sizeof(entry->level_entry.uploadDate), "%s", json_object_get_string(upload_date));

            if (json_object_object_get_ex(level, "updateDate", &update_date))
                snprintf(entry->level_entry.updateDate, sizeof(entry->level_entry.updateDate), "%s", json_object_get_string(update_date));
        } else {
            snprintf(entry->level_entry.uploadDate, sizeof(entry->level_entry.uploadDate), "%s", "N/A");
            snprintf(entry->level_entry.updateDate, sizeof(entry->level_entry.updateDate), "%s", "N/A");
        }


        level_data->count++;
    }

    return true;
}

static bool level_data_list_add(LevelDataList *level_data, const char *key, const LevelData data) {
    if (level_data->count >= level_data->capacity) {
        size_t new_capacity = level_data->capacity == 0 ? 8 : level_data->capacity + 4;

        LevelDataEntry *new_list = realloc(level_data->list, new_capacity * sizeof(LevelDataEntry));
        if (!new_list) {
            return false;
        }

        level_data->list = new_list;
        level_data->capacity = new_capacity;
    }

    LevelDataEntry *entry = &level_data->list[level_data->count];

    entry->key = strdup(key);
    if (!entry->key) {
        return false;
    }

    entry->data = data;

    level_data->count++;

    return true;
}

static void free_saved_level_data_entry(SavedLevelDataEntry *entry) {
    if (!entry) return;

    free(entry->search_entry.description);
}

static bool saved_level_data_list_remove(SavedLevelDataList *level_data, const char *key) {
    if (!level_data->list) return false;

    SavedLevelDataEntry *search = saved_level_data_list_find(level_data, key);
    if (!search) return false;

    for (size_t i = 0; i < level_data->count; i++) {
        SavedLevelDataEntry *entry = &level_data->list[i];
        if (entry == search) {
            free_saved_level_data_entry(entry);
            if (i + 1 < level_data->count) {
                int count = level_data->count - i - 1;
                memmove(&level_data->list[i], &level_data->list[i + 1], sizeof(SavedLevelDataEntry) * count);
            }
            level_data->count--;
            return true;
        }
    }
    return false;
}

static bool saved_level_data_list_add(SavedLevelDataList *level_data, const char *key, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    if (level_data->count >= level_data->capacity) {
        size_t new_capacity = level_data->capacity == 0 ? 8 : level_data->capacity + 4;

        SavedLevelDataEntry *new_list = realloc(level_data->list, new_capacity * sizeof(SavedLevelDataEntry));
        if (!new_list) {
            return false;
        }

        level_data->list = new_list;
        level_data->capacity = new_capacity;
    }

    SavedLevelDataEntry *entry = &level_data->list[level_data->count];

    entry->key = strdup(key);
    if (!entry->key) {
        return false;
    }

    entry->search_entry = *search;
    entry->creator_entry = *creator;
    if (song) {
        entry->song_entry = *song;
    } else {
        // Missing song entry
        entry->song_entry = (SongEntry) { 0 };
        entry->song_entry.ngSongId = -1;
    }

    // Copy here
    if (search->description) {
        entry->search_entry.description = strdup(search->description);
        if (!entry->key) {
            return false;
        }
    }

    level_data->count++;

    return true;
}

static void server_file_init(ServerFile *save_data) {
    save_data->online_levels.list = NULL;
    save_data->online_levels.capacity = 0;
    save_data->online_levels.count = 0;

    save_data->main_levels.list = NULL;
    save_data->main_levels.capacity = 0;
    save_data->main_levels.count = 0;

    save_data->saved_levels.list = NULL;
    save_data->saved_levels.capacity = 0;
    save_data->saved_levels.count = 0;
}

static void external_file_init(ExternalLevelFile *save_data) {
    save_data->external_levels.list = NULL;
    save_data->external_levels.capacity = 0;
    save_data->external_levels.count = 0;
}

static LevelDataEntry *level_data_list_get_or_add(LevelDataList *level_data, const char *key) {
    LevelDataEntry *entry = level_data_list_find(level_data, key);
    if (entry) {
        return entry;
    }

    LevelData data = {
        .level_id = 0,
        .attempts = 0,
        .jumps = 0,
        .normal_progress = 0,
        .practice_progress = 0,
        .stars = false,
        .coin1 = false,
        .coin2 = false,
        .coin3 = false
    };

    if (!level_data_list_add(level_data, key, data)) {
        return NULL;
    }

    return &level_data->list[level_data->count - 1];
}

static bool saved_data_list_add(SavedLevelDataList *level_data, const char *key, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    SavedLevelDataEntry *entry = saved_level_data_list_find(level_data, key);
    if (entry) {
        return true;
    }
    
    if (!saved_level_data_list_add(level_data, key, search, creator, song)) {
        return false;
    }

    return true;
}

bool save_level_to_server_file(ServerFile *save_data, int level_id, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song) {
    char file[16];
    snprintf(file, sizeof(file), "%d", level_id);
    char tmp[17];
    snprintf(tmp, sizeof(tmp), "%016llX", fnv1a64(file));
    return saved_data_list_add(&save_data->saved_levels, tmp, search, creator, song);
}

bool remove_saved_level(int level_id, bool gdps) {
    char tmp[16];
    char bigger_tmp[256];
    snprintf(tmp, sizeof(tmp), "%d", level_id);
    snprintf(bigger_tmp, sizeof(bigger_tmp), "%016llX", fnv1a64(tmp));
    saved_level_data_list_remove((gdps ? &gdps_file.saved_levels : &gd_server_file.saved_levels), bigger_tmp);


    snprintf(tmp, sizeof(tmp), "%d_%d", level_id, (int) gdps);
    snprintf(bigger_tmp, sizeof(bigger_tmp), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(tmp));

    char *path = bigger_tmp;

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

LevelDataEntry *get_or_add_level_to_server_file(ServerFile *save_data, const char *key, LevelListType type) {
    char hash[17];
    snprintf(hash, sizeof(hash), "%016llX", fnv1a64(key));

    switch (type) {
        case LEVEL_LIST_MAIN_LEVELS:
            return level_data_list_get_or_add(&save_data->main_levels, hash);
        case LEVEL_LIST_ONLINE:
            return level_data_list_get_or_add(&save_data->online_levels, hash);
        default:
            break;
    }
    return NULL;
}

LevelDataEntry *get_or_add_level_to_external_file(ExternalLevelFile *save_data, const char *key) {
    char hash[17];
    snprintf(hash, sizeof(hash), "%016llX", fnv1a64(key));

    return level_data_list_get_or_add(&save_data->external_levels, hash);
}

bool load_save_file(const char *path, ServerFile *save_data) {
    size_t out_size;
    
    // Load savefile
    char *file = read_file(path, &out_size);
    if (!file) {
        server_file_init(save_data);
        return save_save_file(path, save_data);
    }

    size_t out_len;
    int out_code;
    char *decompressed = decompress_data((unsigned char *) file, out_size, &out_len, &out_code);
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

    if (!parse_level_data_list(&save_data->online_levels, online)) {
        json_object_put(root);
        return false;
    }

    struct json_object *main_levels = NULL;

    if (!json_object_object_get_ex(root, SAVE_MAIN_LEVEL_KEY, &main_levels)) {
        json_object_put(root);
        return false;
    }

    if (!parse_level_data_list(&save_data->main_levels, main_levels)) {
        json_object_put(root);
        return false;
    }

    struct json_object *saved_levels = NULL;

    if (json_object_object_get_ex(root, SAVE_SAVED_LEVEL_KEY, &saved_levels)) {
        if (!parse_saved_level_data_list(&save_data->saved_levels, saved_levels)) {
            json_object_put(root);
            return false;
        }
    }

    json_object_put(root);

    return true;
}

bool load_external_file(const char *path, ExternalLevelFile *save_data) {
    size_t out_size;
    
    // Load savefile
    char *file = read_file(path, &out_size);
    if (!file) {
        external_file_init(save_data);
        return save_external_file(path, save_data);
    }

    size_t out_len;
    int out_code;
    char *decompressed = decompress_data((unsigned char *) file, out_size, &out_len, &out_code);
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

    if (!parse_level_data_list(&save_data->external_levels, external)) {
        json_object_put(root);
        return false;
    }

    json_object_put(root);

    return true;
}

SavingError save_save_file(const char *path, const ServerFile *save_data) {
    struct json_object *root = json_object_new_object();
    if (!root) {
        return SAVE_ERROR_JSON_FAIL;
    }

    struct json_object *online = make_level_data_list_json(&save_data->online_levels);
    if (!online) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_ONLINE_KEY, online);

    struct json_object *main_levels = make_level_data_list_json(&save_data->main_levels);

    if (!main_levels) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_MAIN_LEVEL_KEY, main_levels);

    struct json_object *saved_levels = make_saved_level_data_list_json(&save_data->saved_levels);

    if (!saved_levels) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_SAVE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_SAVED_LEVEL_KEY, saved_levels);

    const char *json = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);

    SaveType type = (save_data == &gdps_file ? SAVE_1P9_GDPS : SAVE_ROBTOP);

    SavingTask *task = &tasks[type];

    char tmp_path[250];
    snprintf(tmp_path, sizeof(tmp_path), "%s", path);
    strncpy(task->file, tmp_path, sizeof(task->file));
    strip_extension(tmp_path);

    snprintf(task->tmp_file, sizeof(task->tmp_file), "%s.tmp", tmp_path);

    task->root = root;
    task->data = strdup(json);
    
    begin_saving(type);

    return SAVE_ERROR_NONE;
}

SavingError save_external_file(const char *path, const ExternalLevelFile *save_data) {
    struct json_object *root = json_object_new_object();
    if (!root) {
        return SAVE_ERROR_JSON_FAIL;
    }

    struct json_object *external = make_level_data_list_json(&save_data->external_levels);
    if (!external) {
        json_object_put(root);
        return SAVE_ERROR_MAKE_DATA_LIST;
    }

    json_object_object_add(root, SAVE_EXTERNAL_KEY, external);

    const char *json = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);

    SavingTask *task = &tasks[SAVE_EXTERNAL];

    char tmp_path[250];
    snprintf(tmp_path, sizeof(tmp_path), "%s", path);
    strncpy(task->file, tmp_path, sizeof(task->file));
    strip_extension(tmp_path);

    snprintf(task->tmp_file, sizeof(task->tmp_file), "%s.tmp", tmp_path);

    task->root = root;
    task->data = strdup(json);
    
    begin_saving(SAVE_EXTERNAL);

    return SAVE_ERROR_NONE;
}

void save_current_save_file(LevelListType type) {
    const char *path = (gdps ? SAVE_1P9_SERVER_FILE : SAVE_ROBTOP_SERVER_FILE);
    switch (type) {
        case LEVEL_LIST_MAIN_LEVELS:
        case LEVEL_LIST_ONLINE:
            save_save_file(path, current_server_file);
            break;
        case LEVEL_LIST_EXTERNAL:
            save_external_file(SAVE_EXTERNAL_LEVELS_FILE, &external_file);
            break;
    }
}


// Migration

static bool load_old_level_file(const char *path, LevelData *data) {
    size_t size;

    char *file = read_file(path, &size);
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
    data->normal_progress = json_object_get_int(normal);
    data->practice_progress = json_object_get_int(practice);
    data->stars = json_object_get_int(stars);
    data->coin1 = json_object_get_boolean(coin1);
    data->coin2 = json_object_get_boolean(coin2);
    data->coin3 = json_object_get_boolean(coin3);

    json_object_put(root);

    return true;
}

static bool old_data_exists() {
    DIR *dir = opendir(DATA_FOLDER);
    if (!dir) {
        return false;
    }

    closedir(dir);
    return true;
}

static bool remove_old_data() {
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

bool migrate_old_data() {
    if (old_data_exists()) {
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
            size_t hash_length = (size_t)(extension - filename);

            char *hash = malloc(hash_length + 1);
            if (!hash) {
                closedir(dir);
                return false;
            }

            memcpy(hash, filename, hash_length);
            hash[hash_length] = '\0';

            char path[PATH_MAX];

            snprintf(path, sizeof(path), "%s%s", DATA_FOLDER, filename);

            LevelData data;

            if (!load_old_level_file(path, &data)) {
                free(hash);
                continue;
            }
            
            bool main_gdps_level = is_gdps_main_level(filename);
            bool main_level = is_main_level(filename);
            if (main_gdps_level) {
                // Main gdps levels go to 1p9 gdps server
                if (!level_data_list_add(&gdps_file.main_levels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            } else if (main_level) {
                // Main levels go to robtop server
                if (!level_data_list_add(&gd_server_file.main_levels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            } else {
                if (!level_data_list_add(&external_file.external_levels, hash, data)) {
                    free(hash);
                    closedir(dir);
                    return false;
                }
            }

            free(hash);
        }

        closedir(dir);
        
        
        SavingError error_code = save_save_file(SAVE_ROBTOP_SERVER_FILE, &gd_server_file);
        if (error_code) {
            output_log("Failed to migrate robtop: %x\n", error_code);
            return false;
        }

        error_code = save_save_file(SAVE_1P9_SERVER_FILE, &gdps_file);
        if (error_code) {
            output_log("Failed to migrate gdps: %x\n", error_code);
            return false;
        }
        
        error_code = save_external_file(SAVE_EXTERNAL_LEVELS_FILE, &external_file);
        if (error_code) {
            output_log("Failed to migrate external: %x\n", error_code);
            return false;
        }

        // Data has been succesfully migrated!
        remove_old_data();
        
        return true;
    }

    // Nothing to migrate
    return true;
}

static SavingError threaded_save(SavingTask *task) {
    if (!task->data) {
        return SAVE_ERROR_DATA;
    }

    const char *json = task->data;
    size_t out_len;
    int out_code;
    unsigned char *compressed = compress_data((const unsigned char *) json, strlen(json), &out_len, &out_code);
    if (!compressed) {
        return SAVE_ERROR_COMPRESS;
    }

    FILE *file = fopen(task->tmp_file, "wb");
    if (!file) {
        return SAVE_ERROR_OPEN_FILE;
    }

    bool success = true;

    if (fwrite(compressed, 1, out_len, file) != out_len) {
        success = false;
    }

    if (fclose(file) != 0) {
        success = false;
    }
    
    free(compressed);
    
    if (success) {
        if (remove(task->file) != 0) {
            if (errno != ENOENT) {
                output_log(
                    "Failed to remove %s: errno=%d (%s)\n",
                    task->file,
                    errno,
                    strerror(errno)
                );
                remove(task->tmp_file);
                return SAVE_ERROR_REMOVE_FILE;
            }
        }

        int result = rename(task->tmp_file, task->file);
        if (result != 0) {
            output_log(
                "Failed to rename %s -> %s: result=%d, errno=%d\n",
                task->tmp_file,
                task->file,
                result,
                errno
            );
            remove(task->tmp_file);
            return SAVE_ERROR_RENAME_FILE;
        }

        return SAVE_ERROR_NONE;
    } else {
        return SAVE_ERROR_WRITING_FILE;
    } 
}

static void saving_thread(void *arg) {
    SavingTask *task = arg;
    SavingError error = SAVE_ERROR_NONE;

    switch (task->type) {
        case SAVE_ROBTOP:
        case SAVE_1P9_GDPS:
        case SAVE_EXTERNAL:
            error = threaded_save(task);
            json_object_put(task->root);
            break;
        case SAVE_CONFIG:
            config_save(&cfg);
            break;
        default: break;
    }

    if (error) {
        output_log("An error has occured while saving: %d\n", error);
    }

    free((void *) task->data);
    task->running = false;
}

bool is_saving() {
    for (int i = 0; i < SAVE_TYPE_COUNT; i++) {
        if (tasks[i].running) return true;
    }
    return false;
}

void begin_saving(SaveType type) {
    if (!tasks[type].running) {   
        int32_t priority = 0x30;
        svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
        priority += 1;
        priority = priority < 0x18 ? 0x18 : priority;
        priority = priority > 0x3F ? 0x3F : priority;

        tasks[type].type = type;
        tasks[type].running = true;
    
        threadCreate(
            saving_thread,
            &tasks[type],
            32 * 1024,
            priority,
            (is_N3DS ? 2 : 0),
            true
        );
    }
}

// Offline levels

char *load_saved_level(int level_id, bool gdps, size_t *out_size) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", level_id, (int) gdps);
    char path[256];
    snprintf(path, sizeof(path), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(key));

    return read_file(path, out_size);
}

bool save_saved_level(int level_id, bool gdps, const char *data) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", level_id, (int) gdps);
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

bool saved_level_exists(int level_id, bool gdps) {
    char key[16];
    snprintf(key, sizeof(key), "%d_%d", level_id, (int) gdps);
    char path[256];
    snprintf(path, sizeof(path), "%s/%llu.saved", SAVED_LEVELS_DIR, fnv1a64(key));

    FILE *file = fopen(path, "rb");

    if (!file) {
        return false;
    }

    fclose(file);
    return true;
}
