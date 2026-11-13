#pragma once

#include <stdbool.h>
#include "level/main_levels.hpp"
#include "utils/server_utils.hpp"

#define DATA_ATTEMPTS "attempts"
#define DATA_JUMPS "jumps"
#define DATA_NORMAL "normal"
#define DATA_PRACTICE "practice"
#define DATA_COIN1 "coin1"
#define DATA_COIN2 "coin2"
#define DATA_COIN3 "coin3"
#define DATA_STARS "stars"

#define SAVE_ROBTOP_SERVER_FILE (CONFIG_ROOT "gdservers.dat")
#define SAVE_1P9_SERVER_FILE (CONFIG_ROOT "1p9gdps.dat")
#define SAVE_EXTERNAL_LEVELS_FILE (CONFIG_ROOT "external.dat")

#define SAVE_ONLINE_KEY "online"
#define SAVE_MAIN_LEVEL_KEY "main_levels"
#define SAVE_EXTERNAL_KEY "external_levels"
#define SAVE_SAVED_LEVEL_KEY "saved_levels"

typedef struct LevelData {
    int levelID;
    int attempts;
    int jumps;
    int normalProgress;
    int practiceProgress;
    int stars;
    bool coin1;
    bool coin2;
    bool coin3;
    bool lowDetail;
} LevelData;

bool levelHasRate(const LevelData *data);

typedef struct SavedLevelDataEntry {
    char *key;
    int version;
    SearchEntry searchEntry;
    CreatorEntry creatorEntry;
    SongEntry songEntry;
    LevelEntry levelEntry;
} SavedLevelDataEntry;

typedef struct SavedLevelDataList {
    SavedLevelDataEntry *list;
    size_t capacity;
    size_t count;
} SavedLevelDataList;

typedef struct LevelDataEntry {
    char *key;
    LevelData data;
} LevelDataEntry;

typedef struct LevelDataList {
    LevelDataEntry *list;
    size_t capacity;
    size_t count;
} LevelDataList;

typedef struct ServerFile {
    LevelDataList onlineLevels;
    LevelDataList mainLevels;
    SavedLevelDataList savedLevels;
} ServerFile;

typedef struct ExternalLevelFile {
    LevelDataList externalLevels;  
} ExternalLevelFile;

typedef enum {
    LEVEL_LIST_MAIN_LEVELS,
    LEVEL_LIST_EXTERNAL,
    LEVEL_LIST_ONLINE,
} LevelListType;

typedef enum {
    SAVE_ROBTOP,
    SAVE_1P9_GDPS,
    SAVE_EXTERNAL,
    SAVE_CONFIG,
    SAVE_TYPE_COUNT,
} SaveType;

typedef enum {
    SAVE_ERROR_NONE,
    SAVE_ERROR_JSON_FAIL,
    SAVE_ERROR_MAKE_DATA_LIST,
    SAVE_ERROR_DECOMPRESS,
    SAVE_ERROR_COMPRESS,
    SAVE_ERROR_OPEN_FILE,
    SAVE_ERROR_WRITING_FILE,
    SAVE_ERROR_REMOVE_FILE,
    SAVE_ERROR_RENAME_FILE,
    SAVE_ERROR_DATA,
    SAVE_ERROR_MAKE_SAVE_DATA_LIST,
    SAVE_ERROR_OUTOFMEM,
} SavingError;

typedef struct SavingTask {
    struct json_object *root;
    unsigned char *data;
    size_t dataLen;
    SavingError error;
    char file[256];
    char tmpFile[256];
    volatile bool ready;
    volatile bool running;
} SavingTask;

void beginSaving(SaveType type);
void processSaving();
bool isSaving();
void waitForSaving();

extern int totalStars;
extern int totalCoins;
extern int totalUserCoins;
extern int totalAttempts;
extern int totalJumps;
extern int totalDemons;
extern int completedMainLevels;
extern int completedExternalLevels;
extern int playersDestroyed;

bool loadExternalFile(const char *path, ExternalLevelFile *saveData);
SavingError saveExternalFile(const char *path, const ExternalLevelFile *saveData);

bool loadSaveFile(const char *path, ServerFile *saveData);
SavingError saveSaveFile(const char *path, const ServerFile *saveData);

LevelDataEntry *getOrAddLevelToExternalFile(ExternalLevelFile *saveData, const char *key);
LevelDataEntry *getOrAddLevelToServerFile(ServerFile *saveData, const char *key, LevelListType type);

extern LevelDataEntry *currentLevelEntry;
extern LevelListType currentLevelList;

bool migrateOldData();

LevelDataEntry *levelDataListFind(LevelDataList *levelData, const char *key);
LevelDataEntry *getOnlineLevelData(int levelID);
SavedLevelDataEntry *savedLevelDataListFind(SavedLevelDataList *levelData, const char *key);
SavedLevelDataEntry *getSavedLevelData(int levelID);
bool saveLevelToServerFile(ServerFile *saveData, int levelID, const SearchEntry *search, const CreatorEntry *creator, const SongEntry *song);
void saveCurrentSaveFile(LevelListType type);

bool removeSavedLevel(int levelID, bool gdps);

bool savedLevelExists(int levelID, bool gdps);
char *loadSavedLevel(int levelID, bool gdps, size_t *outSize);
bool saveSavedLevel(int levelID, bool gdps, const char *data);

void calculateStats();
