#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include "level_loading.hpp"
#include <zlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "arena.hpp"
#include "practice.hpp"
#include "triggers.hpp"
#include "main.hpp"
#include "objects.hpp"
#include "groups.hpp"
#include "mp3_player.hpp"
#include "graphics.hpp"
#include "fonts/level_fonts.hpp"
#include "fonts/bigFont.hpp"
#include "math_helpers.hpp"
#include "particles/object_particles.hpp"
#include "utils/json_config.hpp"

#include "utils/server_utils.hpp"
#include "utils/string_helpers.hpp"

#include "player/collision.hpp"
#include "utils/utils.hpp"

#include "menus/creator_menu/online/online_level_menu.hpp"
#include "save/saving.hpp"

ObjectsArray objects = { 0 };

Section emptySection = { 0 };

Section *sectionHash[SECTION_HASH_SIZE] = {0};

static int coinCount;
static int coinIds[3];

int channelCount = 0;
GDColorChannel *colorChannels = NULL;

LoadedLevelInfo levelInfo;

char *currLevelString = NULL;

TriggerPool colPool;
TriggerPool movePool;
static TriggerPool countPool;
static TriggerPool instantCountPool;
TriggerPool followPlayerYPool;
static TriggerPool followPool;
static TriggerPool counterLabelPool;
static TriggerPool pickupPool;
bool hasFollowPlayerY = false;
TriggerPool alphaPool;
TriggerPool pulsePool;
TriggerPool togglePool;
TriggerPool touchPool;
TriggerPool spawnPool;
TriggerPool rotatePool;
static TriggerPool stopPool;
static TriggerPool onDeathPool;
TriggerPool simplePool;
static TriggerPool textPool;
static TextObject emptyTextObject;

ColorTrigger *getColorTrigger(int obj) {
    return TRIGGER_AT(colPool, ColorTrigger, objects.triggerIndex[obj]);
}

PulseTrigger *getPulseTrigger(int obj) {
    return TRIGGER_AT(pulsePool, PulseTrigger, objects.triggerIndex[obj]);
}

AlphaTrigger *getAlphaTrigger(int obj) {
    return TRIGGER_AT(alphaPool, AlphaTrigger, objects.triggerIndex[obj]);
}

MoveTrigger *getMoveTrigger(int obj) {
    return TRIGGER_AT(movePool, MoveTrigger, objects.triggerIndex[obj]);
}

CountTrigger *getCountTrigger(int obj) {
    return TRIGGER_AT(countPool, CountTrigger, objects.triggerIndex[obj]);
}

InstantCountTrigger *getInstantCountTrigger(int obj) {
    return TRIGGER_AT(instantCountPool, InstantCountTrigger, objects.triggerIndex[obj]);
}

CounterLabel *getCounterLabel(int obj) {
    int index = objects.triggerIndex[obj];
    if (index < 0 || !counterLabelPool.data || (size_t)index >= counterLabelPool.count) return NULL;
    return TRIGGER_AT(counterLabelPool, CounterLabel, index);
}

PickupItem *getPickupItem(int obj) {
    if (!(objects.flags[obj] & FLAG_PICKUP)) return NULL;
    int index = objects.triggerIndex[obj];
    if (index < 0 || !pickupPool.data || (size_t)index >= pickupPool.count) return NULL;
    return TRIGGER_AT(pickupPool, PickupItem, index);
}

static PickupItem *ensurePickupItem(int obj) {
    if (objects.flags[obj] & FLAG_PICKUP) return getPickupItem(obj);
    int index = triggerPoolAdd(&pickupPool, sizeof(PickupItem));
    if (index < 0) return NULL;
    objects.triggerIndex[obj] = index;
    objects.flags[obj] |= FLAG_PICKUP;
    return TRIGGER_AT(pickupPool, PickupItem, index);
}

FollowPlayerYTrigger *getFollowPlayerYTrigger(int obj) {
    return TRIGGER_AT(followPlayerYPool, FollowPlayerYTrigger, objects.triggerIndex[obj]);
}

FollowTrigger *getFollowTrigger(int obj) {
    return TRIGGER_AT(followPool, FollowTrigger, objects.triggerIndex[obj]);
}

RotateTrigger *getRotateTrigger(int obj) {
    return TRIGGER_AT(rotatePool, RotateTrigger, objects.triggerIndex[obj]);
}

ToggleTrigger *getToggleTrigger(int obj) {
    return TRIGGER_AT(togglePool, ToggleTrigger, objects.triggerIndex[obj]);
}

TouchTrigger *getTouchTrigger(int obj) {
    return TRIGGER_AT(touchPool, TouchTrigger, objects.triggerIndex[obj]);
}

SpawnTrigger *getSpawnTrigger(int obj) {
    return TRIGGER_AT(spawnPool, SpawnTrigger, objects.triggerIndex[obj]);
}

StopTrigger *getStopTrigger(int obj) {
    return TRIGGER_AT(stopPool, StopTrigger, objects.triggerIndex[obj]);
}

OnDeathTrigger *getOnDeathTrigger(int obj) {
    return TRIGGER_AT(onDeathPool, OnDeathTrigger, objects.triggerIndex[obj]);
}

SimpleTrigger *getSimpleTrigger(int obj) {
    return TRIGGER_AT(simplePool, SimpleTrigger, objects.triggerIndex[obj]);
}

TextObject *getTextObject(int obj) {
    int index = objects.triggerIndex[obj];
    if (index < 0 || !textPool.data || (size_t)index >= textPool.count) {
        return &emptyTextObject;
    }
    return TRIGGER_AT(textPool, TextObject, index);
}

const char *levelLengths[] = {
    "Tiny",
    "Short",
    "Medium",
    "Long",
    "XL",
};

static inline unsigned int sectionHashFunc(unsigned int x, unsigned int y) {
    return ((unsigned int)x * 73856093u ^ (unsigned int)y * 19349663u) & (SECTION_HASH_SIZE - 1);
}

Section *getSection(int x, int y) {
    unsigned int h = sectionHashFunc(x, y);
    Section *sec = sectionHash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    return &emptySection;
}

static SectionTriggers uncachedSectionTriggers = { .count = -1 };

static void appendSectionTrigger(Section *sec, int objectIndex) {
    if (!isTriggerObject(objects.id[sec->objects[objectIndex]])) return;
    SectionTriggers *list = sec->triggers;
    if (list && list->count < 0) return;
    if (!list || list->count == list->capacity) {
        int capacity = list ? list->capacity * 2 : 4;
        int count = list ? list->count : 0;
        SectionTriggers *grown = (SectionTriggers *)realloc(list, sizeof(*list) + sizeof(int) * capacity);
        if (!grown) {
            free(list);
            sec->triggers = &uncachedSectionTriggers;
            return;
        }
        grown->count = count;
        grown->capacity = capacity;
        sec->triggers = list = grown;
    }
    list->indices[list->count++] = objectIndex;
}

static void removeSectionTrigger(Section *sec, int index, int lastIndex) {
    SectionTriggers *list = sec->triggers;
    if (!list || list->count < 0) return;
    for (int i = 0; i < list->count; i++) {
        if (list->indices[i] != index) continue;
        list->count--;
        memmove(list->indices + i, list->indices + i + 1,
            sizeof(int) * (list->count - i));
        break;
    }
    if (list->count && list->indices[list->count - 1] == lastIndex) {
        int i = --list->count;
        while (i && list->indices[i - 1] > index) {
            list->indices[i] = list->indices[i - 1];
            i--;
        }
        list->indices[i] = index;
        list->count++;
    }
}

Section *getOrCreateSection(int x, int y) {
    unsigned int h = sectionHashFunc(x, y);
    Section *sec = sectionHash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    sec = (Section *)malloc(sizeof(Section));
    if (!sec) return NULL;

    sec->objects = (int *)malloc(sizeof(int) * 8);
    if (!sec->objects) return NULL;

    sec->objectCount = 0;
    sec->objectCapacity = 8;
    sec->triggers = NULL;
    sec->x = x;
    sec->y = y;
    sec->next = sectionHash[h];
    sectionHash[h] = sec;
    return sec;
}

void freeSections(void) {
    for (int i = 0; i < SECTION_HASH_SIZE; i++) {
        Section *sec = sectionHash[i];
        while (sec) {
            Section *next = sec->next;
            free(sec->objects);
            if (sec->triggers && sec->triggers->count >= 0) free(sec->triggers);
            free(sec);
            sec = next;
        }
        sectionHash[i] = NULL;
    }
}

void assignObjectToSection(int obj) {
    int sx = (int)(objects.x[obj] / SECTION_SIZE);
    int sy = (int)(objects.y[obj] / SECTION_SIZE);
    Section *sec = getOrCreateSection(sx, sy);
    if (!sec) return;

    if (sec->objectCount >= sec->objectCapacity) {
        sec->objectCapacity *= 2;
        sec->objects = (int *)realloc(sec->objects, sizeof(int) * sec->objectCapacity);
        if (!sec->objects) return;
    }
    sec->objects[sec->objectCount++] = obj;
    appendSectionTrigger(sec, sec->objectCount - 1);
    objects.sectionX[obj] = sx;
    objects.sectionY[obj] = sy;
}

void removeObjectFromSection(Section *sec, int obj) {
    for (int i = 0; i < sec->objectCount; i++) {
        if (sec->objects[i] == obj) {
            removeSectionTrigger(sec, i, sec->objectCount - 1);
            sec->objects[i] = sec->objects[--sec->objectCount];
            break;
        }
    }
}

void updateObjectSection(int obj) {
    int newSx = (int)(objects.x[obj] / SECTION_SIZE);
    int newSy = (int)(objects.y[obj] / SECTION_SIZE);
    int oldSx = objects.sectionX[obj];
    int oldSy = objects.sectionY[obj];

    if (newSx == oldSx && newSy == oldSy) return;

    Section *oldSec = getSection(oldSx, oldSy);

    if (oldSec) {
        removeObjectFromSection(oldSec, obj);
    }

    assignObjectToSection(obj);
}

char *readFile(const char *filepath, size_t *outSize) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        outputLog("Failed to open file: %s (%d)\n", filepath, errno);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        outputLog("Failed to ftell file: %s (%d)\n", filepath, errno);
        return NULL;
    }
    rewind(f);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        outputLog("Failed to allocate file\n");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Null-terminate for text files
    fclose(f);

    if (outSize) *outSize = size;
    return buffer;
}

char *readFileArena(Arena *arena, const char *filepath, size_t *outSize) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        outputLog("Failed to open file: %s (%d)\n", filepath, errno);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        outputLog("Failed to ftell file: %s (%d)\n", filepath, errno);
        return NULL;
    }
    rewind(f);

    char *buffer = (char *)arenaAlloc(arena, size + 1);
    if (!buffer) {
        outputLog("Failed to allocate file\n");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Null-terminate for text files
    fclose(f);

    if (outSize) *outSize = size;
    return buffer;
}

char *extractGMDKey(const char *data, const char *key, const char *type) {
    char keyTag[32];
    snprintf(keyTag, sizeof(keyTag), "<k>%s</k>", key);
    
    char *keyPos = strstr(data, keyTag);
    if (!keyPos) {
        return NULL;
    }

    // Move past the key tag
    char *start = keyPos + strlen(keyTag);

    // Skip whitespace (spaces, tabs, newlines, etc.)
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char typeStartTag[16];
    snprintf(typeStartTag, sizeof(typeStartTag), "<%s>", type);

    // Confirm that the type start tag is here
    if (strncmp(start, typeStartTag, strlen(typeStartTag)) != 0) {
        outputLog("Expected start tag '%s' not found after key\n", typeStartTag);
        return NULL;
    }

    // Move past the type start tag
    start += strlen(typeStartTag);

    // Find the end tag
    char typeEndTag[16];
    snprintf(typeEndTag, sizeof(typeEndTag), "</%s>", type);
    char *end = strstr(start, typeEndTag);
    if (!end) {
        outputLog("Could not find end tag '%s'\n", typeEndTag);
        return NULL;
    }

    // Allocate and copy value
    int len = end - start;
    char *value = (char *)malloc(len + 1);
    if (!value) {
        outputLog("malloc for gmd key %s failed\n", key);
        return NULL;
    }
    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

bool isAscii(const unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        if (data[i] > 0x7F) {
            return false; // Non-ASCII byte found
        }
    }
    return true;
}

int b64Char(char c) {
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

void fixBase64URL(char *b64) {
    for (int i = 0; b64[i]; i++) {
        if (b64[i] == '-') b64[i] = '+';
        else if (b64[i] == '_') b64[i] = '/';
    }
}

int base64Decode(const char *in, unsigned char *out) {
    int len = 0;
    for (int i = 0; in[i] && in[i+1] && in[i+2] && in[i+3]; i += 4) {
        int a = b64Char(in[i]);
        int b = b64Char(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64Char(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64Char(in[i+3]);

        if (a == -1 || b == -1 || c == -1 || d == -1) {
            return -1;
        }

        out[len++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[len++] = (b << 4) | (c >> 2);
        if (in[i+3] != '=') out[len++] = (c << 6) | d;
    }
    return len;
}


char *getMetadataValue(const char *levelString, const char *key) {
    if (!levelString || !key) return NULL;

    // Find the first semicolon, which separates metadata from objects
    const char *end = strchr(levelString, ';');
    if (!end) return NULL;

    // We'll scan only the metadata portion
    size_t metadataLen = end - levelString;
    char *metadata = (char *)malloc(metadataLen + 1);
    if (!metadata) return NULL;

    strncpy(metadata, levelString, metadataLen);
    metadata[metadataLen] = '\0';

    // Tokenize metadata by comma
    char *token = strtok(metadata, ",");
    while (token) {
        if (strcmp(token, key) == 0) {
            char *value = strtok(NULL, ",");  // get value after key
            if (!value) break;

            // Copy and return value
            char *result = strdup(value);
            free(metadata);
            return result;
        }
        token = strtok(NULL, ",");
    }
    
    free(metadata);
    return NULL;
}

char *decompressOnlineLevel(char *data, int *outCode) {
    printf("Loading level data...\n");

    fixBase64URL(data);

    // Bro theres some levels that randomly have , at the end of the base64 string wtf
    removeChar(data, ',');

    //output_log("BRO len %d lol %s\n", strlen(data), data);

    unsigned char *decoded = (unsigned char *)arenaAlloc(&levelArena, strlen(data));

    if (!decoded) {
        *outCode = LOAD_OUT_OF_MEMORY;
        return NULL;
    }
    
    int decodedLen = base64Decode(data, decoded);
    if (decodedLen <= 0) {
        outputLog("Failed to decode base64\n");
        *outCode = LOAD_INVALID_BASE64;
        return NULL;
    }

    size_t decompressedLen;
    char *decompressed = decompressDataLevel(decoded, decodedLen, &decompressedLen, outCode);
    if (!decompressed) {
        outputLog("Decompression failed (check zlib error above)\n");
        return NULL;
    }
    
    return decompressed;
}

char *decompressLevel(char *data, int *outCode) {
    printf("Loading level data...\n");
    
    char *b64 = extractGMDKey((const char *) data, "k4", "s");
    if (!b64) {
        // Empty level
        char *temp = strdup(data);
        if (!temp) {
            *outCode = LOAD_OUT_OF_MEMORY;
        }
        return temp;
    }

    const char *semicolon = strchr(b64, ';');
    if (semicolon) return b64; // Rare uncompressed gmd found

    fixBase64URL(b64);

    unsigned char *decoded = (unsigned char *)arenaAlloc(&levelArena, strlen(b64));
    if (!decoded) {
        *outCode = LOAD_OUT_OF_MEMORY;
        return NULL;
    }
    int decodedLen = base64Decode(b64, decoded);
    if (decodedLen <= 0) {
        outputLog("Failed to decode base64\n");
        free(b64);
        *outCode = LOAD_OUT_OF_MEMORY;
        return NULL;
    }

    size_t decompressedLen;
    char *decompressed = decompressDataLevel(decoded, decodedLen, &decompressedLen, outCode);
    if (!decompressed) {
        outputLog("Decompression failed (check zlib error above)\n");
        free(b64);
        return NULL;
    }

    free(b64);
    
    return decompressed;
}

char **splitStringStrDel(const char *str, const char *delimiter, int *outCount, bool ignoreZeroLength) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;
    size_t delimiterLen = strlen(delimiter);

    while (*ptr) {
        if (strncmp(ptr, delimiter, delimiterLen) == 0) {
            int len = ptr - start;

            if (len > (ignoreZeroLength ? -1 : 0)) {
                char *token = (char *)malloc(len + 1);
                if (!token) return NULL;
                strncpy(token, start, len);
                token[len] = '\0';
                result = (char **)realloc(result, sizeof(char *) * (count + 1));
                if (!result) return NULL;
                result[count++] = token;
            }

            ptr += delimiterLen;
            start = ptr;
            continue;
        }
        ptr++;
    }

    if (ptr > start) {
        int len = ptr - start;
        if (len > (ignoreZeroLength ? -1 : 0)) {
            char *token = (char *)malloc(len + 1);
            if (!token) return NULL;
            strncpy(token, start, len);
            token[len] = '\0';
            result = (char **)realloc(result, sizeof(char *) * (count + 1));
            if (!result) return NULL;
            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

char **splitString(const char *str, char delimiter, int *outCount, bool ignoreZeroLength) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr == delimiter) {
            int len = ptr - start;
            if (len > (ignoreZeroLength ? -1 : 0)) {
                char *token = (char *)malloc(len + 1);
                if (!token) return NULL;

                strncpy(token, start, len);
                token[len] = '\0';

                result = (char **)realloc(result, sizeof(char*) * (count + 1));
                if (!result) return NULL;

                result[count++] = token;
            }
            start = ptr + 1;
        }
        ptr++;
    }
    if (ptr > start) {
        int len = ptr - start;
        if (len > (ignoreZeroLength ? -1 : 0)) {
            char *token = (char *)malloc(len + 1);
            if (!token) return NULL;

            strncpy(token, start, len);
            token[len] = '\0';

            result = (char **)realloc(result, sizeof(char*) * (count + 1));
            if (!result) return NULL;

            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

void freeStringArray(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

void parseInts(short *intArray, const char *string) {
    int count = 0;
    char **ints = splitString(string, '.', &count, false);

    for (int j = 0; j < MAX_GROUPS_PER_OBJECT; j++) {
        if (j < count) intArray[j] = (short) atoi(ints[j]);
        else intArray[j] = 0;
    }

    freeStringArray(ints, count);
}

HSV parseHSVString(const char *string) {
    int count = 0;
    char **hsvString = splitString(string, 'a', &count, false);
    HSV obtained = { 0 };

    if (count >= 5) {
        obtained.h = atoi(hsvString[0]);
        obtained.s = atof(hsvString[1]);
        obtained.v = atof(hsvString[2]);
        obtained.sChecked = parseBool(hsvString[3]);
        obtained.vChecked = parseBool(hsvString[4]);
    }

    freeStringArray(hsvString, count);
    return obtained;
}

void parseColorChannel(GDColorChannel *channels, int i, char *channelString) {
    GDColorChannel channel = {0};  // Zero-initialize
    channel.fromOpacity = 1.0f;
    int kvCount = 0;
    char **kvs = splitString(channelString, '_', &kvCount, false);

    for (int j = 0; j + 1 < kvCount; j += 2) {
        int key = atoi(kvs[j]);
        const char *valStr = kvs[j + 1];

        switch (key) {
            case 1:  channel.fromRed = atoi(valStr); break;
            case 2:  channel.fromGreen = atoi(valStr); break;
            case 3:  channel.fromBlue = atoi(valStr); break;
            case 4:  channel.playerColor = atoi(valStr); break;
            case 5:  channel.blending = atoi(valStr) != 0; break;
            case 6:  channel.channelID = atoi(valStr); break;
            case 7:  channel.fromOpacity = atof(valStr); break;
            case 8:  channel.toggleOpacity = atoi(valStr) != 0; break;
            case 9:  channel.inheritedChannelID = atoi(valStr); break;
            case 10: channel.hsv = parseHSVString(valStr); break;
            case 11: channel.toRed = atoi(valStr); break;
            case 12: channel.toGreen = atoi(valStr); break;
            case 13: channel.toBlue = atoi(valStr); break;
            case 14: channel.deltaTime = atof(valStr); break;
            case 15: channel.toOpacity = atof(valStr); break;
            case 16: channel.duration = atof(valStr); break;
            case 17: channel.copyOpacity = atoi(valStr) != 0; break;
        }
    }

    channels[i] = channel;
    freeStringArray(kvs, kvCount);
}

int parseOldChannels(char *levelString, GDColorChannel **outArray, int *outCode) {
    GDColorChannel *channels = (GDColorChannel *)malloc(sizeof(GDColorChannel) * 2);
    if (!channels) {
        outputLog("Couldn't alloc initial pre 2.0 color channels\n");
        *outCode = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    char *v19Bg = getMetadataValue(levelString, "kS29");

    int i = 0;

    if (v19Bg) { // 1.9 only
        parseColorChannel(channels, i, v19Bg);
        channels[i].channelID = CHANNEL_BG;
        i++;
        free(v19Bg);

        char *ground = getMetadataValue(levelString, "kS30");
        if (ground) {
            parseColorChannel(channels, i, ground);
            channels[i].channelID = CHANNEL_GROUND;
            i++;
            free(ground);
        }

        char *line = getMetadataValue(levelString, "kS31");
        if (line) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, line);
            channels[i].channelID = CHANNEL_LINE;
            i++;
            free(line);
        }
        
        char *obj = getMetadataValue(levelString, "kS32");
        if (obj) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, obj);
            channels[i].channelID = CHANNEL_OBJ;
            i++;
            free(obj);
        }

        char *col1 = getMetadataValue(levelString, "kS33");
        if (col1) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, col1);
            channels[i].channelID = 1;
            i++;
            free(col1);
        }

        char *col2 = getMetadataValue(levelString, "kS34");
        if (col2) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, col2);
            channels[i].channelID = 2;
            i++;
            free(col2);
        }

        char *col3 = getMetadataValue(levelString, "kS35");
        if (col3) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, col3);
            channels[i].channelID = 3;
            i++;
            free(col3);
        }

        char *col4 = getMetadataValue(levelString, "kS36");
        if (col4) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, col4);
            channels[i].channelID = 4;
            i++;
            free(col4);
        }

        char *dl3 = getMetadataValue(levelString, "kS37");
        if (dl3) {
            channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
            if (!channels) {
                *outCode = LOAD_OUT_OF_MEMORY;
                return 0;
            }
            parseColorChannel(channels, i, dl3);
            channels[i].channelID = CHANNEL_3DL;
            i++;
            free(dl3);
        }
        
        *outArray = channels;
        return i;
    }

    // Pre 1.9

    char *bgR = getMetadataValue(levelString, "kS1");
    char *bgG = getMetadataValue(levelString, "kS2");
    char *bgB = getMetadataValue(levelString, "kS3");
    
    if (bgR && bgG && bgB) {
        GDColorChannel bgChannel = {0};
        bgChannel.fromOpacity = 1.0f;
        bgChannel.channelID = CHANNEL_BG;
        bgChannel.fromRed = atoi(bgR);
        bgChannel.fromGreen = atoi(bgG);
        bgChannel.fromBlue = atoi(bgB);

        char *bgPlayerColor = getMetadataValue(levelString, "kS16");
        if (bgPlayerColor) {
            bgChannel.playerColor = atoi(bgPlayerColor);
            free(bgPlayerColor);
        }

        channels[i] = bgChannel;
        i++;
    }

    free(bgR);
    free(bgG);
    free(bgB);

    char *gR = getMetadataValue(levelString, "kS4");
    char *gG = getMetadataValue(levelString, "kS5");
    char *gB = getMetadataValue(levelString, "kS6");

    if (gR && gG && gB) {
        GDColorChannel gChannel = {0};
        gChannel.fromOpacity = 1.0f;
        gChannel.channelID = CHANNEL_GROUND;
        gChannel.fromRed = atoi(gR);
        gChannel.fromGreen = atoi(gG);
        gChannel.fromBlue = atoi(gB);

        char *gPlayerColor = getMetadataValue(levelString, "kS17");
        if (gPlayerColor) {
            gChannel.playerColor = atoi(gPlayerColor);
            free(gPlayerColor);
        }
        channels[i] = gChannel;
        i++;
    }

    free(gR);
    free(gG);
    free(gB);

    char *lineR = getMetadataValue(levelString, "kS7");
    char *lineG = getMetadataValue(levelString, "kS8");
    char *lineB = getMetadataValue(levelString, "kS9");

    if (lineR && lineG && lineB) {
        GDColorChannel lineChannel = {0};
        lineChannel.fromOpacity = 1.0f;
        lineChannel.channelID = CHANNEL_LINE;
        lineChannel.fromRed = atoi(lineR);
        lineChannel.fromGreen = atoi(lineG);
        lineChannel.fromBlue = atoi(lineB);
        
        char *linePlayerColor = getMetadataValue(levelString, "kS18");
        if (linePlayerColor) {
            lineChannel.playerColor = atoi(linePlayerColor);
            free(linePlayerColor);
        }

        channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
        if (!channels) {
            *outCode = LOAD_OUT_OF_MEMORY;
            return 0;
        }
        channels[i] = lineChannel;
        i++;
    }
    
    free(lineR);
    free(lineG);
    free(lineB);

    char *objR = getMetadataValue(levelString, "kS10");
    char *objG = getMetadataValue(levelString, "kS11");
    char *objB = getMetadataValue(levelString, "kS12");

    if (objR && objG && objB) {
        GDColorChannel objChannel = {0};
        objChannel.fromOpacity = 1.0f;
        objChannel.channelID = CHANNEL_OBJ;
        objChannel.fromRed = atoi(objR);
        objChannel.fromGreen = atoi(objG);
        objChannel.fromBlue = atoi(objB);

        char *objPlayerColor = getMetadataValue(levelString, "kS19");
        if (objPlayerColor) {
            objChannel.playerColor = atoi(objPlayerColor);
            free(objPlayerColor);
        }
        
        channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
        if (!channels) {
            *outCode = LOAD_OUT_OF_MEMORY;
            return 0;
        }
        channels[i] = objChannel;
        i++;
    }

    free(objR);
    free(objG);
    free(objB);

    char *obj2R = getMetadataValue(levelString, "kS13");
    char *obj2G = getMetadataValue(levelString, "kS14");
    char *obj2B = getMetadataValue(levelString, "kS15");

    if (obj2R && obj2G && obj2B) {
        GDColorChannel obj2Channel = {0};
        obj2Channel.fromOpacity = 1.0f;
        obj2Channel.channelID = 1;
        obj2Channel.fromRed = atoi(obj2R);
        obj2Channel.fromGreen = atoi(obj2G);
        obj2Channel.fromBlue = atoi(obj2B);
        
        char *obj2PlayerColor = getMetadataValue(levelString, "kS20");
        if (obj2PlayerColor) {
            obj2Channel.playerColor = atoi(obj2PlayerColor);
            free(obj2PlayerColor);
        }

        char *obj2Blending = getMetadataValue(levelString, "kA5");
        if (obj2Blending) {
            obj2Channel.blending = atoi(obj2Blending) != 0;
            free(obj2Blending);
        }

        channels = (GDColorChannel *)realloc(channels, sizeof(GDColorChannel) * (i + 1));
        if (!channels) {
            *outCode = LOAD_OUT_OF_MEMORY;
            return 0;
        }
        channels[i] = obj2Channel;
        i++;
    }
    
    free(obj2R);
    free(obj2G);
    free(obj2B);

    *outArray = channels;

    return i;
}

int parseColorChannels(const char *colorString, GDColorChannel **outArray, int *outCode) {
    if (!colorString || !outArray) return 0;

    int count = 0;
    // Split string into each channel
    char **entries = splitString(colorString, '|', &count, false);
    if (!entries) {
        *outCode = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    GDColorChannel *channels = (GDColorChannel *)malloc(sizeof(GDColorChannel) * count);
    if (!channels) {
        outputLog("Couldn't alloc color channels\n");
        freeStringArray(entries, count);
        *outCode = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    for (int i = 0; i < count; i++) {
        parseColorChannel(channels, i, entries[i]);
    }

    *outArray = channels;
    freeStringArray(entries, count);
    return count;
}

GDValueType getValueTypeForKey(int key) {
    switch (key) {
        case 1:  return GD_VAL_INT;    // Object ID
        case 2:  return GD_VAL_FLOAT;  // X Position
        case 3:  return GD_VAL_FLOAT;  // Y Position
        case 4:  return GD_VAL_BOOL;   // Flipped Horizontally
        case 5:  return GD_VAL_BOOL;   // Flipped Vertically
        case 6:  return GD_VAL_FLOAT;  // Rotation
        case 7:  return GD_VAL_INT;    // (Color/Pulse trigger) Red
        case 8:  return GD_VAL_INT;    // (Color/Pulse trigger) Green
        case 9:  return GD_VAL_INT;    // (Color/Pulse trigger) Blue
        case 10: return GD_VAL_FLOAT;  // (Color trigger) Duration
        case 11: return GD_VAL_BOOL;   // (Triggers) Touch Triggered
        case 14: return GD_VAL_BOOL;   // (Color trigger) Tint ground
        case 15: return GD_VAL_BOOL;   // (Color trigger) Player 1 color
        case 16: return GD_VAL_BOOL;   // (Color trigger) Player 2 color
        case 17: return GD_VAL_BOOL;   // (Color trigger) Blending
        case 19: return GD_VAL_INT;    // 1.9 color channel
        case 21: return GD_VAL_INT;    // Main col channel
        case 22: return GD_VAL_INT;    // Detail col channel
        case 23: return GD_VAL_INT;    // (Color trigger) Target color ID
        case 24: return GD_VAL_INT;    // Zlayer
        case 25: return GD_VAL_INT;    // Zorder
        case 28: return GD_VAL_FLOAT;  // move offset X
        case 29: return GD_VAL_FLOAT;  // move offset Y
        case 30: return GD_VAL_INT;    // move easing
        case 31: return GD_VAL_STRING; // (Text object) Text
        case 32: return GD_VAL_FLOAT;  // scale (uniform)
        case 35: return GD_VAL_FLOAT;  // opacity (alpha trigger)
        case 41: return GD_VAL_BOOL;   // main_col_HSV_enabled
        case 42: return GD_VAL_BOOL;   // detail_col_HSV_enabled
        case 43: return GD_VAL_HSV;    // main_col_HSV
        case 44: return GD_VAL_HSV;    // detail_col_HSV
        case 45: return GD_VAL_FLOAT;  // pulse fade_in
        case 46: return GD_VAL_FLOAT;  // pulse hold
        case 47: return GD_VAL_FLOAT;  // pulse fade_out
        case 48: return GD_VAL_INT;    // pulse_mode
        case 49: return GD_VAL_HSV;    // copied_hsv
        case 50: return GD_VAL_INT;    // copied_color_id
        case 51: return GD_VAL_INT;    // target group (triggers)
        case 52: return GD_VAL_INT;    // pulse_target_type
        case 54: return GD_VAL_FLOAT;  // teleport portal Y offset
        case 56: return GD_VAL_BOOL;   // activate group (toggle trigger)
        case 58: return GD_VAL_BOOL;   // lock to player X (move trigger)
        case 59: return GD_VAL_BOOL;   // lock to player Y (move trigger)
        case 62: return GD_VAL_BOOL;   // spawn_triggered
        case 63: return GD_VAL_FLOAT;  // spawn_delay
        case 64: return GD_VAL_BOOL;   // dont fade
        case 65: return GD_VAL_BOOL;   // pulse main_only
        case 66: return GD_VAL_BOOL;   // pulse detail_only
        case 67: return GD_VAL_BOOL;   // dont enter
        case 68: return GD_VAL_FLOAT;  // rotate degrees
        case 69: return GD_VAL_INT;    // rotate times360
        case 70: return GD_VAL_BOOL;   // rotate lock object rotation
        case 71: return GD_VAL_INT;    // rotate center group
        case 72: return GD_VAL_FLOAT;  // follow x mod
        case 73: return GD_VAL_FLOAT;  // follow y mod
        case 77: return GD_VAL_INT;    // target count (count/instant count)
        case 78: return GD_VAL_BOOL;   // sub count (collectible, 2.1)
        case 79: return GD_VAL_BOOL;   // pickup item (2.1)
        case 80: return GD_VAL_INT;    // item ID (pickup/count/counter label)
        case 81: return GD_VAL_INT;    // hold mode (touch trigger)
        case 82: return GD_VAL_INT;    // on/off (touch trigger)
        case 85: return GD_VAL_FLOAT;  // easing rate
        case 89: return GD_VAL_INT;    // dual mode (touch trigger)
        case 87: return GD_VAL_BOOL;   // multi_triggered
        case 88: return GD_VAL_INT;    // instant count mode (equals/larger/smaller)
        case 104: return GD_VAL_BOOL;  // multi activate (count trigger)
        case 381: return GD_VAL_BOOL;  // pickup item (2.2, robtop changed the pickup_item id to 381 for some reason but 2.1 levels like fingerdash use 79)
        case 90: return GD_VAL_FLOAT;  // follow speed (follow player Y trigger)
        case 91: return GD_VAL_FLOAT;  // follow delay (follow player Y trigger)
        case 92: return GD_VAL_FLOAT;  // follow offset (follow player Y trigger)
        case 105: return GD_VAL_FLOAT; // follow max speed (follow plaeyr Y trigger)
        case 128: return GD_VAL_FLOAT; // scale X
        case 129: return GD_VAL_FLOAT; // scale Y
        case 57: return GD_VAL_INT_ARRAY; // groups
        case 504: return GD_VAL_BOOL;   // spawn only (toggle orb)
        case 99: return GD_VAL_BOOL;   // multi activate (orbs / pads)
        case 100: return GD_VAL_BOOL;  // target mode (move trigger) are you happy now, cloud?
        default:
            return GD_VAL_INT; // Default fallback
    }
}

int convertObject(int id) {
    switch (id) {
        case V2_0_LINE_TRIGGER:
            return LINE_TRIGGER;

        // User coin
        case 1329:
            return SECRET_COIN;
    }
    return id;
}

int allocateTriggerComponent(int obj) {
    int index = -1;
    switch (objects.id[obj]) {
        case COL_TRIGGER:
        case BG_TRIGGER:
        case GROUND_TRIGGER:
        case LINE_TRIGGER:
        case V2_0_LINE_TRIGGER:
        case OBJ_TRIGGER:
        case OBJ_2_TRIGGER:
        case COL2_TRIGGER:
        case COL3_TRIGGER:
        case COL4_TRIGGER:
        case THREEDL_TRIGGER:
        case GROUND_2_TRIGGER:
            index = triggerPoolAdd(&colPool, sizeof(ColorTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
                getColorTrigger(obj)->opacity = 1.0f;
            }
            break;
        case MOVE_TRIGGER:
            index = triggerPoolAdd(&movePool, sizeof(MoveTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case COUNT_TRIGGER:
            index = triggerPoolAdd(&countPool, sizeof(CountTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case INSTANT_COUNT_TRIGGER:
            index = triggerPoolAdd(&instantCountPool, sizeof(InstantCountTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case FOLLOW_PLAYER_Y_TRIGGER:
            hasFollowPlayerY = true;
            index = triggerPoolAdd(&followPlayerYPool, sizeof(FollowPlayerYTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case FOLLOW_TRIGGER:
            index = triggerPoolAdd(&followPool, sizeof(FollowTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case ALPHA_TRIGGER:
            index = triggerPoolAdd(&alphaPool, sizeof(AlphaTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
                getAlphaTrigger(obj)->triggerOpacity = 1.0f;
            }
            break;
        case PULSE_TRIGGER:
            index = triggerPoolAdd(&pulsePool, sizeof(PulseTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
                getPulseTrigger(obj)->opacity = 1.0f;
            }
            break;
        case ENABLE_TRAIL: 
        case DISABLE_TRAIL:
        case HIDE_PLAYER_TRIGGER:
        case SHOW_PLAYER_TRIGGER:
            index = triggerPoolAdd(&simplePool, sizeof(SimpleTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case TOGGLE_TRIGGER:
        case TOGGLE_ORB:
            index = triggerPoolAdd(&togglePool, sizeof(ToggleTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case TOUCH_TRIGGER:
            index = triggerPoolAdd(&touchPool, sizeof(TouchTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case SPAWN_TRIGGER:
            index = triggerPoolAdd(&spawnPool, sizeof(SpawnTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case STOP_TRIGGER:
            index = triggerPoolAdd(&stopPool, sizeof(StopTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case ON_DEATH_TRIGGER:
            index = triggerPoolAdd(&onDeathPool, sizeof(OnDeathTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
        case ROTATE_TRIGGER:
            index = triggerPoolAdd(&rotatePool, sizeof(RotateTrigger));
            if (index >= 0) {
                objects.triggerIndex[obj] = index;
            }
            break;
    }
    return index;
}

void initDefaultVariables(int object) {
    if (!isValidObject(objects.id[object])) {
        return;
    }
    
    const GameObject *gameObj = &gameObjects[objects.id[object]];

    objects.zlayer[object] = gameObj->zLayer;
    objects.zorder[object] = gameObj->zOrder;
}

bool fillObjectData(int object, int key, GDValueType type, GDValue val) {
    
    // Default members
    int triggerIndex = objects.triggerIndex[object];
    switch (key) {
        case 1:  // ID
            if (type == GD_VAL_INT) {
                objects.id[object] = convertObject(val.i);

                initDefaultVariables(object);
                if (isComplexTriggerObject(val.i) || isSimpleTriggerObject(val.i) || val.i == TOGGLE_ORB) {
                    // Allocate its data
                    triggerIndex = allocateTriggerComponent(object);
                    if (triggerIndex < 0) {
                        return false;
                    }
                }

                if (objects.id[object] == TEXT_OBJECT) {
                    int textIndex = triggerPoolAdd(&textPool, sizeof(TextObject));
                    objects.triggerIndex[object] = (textIndex < 0) ? -1 : textIndex;
                }
                if (objects.id[object] == COUNTER_LABEL) {
                    int counterLabelIndex = triggerPoolAdd(&counterLabelPool, sizeof(CounterLabel));
                    objects.triggerIndex[object] = (counterLabelIndex < 0) ? -1 : counterLabelIndex;
                }

            }
            break;
        case 2:  // X
            if (type == GD_VAL_FLOAT) {
                objects.x[object] = val.f;
                objects.originalX[object] = val.f;
            }
            break;
        case 3:  // Y
            if (type == GD_VAL_FLOAT) {
                objects.y[object] = val.f;
                objects.lastY[object] = val.f;
                objects.originalY[object] = val.f;
            }
            break;
        case 4:  // FlippedH
            if (type == GD_VAL_BOOL) objects.flippedH[object] = val.b;
            break;
        case 5:  // FlippedV
            if (type == GD_VAL_BOOL) objects.flippedV[object] = val.b;
            break;
        case 6:  // Rotation
            if (type == GD_VAL_FLOAT) {
                objects.rotation[object] = val.f;
                objects.visualRotation[object] = val.f;
            }
            break;
        case 7:  // Color R
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->trigColorR = val.i;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->trigColorR = val.i;
                }
            }
            break;
        case 8:  // Color G
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->trigColorG = val.i;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->trigColorG = val.i;
                }
            }
            break;
        case 9:  // Color B
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->trigColorB = val.i;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->trigColorB = val.i;
                }
            }
            break;
        case 10: // Duration
            if (type == GD_VAL_FLOAT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->trigDuration = val.f;
                } else {
                    switch (objects.id[object]) {
                        case MOVE_TRIGGER:
                            getMoveTrigger(object)->trigDuration = val.f;
                            break;
                        case ALPHA_TRIGGER:
                            getAlphaTrigger(object)->trigDuration = val.f;
                            break;
                        case PULSE_TRIGGER:
                            getPulseTrigger(object)->trigDuration = val.f;
                            break;
                        case SPAWN_TRIGGER:
                            getSpawnTrigger(object)->trigDuration = val.f;
                            break;
                        case ROTATE_TRIGGER:
                            getRotateTrigger(object)->trigDuration = val.f;
                            break;
                        case FOLLOW_TRIGGER:
                            getFollowTrigger(object)->trigDuration = val.f;
                            break;
                    }
                }
            }
            break;
        case 11: // Touch triggered
            if (type == GD_VAL_BOOL) objects.touchTriggered[object] = val.b;
            break;
        case 14: // Tint Ground
            if (type == GD_VAL_BOOL && isColorTrigger(objects.id[object])) {
                getColorTrigger(object)->tintGround = val.b;
            }
            break;
        case 15: // Player 1 color
            if (type == GD_VAL_BOOL && isColorTrigger(objects.id[object])) {
                getColorTrigger(object)->p1Color = val.b;
            }
            break;
        case 16: // Player 2 color
            if (type == GD_VAL_BOOL && isColorTrigger(objects.id[object])) {
                getColorTrigger(object)->p2Color = val.b;
            }
            break;
        case 17: // Blending
            if (type == GD_VAL_BOOL && isColorTrigger(objects.id[object])) {
                getColorTrigger(object)->blending = val.b;
            }
            break;
        case 19: // 1.9 channel id
            if (type == GD_VAL_INT) objects.v1p9ColChannel[object] = convertOnePointNineChannel(val.i);
            break;
        case 21: // Main col channel
            if (type == GD_VAL_INT) objects.colChannel[object] = val.i;
            break;
        case 22: // Detail col channel
            if (type == GD_VAL_INT) objects.detailColChannel[object] = val.i;
            break;
        case 23: // Target color ID
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->targetColorID = val.i;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->targetColorID = val.i;
                }
            }
            break;
        case 24: // Z layer
            if (type == GD_VAL_INT) objects.zlayer[object] = val.i;
            break;
        case 25: // Z order
            if (type == GD_VAL_INT) objects.zorder[object] = val.i;
            break;
        case 28: // Move offset X
            if (type == GD_VAL_FLOAT && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->moveOffsetX = val.f;
            }
            break;
        case 29: // Move offset Y
            if (type == GD_VAL_FLOAT && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->moveOffsetY = val.f;
            }
            break;
        case 30: // Move easing
            if (type == GD_VAL_INT) {
                if (objects.id[object] == MOVE_TRIGGER) {
                    getMoveTrigger(object)->moveEasing = val.i;
                } else if (objects.id[object] == ROTATE_TRIGGER) {
                    getRotateTrigger(object)->rotateEasing = val.i;
                }
            }
            break;
        case 31: // Text
            if (type == GD_VAL_STRING && objects.id[object] == TEXT_OBJECT && val.str) {
                fixBase64URL(val.str);

                char *decoded = (char *)malloc(strlen(val.str) + 1);
                if (decoded) {
                    int decodedLen = base64Decode(val.str, (unsigned char *)decoded);

                    if (decodedLen > 0 && isAscii((unsigned char *)decoded, decodedLen)) {
                        int len = decodedLen > MAX_TEXT_LEN ? MAX_TEXT_LEN : decodedLen;
                        TextObject *textObj = getTextObject(object);
                        memcpy(textObj->text, decoded, len);
                        textObj->text[len] = '\0';
                        textObj->len = (unsigned char)len;
                    }

                    free(decoded);
                }
            }
            break;
        case 32: // Scale (uniform)
            if (type == GD_VAL_FLOAT) {
                objects.scaleX[object] = objects.scaleY[object] = val.f;
                objects.originalScaleX[object] = objects.originalScaleY[object] = val.f;
            }
            break;
        case 35: // Opacity (alpha trigger)
            if (type == GD_VAL_FLOAT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->opacity = val.f;
                } else if (objects.id[object] == ALPHA_TRIGGER) {
                    getAlphaTrigger(object)->triggerOpacity = val.f;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->opacity = val.f;
                }
            }
            break;
        case 41: // main_col_HSV_enabled
            if (type == GD_VAL_BOOL) objects.mainColHSVEnabled[object] = val.b;
            break;
        case 42: // detail_col_HSV_enabled
            if (type == GD_VAL_BOOL) objects.detailColHSVEnabled[object] = val.b;
            break;
        case 43: // main_col_HSV
            if (type == GD_VAL_HSV) objects.mainColHSV[object] = val.hsv;
            break;
        case 44: // detail_col_HSV
            if (type == GD_VAL_HSV) objects.detailColHSV[object] = val.hsv;
            break;
        case 45: // pulse fade_in
            if (type == GD_VAL_FLOAT && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseFadeIn = val.f;
            }
            break;
        case 46: // pulse hold
            if (type == GD_VAL_FLOAT && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseHold = val.f;
            }
            break;
        case 47: // pulse fade_out
            if (type == GD_VAL_FLOAT && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseFadeOut = val.f;
            }
            break;
        case 48: // pulse_mode
            if (type == GD_VAL_INT && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseMode = val.i;
            }
            break;
        case 49: // copied_hsv
            if (type == GD_VAL_HSV) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->copiedHSV = val.hsv;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->copiedHSV = val.hsv;
                }
            }
            break;
        case 50: // copied_color_id
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->copiedColorID = val.i;
                } else if (objects.id[object] == PULSE_TRIGGER) {
                    getPulseTrigger(object)->copiedColorID = val.i;
                }
            }
            break;
        case 51: // Target group
            if (type == GD_VAL_INT) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->targetGroup = val.i;
                } else {
                    switch (objects.id[object]) {
                        case MOVE_TRIGGER:
                            getMoveTrigger(object)->targetGroup = val.i;
                            break;
                        case COUNT_TRIGGER:
                            getCountTrigger(object)->targetGroup = val.i;
                            break;
                        case INSTANT_COUNT_TRIGGER:
                            getInstantCountTrigger(object)->targetGroup = val.i;
                            break;
                        case FOLLOW_PLAYER_Y_TRIGGER:
                            getFollowPlayerYTrigger(object)->targetGroup = val.i;
                            break;
                        case FOLLOW_TRIGGER:
                            getFollowTrigger(object)->targetGroup = val.i;
                            break;
                        case ALPHA_TRIGGER:
                            getAlphaTrigger(object)->targetGroup = val.i;
                            break;
                        case PULSE_TRIGGER:
                            getPulseTrigger(object)->targetGroup = val.i;
                            break;
                        case TOGGLE_TRIGGER:
                            getToggleTrigger(object)->targetGroup = val.i;
                            break;
                        case TOGGLE_ORB:
                            getToggleTrigger(object)->targetGroup = val.i;
                            break;
                        case TOUCH_TRIGGER:
                            getTouchTrigger(object)->targetGroup = val.i;
                            break;
                        case SPAWN_TRIGGER:
                            getSpawnTrigger(object)->targetGroup = val.i;
                            break;
                        case STOP_TRIGGER:
                            getStopTrigger(object)->targetGroup = val.i;
                            break;
                        case ON_DEATH_TRIGGER:
                            getOnDeathTrigger(object)->targetGroup = val.i;
                            break;
                        case ROTATE_TRIGGER:
                            getRotateTrigger(object)->targetGroup = val.i;
                            break;
                    }
                }
            }
            break;
        case 52: // pulse_target_type
            if (type == GD_VAL_INT && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseTargetType = val.i;
            }
            break;
        case 54: // teleport portal Y offset
            if (type == GD_VAL_FLOAT) objects.tpYOffset[object] = val.f;
            break;
        case 56: // Activate group
            if (type == GD_VAL_BOOL) {
                if (objects.id[object] == TOGGLE_TRIGGER || objects.id[object] == TOGGLE_ORB) {
                    getToggleTrigger(object)->activateGroup = val.b;
                } else if (objects.id[object] == COUNT_TRIGGER) {
                    getCountTrigger(object)->activateGroup = val.b;
                } else if (objects.id[object] == INSTANT_COUNT_TRIGGER) {
                    getInstantCountTrigger(object)->activateGroup = val.b;
                } else if (objects.id[object] == ON_DEATH_TRIGGER) {
                    getOnDeathTrigger(object)->activate = val.b;
                }
            }
            break;
        case 58: // Lock to player X
            if (type == GD_VAL_BOOL && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->lockToPlayerX = val.b;
            }
            break;
        case 59: // Lock to player Y
            if (type == GD_VAL_BOOL && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->lockToPlayerY = val.b;
            }
            break;
        case 62: // spawn_triggered
            if (type == GD_VAL_BOOL) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->spawnTriggered = val.b;
                } else if (isSimpleTriggerObject(objects.id[object])) {
                    getSimpleTrigger(object)->spawnTriggered = val.b;
                } else {
                    switch (objects.id[object]) {
                        case MOVE_TRIGGER:
                            getMoveTrigger(object)->spawnTriggered = val.b;
                            break;
                        case FOLLOW_PLAYER_Y_TRIGGER:
                            getFollowPlayerYTrigger(object)->spawnTriggered = val.b;
                            break;
                        case FOLLOW_TRIGGER:
                            getFollowTrigger(object)->spawnTriggered = val.b;
                            break;
                        case ALPHA_TRIGGER:
                            getAlphaTrigger(object)->spawnTriggered = val.b;
                            break;
                        case PULSE_TRIGGER:
                            getPulseTrigger(object)->spawnTriggered = val.b;
                            break;
                        case TOGGLE_TRIGGER:
                            getToggleTrigger(object)->spawnTriggered = val.b;
                            break;
                        case TOUCH_TRIGGER:
                            getTouchTrigger(object)->spawnTriggered = val.b;
                            break;
                        case SPAWN_TRIGGER:
                            getSpawnTrigger(object)->spawnTriggered = val.b;
                            break;
                        case ROTATE_TRIGGER:
                            getRotateTrigger(object)->spawnTriggered = val.b;
                            break;
                        case COUNT_TRIGGER:
                            getCountTrigger(object)->spawnTriggered = val.b;
                            break;
                        case INSTANT_COUNT_TRIGGER:
                            getInstantCountTrigger(object)->spawnTriggered = val.b;
                            break;
                        case STOP_TRIGGER:
                            getStopTrigger(object)->spawnTriggered = val.b;
                            break;
                        case ON_DEATH_TRIGGER:
                            getOnDeathTrigger(object)->spawnTriggered = val.b;
                            break;
                    }
                }
            }
            break;
        case 63: // spawn_delay
            if (type == GD_VAL_FLOAT && objects.id[object] == SPAWN_TRIGGER) {
                getSpawnTrigger(object)->spawnDelay = val.f;
            }
            break;
        case 64: // dont fade
            if (type == GD_VAL_BOOL) {
                objects.flags[object] |= val.b ? FLAG_DONT_FADE : 0;
            }
            break;
        case 65: // pulse main_only
            if (type == GD_VAL_BOOL && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseMainOnly = val.b;
            }
            break;
        case 66: // pulse detail_only
            if (type == GD_VAL_BOOL && objects.id[object] == PULSE_TRIGGER) {
                getPulseTrigger(object)->pulseDetailOnly = val.b;
            }
            break;
        case 67: // dont enter
            if (type == GD_VAL_BOOL) {
                objects.flags[object] |= val.b ? FLAG_DONT_ENTER : 0;
            }
            break;
        case 68:
            if (type == GD_VAL_FLOAT && objects.id[object] == ROTATE_TRIGGER) {
                getRotateTrigger(object)->degrees = val.f;
            }
            break;
        case 69:
            if (type == GD_VAL_INT && objects.id[object] == ROTATE_TRIGGER) {
                getRotateTrigger(object)->times360 = val.i;
            }
            break;
        case 70:
            if (type == GD_VAL_BOOL && objects.id[object] == ROTATE_TRIGGER) {
                getRotateTrigger(object)->lockObjectRotation = val.b;
            }
            break;
        case 71:
            if (type == GD_VAL_INT && objects.id[object] == ROTATE_TRIGGER) {
                getRotateTrigger(object)->centerGroup = val.i;
            } else if (type == GD_VAL_INT && objects.id[object] == FOLLOW_TRIGGER) {
                getFollowTrigger(object)->followGroup = val.i;
            } else if (type == GD_VAL_INT && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->targetGroup2 = val.i;
            }
            break;
        case 72:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_TRIGGER) {
                getFollowTrigger(object)->xMod = val.f;
            }
            break;
        case 73:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_TRIGGER) {
                getFollowTrigger(object)->yMod = val.f;
            }
            break;
        case 85:
            if (type == GD_VAL_FLOAT && objects.id[object] == ROTATE_TRIGGER) {
                getRotateTrigger(object)->easingRate = val.f;
            }
            break;
        case 81:
            if (type == GD_VAL_INT && objects.id[object] == TOUCH_TRIGGER) {
                getTouchTrigger(object)->holdMode = val.i != 0;
            }
            break;
        case 82:
            if (type == GD_VAL_INT && objects.id[object] == TOUCH_TRIGGER) {
                getTouchTrigger(object)->toggleMode = (unsigned char)val.i;
            }
            break;
        case 89:
            break;
        case 87: // multi_triggered
            if (type == GD_VAL_BOOL) {
                if (isColorTrigger(objects.id[object])) {
                    getColorTrigger(object)->multiTriggered = val.b;
                } else if (isSimpleTriggerObject(objects.id[object])) {
                    getSimpleTrigger(object)->multiTriggered = val.b;
                } else {
                    switch (objects.id[object]) {
                        case MOVE_TRIGGER:
                            getMoveTrigger(object)->multiTriggered = val.b;
                            break;
                        case FOLLOW_PLAYER_Y_TRIGGER:
                            getFollowPlayerYTrigger(object)->multiTriggered = val.b;
                            break;
                        case FOLLOW_TRIGGER:
                            getFollowTrigger(object)->multiTriggered = val.b;
                            break;
                        case ALPHA_TRIGGER:
                            getAlphaTrigger(object)->multiTriggered = val.b;
                            break;
                        case PULSE_TRIGGER:
                            getPulseTrigger(object)->multiTriggered = val.b;
                            break;
                        case TOGGLE_TRIGGER:
                            getToggleTrigger(object)->multiTriggered = val.b;
                            break;
                        case TOUCH_TRIGGER:
                            getTouchTrigger(object)->multiTriggered = val.b;
                            break;
                        case SPAWN_TRIGGER:
                            getSpawnTrigger(object)->multiTriggered = val.b;
                            break;
                        case ROTATE_TRIGGER:
                            getRotateTrigger(object)->multiTriggered = val.b;
                            break;
                        case COUNT_TRIGGER:
                            getCountTrigger(object)->multiTriggered = val.b;
                            break;
                        case INSTANT_COUNT_TRIGGER:
                            getInstantCountTrigger(object)->multiTriggered = val.b;
                            break;
                        case STOP_TRIGGER:
                            getStopTrigger(object)->multiTriggered = val.b;
                            break;
                        case ON_DEATH_TRIGGER:
                            getOnDeathTrigger(object)->multiTriggered = val.b;
                            break;
                    }
                }
            }
            break;
        case 88:
            if (type == GD_VAL_INT && objects.id[object] == INSTANT_COUNT_TRIGGER) {
                getInstantCountTrigger(object)->instantCountMode = val.i;
            }
            break;
        case 77:
            if (type == GD_VAL_INT) {
                if (objects.id[object] == COUNT_TRIGGER) {
                    getCountTrigger(object)->targetCount = val.i;
                } else if (objects.id[object] == INSTANT_COUNT_TRIGGER) {
                    getInstantCountTrigger(object)->targetCount = val.i;
                }
            }
            break;
        case 78:
            if (type == GD_VAL_BOOL && val.b) {
                PickupItem *pickup = ensurePickupItem(object);
                if (pickup) pickup->subCount = true;
            }
            break;
        case 80:
            if (type == GD_VAL_INT) {
                switch (objects.id[object]) {
                    case COUNT_TRIGGER:
                        getCountTrigger(object)->itemID = val.i;
                        break;
                    case INSTANT_COUNT_TRIGGER:
                        getInstantCountTrigger(object)->itemID = val.i;
                        break;
                    case COUNTER_LABEL:
                    {
                        CounterLabel *label = getCounterLabel(object);
                        if (label) label->itemID = val.i;
                        break;
                    }
                    default:
                    {
                        PickupItem *pickup = ensurePickupItem(object);
                        if (pickup) pickup->itemID = val.i;
                        break;
                    }
                }
            }
            break;
        case 104:
            if (type == GD_VAL_BOOL && objects.id[object] == COUNT_TRIGGER) {
                getCountTrigger(object)->multiActivate = val.b;
            }
            break;
        case 79:
        case 381:
            if (type == GD_VAL_BOOL && val.b) {
                PickupItem *pickup = ensurePickupItem(object);
                if (pickup) pickup->isPickup = true;
            }
            break;
        case 90:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_PLAYER_Y_TRIGGER) {
                getFollowPlayerYTrigger(object)->followSpeed = val.f;
            }
            break;
        case 91:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_PLAYER_Y_TRIGGER) {
                getFollowPlayerYTrigger(object)->followDelay = val.f;
            }
            break;
        case 92:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_PLAYER_Y_TRIGGER) {
                getFollowPlayerYTrigger(object)->followOffset = val.f;
            }
            break;
        case 105:
            if (type == GD_VAL_FLOAT && objects.id[object] == FOLLOW_PLAYER_Y_TRIGGER) {
                getFollowPlayerYTrigger(object)->followMaxSpeed = val.f;
            }
            break;
        case 128: // Scale X
            if (type == GD_VAL_FLOAT) {
                objects.scaleX[object] = val.f;
                objects.originalScaleX[object] = val.f;
            }
            break;
        case 129: // Scale Y
            if (type == GD_VAL_FLOAT) {
                objects.scaleY[object] = val.f;
                objects.originalScaleY[object] = val.f;
            }
            break;
        case 57: // Groups
            if (type == GD_VAL_INT_ARRAY) {
                int gcount = 0;
                for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
                    objects.groups[object][i] = val.intArray[i];
                    if (val.intArray[i] != 0) gcount++;
                }
                objects.groupCount[object] = gcount;
            }
            break;
        case 504:
            if (type == GD_VAL_BOOL && objects.id[object] == TOGGLE_ORB) {
                getToggleTrigger(object)->spawnOnly = val.b;
            }
            break;
        case 99:
            if (type == GD_VAL_BOOL) {
                objects.flags[object] |= val.b ? FLAG_MULTI_ACTIVATE : 0;
            }
            break;
        case 100:
            if (type == GD_VAL_BOOL && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->targetMode = val.b;
            }
            break;
        case 395:
            if (type == GD_VAL_INT && objects.id[object] == MOVE_TRIGGER) {
                getMoveTrigger(object)->centerGroup = val.i;
            }
            break;
    }
    return true;
}

bool objHasMain(const GameObject *obj) {
    if (obj->colorType != COLOR_TYPE_DETAIL) return true;
    
    for (size_t i = 0; i < obj->childCount; i++) {
        if (obj->children[i].colorType != COLOR_TYPE_DETAIL) return true;
    }
    return false;
}

bool objHasDetail(const GameObject *obj) {
    if (obj->colorType == COLOR_TYPE_DETAIL) return true;
    
    for (size_t i = 0; i < obj->childCount; i++) {
        if (obj->children[i].colorType == COLOR_TYPE_DETAIL) return true;
    }
    return false;
}

bool isValidObject(int id) {
    return id >= 1 && id < GAME_OBJECT_COUNT;
}

int getLevelCoinCount(void) {
    return coinCount;
}

bool isColorTrigger(int id) {
    return id == COL_TRIGGER
        || id == BG_TRIGGER
        || id == GROUND_TRIGGER
        || id == LINE_TRIGGER
        || id == V2_0_LINE_TRIGGER
        || id == OBJ_TRIGGER
        || id == OBJ_2_TRIGGER
        || id == COL2_TRIGGER
        || id == COL3_TRIGGER
        || id == COL4_TRIGGER
        || id == THREEDL_TRIGGER
        || id == GROUND_2_TRIGGER;
}

bool isComplexTriggerObject(int id) {
    return isColorTrigger(id)
        || id == MOVE_TRIGGER
        || id == ALPHA_TRIGGER
        || id == TOGGLE_TRIGGER
        || id == TOUCH_TRIGGER
        || id == PULSE_TRIGGER
        || id == SPAWN_TRIGGER
        || id == ROTATE_TRIGGER
        || id == COUNT_TRIGGER
        || id == INSTANT_COUNT_TRIGGER
        || id == FOLLOW_PLAYER_Y_TRIGGER
        || id == FOLLOW_TRIGGER
        || id == STOP_TRIGGER
        || id == ON_DEATH_TRIGGER;
}

// Triggers that aren't configurable but have spawn triggered option
bool isSimpleTriggerObject(int id) {
    switch (id) {
        case HIDE_PLAYER_TRIGGER: 
        case SHOW_PLAYER_TRIGGER:
        case ENABLE_TRAIL: 
        case DISABLE_TRAIL:
            return true;
        default:
            return false;
    }
}

bool isTriggerObject(int id) {
    if (isComplexTriggerObject(id) || isSimpleTriggerObject(id)) return true;
    switch (id) {
        case TRIGGER_FADE_SIMPLE: case TRIGGER_FADE_UP: case TRIGGER_FADE_DOWN:
        case TRIGGER_FADE_RIGHT: case TRIGGER_FADE_LEFT:
        case TRIGGER_FADE_SCALE_IN: case TRIGGER_FADE_SCALE_OUT:
        case TRIGGER_FADE_INWARDS: case TRIGGER_FADE_OUTWARDS:
        case TRIGGER_FADE_LEFT_SEMICIRCLE: case TRIGGER_FADE_RIGHT_SEMICIRCLE:
            return true;
        default: return false;
    }
}

bool triggerIsSpawnTriggered(int id, int obj) {
    if (isColorTrigger(id)) return getColorTrigger(obj)->spawnTriggered;
    if (isSimpleTriggerObject(id)) return getSimpleTrigger(obj)->spawnTriggered;
    switch (id) {
        case MOVE_TRIGGER:   return getMoveTrigger(obj)->spawnTriggered;
        case COUNT_TRIGGER:  return getCountTrigger(obj)->spawnTriggered;
        case INSTANT_COUNT_TRIGGER: return getInstantCountTrigger(obj)->spawnTriggered;
        case FOLLOW_PLAYER_Y_TRIGGER: return getFollowPlayerYTrigger(obj)->spawnTriggered;
        case FOLLOW_TRIGGER: return getFollowTrigger(obj)->spawnTriggered;
        case ALPHA_TRIGGER:  return getAlphaTrigger(obj)->spawnTriggered;
        case PULSE_TRIGGER:  return getPulseTrigger(obj)->spawnTriggered;
        case TOGGLE_TRIGGER: return getToggleTrigger(obj)->spawnTriggered;
        case TOUCH_TRIGGER:  return getTouchTrigger(obj)->spawnTriggered;
        case SPAWN_TRIGGER:  return getSpawnTrigger(obj)->spawnTriggered;
        case ROTATE_TRIGGER: return getRotateTrigger(obj)->spawnTriggered;
        case STOP_TRIGGER:   return getStopTrigger(obj)->spawnTriggered;
        case ON_DEATH_TRIGGER: return getOnDeathTrigger(obj)->spawnTriggered;
    }
    return false;
}

bool triggerIsMultiTriggered(int id, int obj) {
    if (isColorTrigger(id)) return getColorTrigger(obj)->multiTriggered;
    if (isSimpleTriggerObject(id)) return getSimpleTrigger(obj)->multiTriggered;
    switch (id) {
        case MOVE_TRIGGER:   return getMoveTrigger(obj)->multiTriggered;
        case COUNT_TRIGGER:  return getCountTrigger(obj)->multiTriggered;
        case INSTANT_COUNT_TRIGGER: return getInstantCountTrigger(obj)->multiTriggered;
        case FOLLOW_PLAYER_Y_TRIGGER: return getFollowPlayerYTrigger(obj)->multiTriggered;
        case FOLLOW_TRIGGER: return getFollowTrigger(obj)->multiTriggered;
        case ALPHA_TRIGGER:  return getAlphaTrigger(obj)->multiTriggered;
        case PULSE_TRIGGER:  return getPulseTrigger(obj)->multiTriggered;
        case TOGGLE_TRIGGER: return getToggleTrigger(obj)->multiTriggered;
        case TOUCH_TRIGGER:  return getTouchTrigger(obj)->multiTriggered;
        case SPAWN_TRIGGER:  return getSpawnTrigger(obj)->multiTriggered;
        case ROTATE_TRIGGER: return getRotateTrigger(obj)->multiTriggered;
        case STOP_TRIGGER:   return getStopTrigger(obj)->multiTriggered;
        case ON_DEATH_TRIGGER: return getOnDeathTrigger(obj)->multiTriggered;
    }
    return false;
}

static bool isHighDetailObject(const char *section, const char *end, float *outX) {
    if (!levelInfo.lowDetail) return false;

    if (!end) end = section + strlen(section);

    bool highDetail = false;
    const char *p = section;

    while (p < end) {
        char *keyEnd;
        long key = strtol(p, &keyEnd, 10);
        if (keyEnd == p || keyEnd >= end || *keyEnd != ',') break;

        const char *value = keyEnd + 1;
        const char *valueEnd = value;
        while (valueEnd < end && *valueEnd != ',') valueEnd++;

        if (key == 103) {
            highDetail = (valueEnd - value == 1 && value[0] == '1');
        } else if (key == 2 && outX) {
            *outX = atof(value);
        }

        p = (valueEnd < end) ? valueEnd + 1 : end;
    }

    return highDetail;
}

int parseGdObject(const char *objStr, const char *objEnd, int obj, int capacity) {
    float highDetailX = 0.f;

    if (isHighDetailObject(objStr, objEnd, &highDetailX)) {
        if (highDetailX > levelInfo.lastObjX) levelInfo.lastObjX = highDetailX;
        return -1;
    }

    if (obj >= capacity) return -2;

    int count = 0;
    // Split object into each key
    char **tokens = splitString(objStr, ',', &count, false);
    if (count < 1) {
        freeStringArray(tokens, count);
        return 0;
    }

    // Iterate through all keys
    for (int i = 0; i + 1 < count; i += 2) {
        int key = atoi(tokens[i]);
        const char *valStr = tokens[i + 1];
        GDValueType type = getValueTypeForKey(key);
        GDValue val;

        switch (type) {
            case GD_VAL_INT:
                val.i = atoi(valStr);
                if (!fillObjectData(obj, key, GD_VAL_INT, val)) {
                    return 0;
                }
                break;
            case GD_VAL_FLOAT:
                val.f = atof(valStr);
                if (!fillObjectData(obj, key, GD_VAL_FLOAT, val)) {
                    return 0;
                }
                break;
            case GD_VAL_BOOL:
                val.b = parseBool(valStr);
                if (!fillObjectData(obj, key, GD_VAL_BOOL, val)) {
                    return 0;
                }
                break;
            case GD_VAL_HSV:
                val.hsv = parseHSVString(valStr);
                if (!fillObjectData(obj, key, GD_VAL_HSV, val)) {
                    return 0;
                }
                break;
            case GD_VAL_INT_ARRAY:
                parseInts(val.intArray, valStr);
                fillObjectData(obj, key, GD_VAL_INT_ARRAY, val);
                break;
            case GD_VAL_STRING:
                val.str = (char *) valStr;
                fillObjectData(obj, key, GD_VAL_STRING, val);
                break;
            default:
                break;
        }
    }

    int objID = objects.id[obj];
    
    if (isValidObject(objID)) {
        // Get coins
        if (objID == SECRET_COIN) {
            if (coinCount < 3) {
                coinIds[coinCount++] = obj;
            }
        }
        
        const GameObject *gameObject = &gameObjects[objects.id[obj]];

        // Get proper color channels
        if (gameObject->swapBaseDetail) {
            if (!objHasMain(gameObject)) {
                if (!objects.colChannel[obj]) objects.colChannel[obj] = gameObject->baseColor;
            } else {
                if (!objects.detailColChannel[obj]) objects.detailColChannel[obj] = gameObject->baseColor;
            }
        } else {
            if (!objects.colChannel[obj]) objects.colChannel[obj] = gameObject->baseColor;
            if (!objects.detailColChannel[obj]) objects.detailColChannel[obj] = gameObject->detailColor ? gameObject->detailColor : 1;
        }

        // Give each object its own random value
        objects.random[obj] = rand();

        const ObjectHitbox *hitbox = gameObject->hitbox;
        if (hitbox) {
            objects.width[obj] = hitbox->width;
            objects.height[obj] = hitbox->height;
            
            // If object is and slope, calculate orientation
            if (hitbox->type == COLLISION_SLOPE) {
                int orientation = objects.rotation[obj] / 90;
                if (objects.flippedH[obj] && objects.flippedV[obj]) orientation += 2;
                else if (objects.flippedH[obj]) orientation += 1;
                else if (objects.flippedV[obj]) orientation += 3;
                
                orientation = orientation % 4;
                if (orientation < 0) orientation += 4;

                objects.orientation[obj] = orientation;
            }

            if (hitbox->collisionType == HITBOX_SOLID) {
                // Modify height and width depending on rotation
                if ((int) fabsf(objects.rotation[obj]) % 180 != 0) {
                    objects.width[obj] = hitbox->height;
                    objects.height[obj] = hitbox->width;
                } else {
                    objects.width[obj] = hitbox->width;
                    objects.height[obj] = hitbox->height;
                }
            }

            // precompute scaled hitbox dimensions (scale_x/scale_y)
            if (hitbox->type == COLLISION_CIRCLE) {
                float s = fmaxf(fabsf(objects.scaleX[obj]), fabsf(objects.scaleY[obj]));
                objects.width[obj] *= s;
            } else if (hitbox->type == COLLISION_SLOPE || hitbox->collisionType == HITBOX_SOLID) {
                if ((int) fabsf(objects.rotation[obj]) % 180 != 0) {
                    objects.width[obj] *= fabsf(objects.scaleY[obj]);
                    objects.height[obj] *= fabsf(objects.scaleX[obj]);
                } else {
                    objects.width[obj] *= fabsf(objects.scaleX[obj]);
                    objects.height[obj] *= fabsf(objects.scaleY[obj]);
                }
            } else {
                objects.width[obj] *= fabsf(objects.scaleX[obj]);
                objects.height[obj] *= fabsf(objects.scaleY[obj]);
            }
        }

        // Modify level ending pos
        if (objects.x[obj] > levelInfo.lastObjX) {
            levelInfo.lastObjX = objects.x[obj];
        }
    } else if (!isTriggerObject(objID)) { // Keep trigger objects even if outside the table
        // Invalid object
        objects.id[obj] = 0;
    }

    if (isTriggerObject(objID)) {
        objects.width[obj] = 30 * fabsf(objects.scaleX[obj]);
        objects.height[obj] = 30 * fabsf(objects.scaleY[obj]);
    }

    if (objectCanBeXMoved(obj)) {
        objects.flags[obj] |= FLAG_CAN_BE_X_MOVED;
    }

    freeStringArray(tokens, count);
    return 1;
}

static int *objects_int_block = NULL;
static float *objects_float_block = NULL;
static unsigned short *objects_ushort_block = NULL;
static unsigned char *objects_uchar_block = NULL;
static bool *objects_bool_block = NULL;
static short (*objects_groups_block)[MAX_GROUPS_PER_OBJECT] = NULL;
static u8 *objects_u8_block = NULL;
static Color *objects_color_block = NULL;
static HSV *objects_hsv_block = NULL;

void freeArrays() {
    hasFollowPlayerY = false;
    colPool = (TriggerPool){0};
    movePool = (TriggerPool){0};
    countPool = (TriggerPool){0};
    instantCountPool = (TriggerPool){0};
    counterLabelPool = (TriggerPool){0};
    pickupPool = (TriggerPool){0};
    followPlayerYPool = (TriggerPool){0};
    followPool = (TriggerPool){0};
    alphaPool = (TriggerPool){0};
    pulsePool = (TriggerPool){0};
    togglePool = (TriggerPool){0};
    touchPool = (TriggerPool){0};
    spawnPool = (TriggerPool){0};
    rotatePool = (TriggerPool){0};
    stopPool = (TriggerPool){0};
    onDeathPool = (TriggerPool){0};
    simplePool = (TriggerPool){0};
    textPool = (TriggerPool){0};

    objects.random = NULL;
    objects.id = NULL;
    objects.x = NULL;
    objects.y = NULL;
    objects.lastY = NULL;
    objects.rotation = NULL;
    objects.visualRotation = NULL;
    objects.zlayer = NULL;
    objects.zorder = NULL;
    objects.opacity = NULL;
    objects.alphaTriggerOpacity = NULL;
    objects.width = NULL;
    objects.height = NULL;
    objects.v1p9ColChannel = NULL;
    objects.colChannel = NULL;
    objects.detailColChannel = NULL;
    objects.triggerIndex = NULL;
    objects.hitboxCounter = NULL;
    objects.transitionApplied = NULL;
    objects.orientation = NULL;
    objects.touchTriggered = NULL;
    objects.flippedH = NULL;
    objects.flippedV = NULL;
    objects.groups = NULL;
    objects.groupCount = NULL;
    objects.flags = NULL;
    objects.activated = NULL;
    objects.collided = NULL;
    objects.originalX = NULL;
    objects.originalY = NULL;
    objects.numMainPulses = NULL;
    objects.numDetailPulses = NULL;
    objects.mainBeingPulsed = NULL;
    objects.detailBeingPulsed = NULL;
    objects.mainNonPulseColor = NULL;
    objects.detailNonPulseColor = NULL;
    objects.mainColor = NULL;
    objects.detailColor = NULL;
    objects.mainColHSVEnabled = NULL;
    objects.detailColHSVEnabled = NULL;
    objects.mainColHSV = NULL;
    objects.detailColHSV = NULL;
    objects.cachedMainHSVSrcColor = NULL;
    objects.cachedDetailHSVSrcColor = NULL;
    objects.cachedMainHSVColor = NULL;
    objects.cachedDetailHSVColor = NULL;
    objects.cachedMainHSVValid = NULL;
    objects.cachedDetailHSVValid = NULL;
    objects.scaleX = NULL;
    objects.scaleY = NULL;
    objects.originalScaleX = NULL;
    objects.originalScaleY = NULL;
    objects.childObject = NULL;
    objects.tpYOffset = NULL;
    objects.sectionX = NULL;
    objects.sectionY = NULL;
    objects.groupCombinationIndex = NULL;
}

bool initArrays(size_t count) {
    hasFollowPlayerY = false;
    objects_int_block = (int *)arenaAlloc(&levelArena, sizeof(int) * count * 9);
    if (!objects_int_block) return false;

    objects_float_block = (float *)arenaAlloc(&levelArena, sizeof(float) * count * 16);
    if (!objects_float_block) return false;

    objects_ushort_block = (unsigned short *)arenaAlloc(&levelArena, sizeof(unsigned short) * count * 4);
    if (!objects_ushort_block) return false;

    objects_uchar_block = (unsigned char *)arenaAlloc(&levelArena, sizeof(unsigned char) * count * 2);
    if (!objects_uchar_block) return false;

    objects_bool_block = (bool *)arenaAlloc(&levelArena, sizeof(bool) * count * 9);
    if (!objects_bool_block) return false;

    objects_groups_block = (short (*)[MAX_GROUPS_PER_OBJECT])arenaAlloc(&levelArena, sizeof(short[MAX_GROUPS_PER_OBJECT]) * count);
    if (!objects_groups_block) return false;

    objects_u8_block = (u8 *)arenaAlloc(&levelArena, sizeof(u8) * count * 6);
    if (!objects_u8_block) return false;

    objects_color_block = (Color *)arenaAlloc(&levelArena, sizeof(Color) * count * 8);
    if (!objects_color_block) return false;

    objects_hsv_block = (HSV *)arenaAlloc(&levelArena, sizeof(HSV) * count * 2);
    if (!objects_hsv_block) return false;

    objects.random = objects_int_block + 0 * count;
    objects.id = objects_int_block + 1 * count;
    objects.zlayer = objects_int_block + 2 * count;
    objects.zorder = objects_int_block + 3 * count;
    objects.triggerIndex = objects_int_block + 4 * count;
    objects.childObject = objects_int_block + 5 * count;
    objects.sectionX = objects_int_block + 6 * count;
    objects.sectionY = objects_int_block + 7 * count;
    objects.groupCombinationIndex = objects_int_block + 8 * count;

    objects.x = objects_float_block + 0 * count;
    objects.y = objects_float_block + 1 * count;
    objects.lastY = objects_float_block + 2 * count;
    objects.rotation = objects_float_block + 3 * count;
    objects.visualRotation = objects_float_block + 4 * count;
    objects.opacity = objects_float_block + 5 * count;
    objects.alphaTriggerOpacity = objects_float_block + 6 * count;
    objects.width = objects_float_block + 7 * count;
    objects.height = objects_float_block + 8 * count;
    objects.originalX = objects_float_block + 9 * count;
    objects.originalY = objects_float_block + 10 * count;
    objects.scaleX = objects_float_block + 11 * count;
    objects.scaleY = objects_float_block + 12 * count;
    objects.originalScaleX = objects_float_block + 13 * count;
    objects.originalScaleY = objects_float_block + 14 * count;
    objects.tpYOffset = objects_float_block + 15 * count;

    objects.v1p9ColChannel = objects_ushort_block + 0 * count;
    objects.colChannel = objects_ushort_block + 1 * count;
    objects.detailColChannel = objects_ushort_block + 2 * count;
    objects.hitboxCounter = (unsigned char *)(objects_ushort_block + 3 * count);

    objects.transitionApplied = objects_uchar_block + 0 * count;
    objects.orientation = objects_uchar_block + 1 * count;

    objects.touchTriggered = objects_bool_block + 0 * count;
    objects.flippedH = objects_bool_block + 1 * count;
    objects.flippedV = objects_bool_block + 2 * count;
    objects.mainBeingPulsed = objects_bool_block + 3 * count;
    objects.detailBeingPulsed = objects_bool_block + 4 * count;
    objects.mainColHSVEnabled = objects_bool_block + 5 * count;
    objects.detailColHSVEnabled = objects_bool_block + 6 * count;
    objects.cachedMainHSVValid = objects_bool_block + 7 * count;
    objects.cachedDetailHSVValid = objects_bool_block + 8 * count;

    objects.groups = objects_groups_block;

    objects.groupCount = objects_u8_block + 0 * count;
    objects.flags = objects_u8_block + 1 * count;
    objects.activated = objects_u8_block + 2 * count;
    objects.collided = objects_u8_block + 3 * count;
    objects.numMainPulses = objects_u8_block + 4 * count;
    objects.numDetailPulses = objects_u8_block + 5 * count;

    objects.mainNonPulseColor = objects_color_block + 0 * count;
    objects.detailNonPulseColor = objects_color_block + 1 * count;
    objects.mainColor = objects_color_block + 2 * count;
    objects.detailColor = objects_color_block + 3 * count;
    objects.cachedMainHSVSrcColor = objects_color_block + 4 * count;
    objects.cachedDetailHSVSrcColor = objects_color_block + 5 * count;
    objects.cachedMainHSVColor = objects_color_block + 6 * count;
    objects.cachedDetailHSVColor = objects_color_block + 7 * count;

    objects.mainColHSV = objects_hsv_block + 0 * count;
    objects.detailColHSV = objects_hsv_block + 1 * count;

    // Initialize the values
    memset(objects.random,             0, sizeof(int) * count);
    memset(objects.id,                 0, sizeof(int) * count);
    memset(objects.x,                  0, sizeof(float) * count);
    memset(objects.y,                  0, sizeof(float) * count);
    memset(objects.rotation,           0, sizeof(float) * count);
    memset(objects.visualRotation,    0, sizeof(float) * count);
    memset(objects.zlayer,             0, sizeof(int) * count);
    memset(objects.zorder,             0, sizeof(int) * count);
    memset(objects.opacity,            0, sizeof(float) * count);
    memset(objects.alphaTriggerOpacity, 0, sizeof(float) * count);
    memset(objects.width,              0, sizeof(float) * count);
    memset(objects.height,             0, sizeof(float) * count);
    memset(objects.v1p9ColChannel,   0, sizeof(unsigned short) * count);
    memset(objects.colChannel,        0, sizeof(unsigned short) * count);
    memset(objects.detailColChannel, 0, sizeof(unsigned short) * count);
    memset(objects.triggerIndex,      0, sizeof(int) * count);
    memset(objects.hitboxCounter,     0, sizeof(unsigned short) * count);
    memset(objects.transitionApplied, 0, sizeof(unsigned char) * count);
    memset(objects.orientation,        0, sizeof(unsigned char) * count);
    memset(objects.touchTriggered,    0, sizeof(bool) * count);
    memset(objects.flippedH,           0, sizeof(bool) * count);
    memset(objects.flippedV,           0, sizeof(bool) * count);
    memset(objects.groups,             0, sizeof(short[MAX_GROUPS_PER_OBJECT]) * count);
    memset(objects.groupCount,        0, sizeof(u8) * count);
    memset(objects.flags,              0, sizeof(u8) * count);
    memset(objects.activated,          0, sizeof(u8) * count);
    memset(objects.collided,           0, sizeof(u8) * count);

    for (size_t i = 0; i < count; i++) {
        objects.alphaTriggerOpacity[i] = 1.0f;
        objects.scaleX[i] = 1.0f;
        objects.scaleY[i] = 1.0f;
        objects.originalScaleX[i] = 1.0f;
        objects.originalScaleY[i] = 1.0f;
    }

    memset(objects.numMainPulses,    0, sizeof(u8) * count);
    memset(objects.numDetailPulses,  0, sizeof(u8) * count);
    memset(objects.mainBeingPulsed,  0, sizeof(bool) * count);
    memset(objects.detailBeingPulsed,0, sizeof(bool) * count);

    for (size_t i = 0; i < count; i++) {
        objects.mainNonPulseColor[i].r = 255;
        objects.mainNonPulseColor[i].g = 255;
        objects.mainNonPulseColor[i].b = 255;
        objects.detailNonPulseColor[i].r = 255;
        objects.detailNonPulseColor[i].g = 255;
        objects.detailNonPulseColor[i].b = 255;
        objects.mainColor[i].r = 255;
        objects.mainColor[i].g = 255;
        objects.mainColor[i].b = 255;
        objects.detailColor[i].r = 255;
        objects.detailColor[i].g = 255;
        objects.detailColor[i].b = 255;
    }

    memset(objects.mainColHSVEnabled,   0, sizeof(bool) * count);
    memset(objects.detailColHSVEnabled, 0, sizeof(bool) * count);
    memset(objects.mainColHSV,           0, sizeof(HSV) * count);
    memset(objects.detailColHSV,         0, sizeof(HSV) * count);
    memset(objects.cachedMainHSVSrcColor,  0, sizeof(Color) * count);
    memset(objects.cachedDetailHSVSrcColor, 0, sizeof(Color) * count);
    memset(objects.cachedMainHSVColor,       0, sizeof(Color) * count);
    memset(objects.cachedDetailHSVColor,     0, sizeof(Color) * count);
    memset(objects.cachedMainHSVValid,       0, sizeof(bool) * count);
    memset(objects.cachedDetailHSVValid,     0, sizeof(bool) * count);

    for (size_t i = 0; i < count; i++) {
        objects.childObject[i] = -1;
        objects.tpYOffset[i] = 0.0f;
    }

    memset(objects.sectionX,           0, sizeof(int) * count);
    memset(objects.sectionY,           0, sizeof(int) * count);
    memset(objects.groupCombinationIndex, 0, sizeof(int) * count);

    return true;
}

int compareCoins(const void *a, const void *b) {
    const int *c1 = (const int *)a;
    const int *c2 = (const int *)b;

    return objects.x[*c1] - objects.x[*c2];
}

int parseString(char *levelString, int extraSlots, int *outObjectCount) {
    int sectionCount = 0;

    // Count sections
    const char *sectionStart = levelString;
    char *cursor = levelString;
    int highDetailCount = 0;
    while(true) {
        if (*cursor == ';' || *cursor == '\0') {
            if (cursor > sectionStart) {
                if (sectionCount > 0 && isHighDetailObject(sectionStart, cursor, NULL)) {
                    highDetailCount++;
                }
                sectionCount++;
            }
            if (*cursor == '\0') break;
            sectionStart = cursor + 1;
        }
        cursor++;
    }

    if (sectionCount < 1) {
        outputLog("Level string missing sections!\n");
        return LOAD_LEVEL_STRING_MISSING_SECTIONS;
    }
    
    int objectCount = sectionCount - 1;
    int parseCapacity = objectCount - highDetailCount;
    int allocCount = parseCapacity + extraSlots;

    if (!initArrays(allocCount)) {
        freeArrays();
        outputLog("Failed to allocate object array\n");
        return LOAD_OUT_OF_MEMORY;
    }

    objects.count = allocCount;

    printf("Parsing string and converting objects...\n");
    printf("Aproximately %d bytes of pure objects\n", sizeof(ObjectsArray) * allocCount);
    
    coinCount = 0;

    sectionStart = levelString;
    int sectionIndex = 0;
    int objectIndex = 0;

    cursor = levelString;
    while (true) {
        if (*cursor == ';' || *cursor == '\0') {
            bool atEnd = *cursor == '\0';
            if (cursor > sectionStart) {
                if (!atEnd) *cursor = '\0';
                if (sectionIndex > 0) {
                    // Parse
                    int parsed = parseGdObject(sectionStart, cursor, objectIndex, parseCapacity);
                    if (parsed == -2) {
                        outputLog("Object capacity exceeded\n");
                        freeArrays();
                        return LOAD_OUT_OF_MEMORY;
                    }
                    if (parsed > 0) {
                        assignObjectToSection(objectIndex);
                        objectIndex++;
                    } else if (!parsed) {
                        outputLog("Failed to parse object %d\n", objectIndex);
                        freeArrays();
                        return LOAD_COULDNT_PARSE_OBJECTS;
                    }
                }
                sectionIndex++;
            }
            if (atEnd) break;
            sectionStart = cursor + 1;
        }
        cursor++;
    }

    objects.count = objectIndex + extraSlots;

    if (outObjectCount) *outObjectCount = objectIndex;

    for (int i = 0; i < objectIndex; i++) {
        for (int g = 0; g < objects.groupCount[i]; g++) {
            int groupID = objects.groups[i][g];
            if (groupID > 0) {
                addToGroup(i, groupID);
            }
        }
    }

    qsort(coinIds, coinCount, sizeof(int), compareCoins);

    // Set IDs here
    for (int i = 0; i < coinCount; i++) {
        objects.coinID[coinIds[i]] = i;
        //output_log("Le coin %d on objeto %d\n", i, coin_ids[i]);
    }
    
    levelInfo.lastObjX += (11 * 30.f);

    for (int i = 1; i < MAX_GROUPS; i++) {
        sortGroup(i);
    }

    return LOAD_NO_ERROR;
}

static int streamLevelInput(const LevelStreamInput *input, LevelStreamCallback callback, void *context) {
    // Check if uncompressed
    if (input->rawData) {
        return callback((const unsigned char *)input->rawData, strlen(input->rawData), context);
    }

    z_stream stream = {0};
    stream.next_in = Z_NULL;
    stream.avail_in = 0;
    if (inflateInit2(&stream, 15 | 32) != Z_OK) {
        return LOAD_INVALID_COMPRESSED_DATA;
    }

    unsigned char buffer[4096];
    size_t inputOffset = 0;
    int result = LOAD_INVALID_COMPRESSED_DATA;
    bool complete = false;

    while (!complete) {
        if (stream.avail_in == 0 && inputOffset < input->compressedLen) {
            // Give more data to zlib
            size_t chunkLen = input->compressedLen - inputOffset;
            stream.next_in = input->compressedData + inputOffset;
            stream.avail_in = chunkLen;
            inputOffset += chunkLen;
        }

        // Decompress it
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);

        uInt beforeIn = stream.avail_in;
        int ret = inflate(&stream, inputOffset == input->compressedLen ? Z_FINISH : Z_NO_FLUSH);

        // Send to callback
        size_t produced = sizeof(buffer) - stream.avail_out;
        if (produced) {
            result = callback(buffer, produced, context);
            if (result != LOAD_NO_ERROR) break;
        }

        if (ret == Z_STREAM_END) {
            // Success! yay
            complete = true;
            result = LOAD_NO_ERROR;
        } else if (ret != Z_OK && ret != Z_BUF_ERROR) {
            // Fail :(
            result = LOAD_INVALID_COMPRESSED_DATA;
            break;
        } else if (beforeIn == stream.avail_in && produced == 0 && 
                (stream.avail_in > 0 ||
                (stream.avail_in == 0 && inputOffset == input->compressedLen))) {
            // Avoid decompression loop
            result = LOAD_INVALID_COMPRESSED_DATA;
            break;
        }
    }

    inflateEnd(&stream);
    return result;
}

static bool reserveCharBuffer(char **buffer, size_t *capacity, size_t needed) {
    if (needed <= *capacity) return true;

    size_t newCapacity = *capacity ? *capacity : 256;
    if (newCapacity < needed) {
        newCapacity = needed;
    }

    char *section = (char *)realloc(*buffer, newCapacity);
    if (!section) return false;

    *buffer = section;
    *capacity = newCapacity;
    return true;
}

static int finishLevelScanSection(LevelStreamScan *scan, bool terminated) {
    if (scan->sectionLen > 0) {
        if (scan->sectionCount == 0) {
            // Handle metadata
            size_t metadataLen = scan->sectionLen + (terminated ? 1 : 0);

            scan->metadata = (char *)malloc(metadataLen + 1);
            if (!scan->metadata) return LOAD_OUT_OF_MEMORY;

            memcpy(scan->metadata, scan->section, scan->sectionLen);

            if (terminated) scan->metadata[scan->sectionLen] = ';';
            scan->metadata[metadataLen] = '\0';
        // Count high detail objects
        } else if (isHighDetailObject(scan->section, scan->section + scan->sectionLen, NULL)) {
            scan->highDetailCount++;
        }
        scan->sectionCount++;
    }
    scan->sectionLen = 0;
    return LOAD_NO_ERROR;
}

static int scanLevelChunk(const unsigned char *data, size_t len, void *context) {
    static const char portalPattern[] = "1,747,";
    LevelStreamScan *scan = (LevelStreamScan *)context;

    for (size_t i = 0; i < len; i++) {
        char ch = (char)data[i];

        if (ch == ';') {
            // Section end
            int result = finishLevelScanSection(scan, true);
            if (result != LOAD_NO_ERROR) return result;
        } else {
            if (!reserveCharBuffer(&scan->section, &scan->sectionCapacity, scan->sectionLen + 1)) 
                return LOAD_OUT_OF_MEMORY;

            scan->section[scan->sectionLen++] = ch;
        }

        // Additional slots for tp portal
        if (ch == portalPattern[scan->portalMatch]) {
            scan->portalMatch++;
            if (portalPattern[scan->portalMatch] == '\0') {
                scan->extraSlots++;
                scan->portalMatch = 0;
            }
        } else {
            scan->portalMatch = ch == portalPattern[0] ? 1 : 0;
        }
    }

    return LOAD_NO_ERROR;
}

static void freeLevelStreamScan(LevelStreamScan *scan) {
    free(scan->section);
    free(scan->metadata);
}

static int parseStreamSection(LevelStreamParser *parser) {
    // Check for nothing
    if (!parser->sectionLen) return LOAD_NO_ERROR;

    parser->section[parser->sectionLen] = '\0';

    if (parser->sectionCount > 0) {
        int parsed = parseGdObject(parser->section, parser->section + parser->sectionLen, parser->objectIndex, parser->parseCapacity);
        if (parsed == -2) {
            outputLog("Object capacity exceeded\n");
            return LOAD_OUT_OF_MEMORY;
        }
        
        if (parsed == 0) {
            outputLog("Failed to parse object %d\n", parser->objectIndex);
            return LOAD_COULDNT_PARSE_OBJECTS;
        }

        if (parsed > 0) {
            assignObjectToSection(parser->objectIndex);
            parser->objectIndex++;
        }
    }

    parser->sectionCount++;
    parser->sectionLen = 0;
    return LOAD_NO_ERROR;
}

static int parseLevelChunk(const unsigned char *data, size_t len, void *context) {
    LevelStreamParser *parser = (LevelStreamParser *)context;

    for (size_t i = 0; i < len; i++) {
        if (data[i] == ';') {
            // Section end
            int result = parseStreamSection(parser);
            if (result != LOAD_NO_ERROR) return result;
        } else {
            // Reserve enough capacity
            if (!reserveCharBuffer(&parser->section, &parser->sectionCapacity, parser->sectionLen + 2)) {
                return LOAD_OUT_OF_MEMORY;
            }

            // Append
            parser->section[parser->sectionLen++] = (char)data[i];
        }
    }

    return LOAD_NO_ERROR;
}

static int parseLevelStream(const LevelStreamInput *input, const LevelStreamScan *scan, int *outObjectCount) {
    if (scan->sectionCount < 1) {
        outputLog("Level string missing sections!\n");
        return LOAD_LEVEL_STRING_MISSING_SECTIONS;
    }

    int objectCount = scan->sectionCount - 1;
    int parseCapacity = objectCount - scan->highDetailCount;

    int allocCount = parseCapacity + scan->extraSlots;
    if (allocCount < 0 || !initArrays((size_t)allocCount)) {
        freeArrays();
        outputLog("Failed to allocate object array\n");
        return LOAD_OUT_OF_MEMORY;
    }

    objects.count = allocCount;
    coinCount = 0;

    // Initialize parser
    LevelStreamParser parser = {0};
    parser.parseCapacity = parseCapacity;

    int result = streamLevelInput(input, parseLevelChunk, &parser);

    // Parse last section, because we check for ; to mark the end of a section
    if (result == LOAD_NO_ERROR) result = parseStreamSection(&parser);

    free(parser.section);

    if (result != LOAD_NO_ERROR) {
        freeArrays();
        return result;
    }

    // Update object count
    objects.count = parser.objectIndex + scan->extraSlots;
    if (outObjectCount) *outObjectCount = parser.objectIndex;

    // Add groupma
    for (int i = 0; i < parser.objectIndex; i++) {
        for (int g = 0; g < objects.groupCount[i]; g++) {
            int groupID = objects.groups[i][g];
            if (groupID > 0) {
                addToGroup(i, groupID);
            }
        }
    }

    qsort(coinIds, coinCount, sizeof(int), compareCoins);

    for (int i = 0; i < coinCount; i++) {
        objects.coinID[coinIds[i]] = i;
        //output_log("Le coin %d on objeto %d\n", i, coin_ids[i]);
    }
    levelInfo.lastObjX += (11 * 30.f);

    for (int i = 1; i < MAX_GROUPS; i++) {
        sortGroup(i);
    }
    return LOAD_NO_ERROR;
}

static void generateOrangePortals(int orangeStart);
void setColorChannels(void);
void loadOnlineLevelInfo(char *levelString);
void loadLevelInfo(char *data, char *levelString);

static int loadLevelStream(const LevelStreamInput *input, char *gmdData, bool online) {
    LevelStreamScan scan = {0};
    levelInfo.lowDetail = currentLevelEntry ? currentLevelEntry->data.lowDetail : false;

    // Scan the level, we need to know how many objects to parse
    int result = streamLevelInput(input, scanLevelChunk, &scan);
    if (result == LOAD_NO_ERROR) {
        result = finishLevelScanSection(&scan, false);
    } else {
        freeLevelStreamScan(&scan);
        return result;
    }

    free(scan.section);
    scan.section = NULL;
    scan.sectionCapacity = 0;
    scan.sectionLen = 0;
    if (scan.sectionCount < 1) {
        freeLevelStreamScan(&scan);
        outputLog("Level string missing sections!\n");
        return LOAD_LEVEL_STRING_MISSING_SECTIONS;
    }

    char *metaStr = getMetadataValue(scan.metadata, "kS38");

    int outCode = LOAD_NO_ERROR;
    if (metaStr) {
        channelCount = parseColorChannels(metaStr, &colorChannels, &outCode);
    } else {
        channelCount = parseOldChannels(scan.metadata, &colorChannels, &outCode);
    }

    if (outCode) {
        free(metaStr);
        freeLevelStreamScan(&scan);
        return outCode;
    }

    if (online) loadOnlineLevelInfo(scan.metadata);
    else loadLevelInfo(gmdData, scan.metadata);

    levelInfo.lastObjX = 570.f;

    // Start parsing
    int orangeStart = 0;
    result = parseLevelStream(input, &scan, &orangeStart);

    free(metaStr);
    freeLevelStreamScan(&scan);

    if (result != LOAD_NO_ERROR) return result;

    generateOrangePortals(orangeStart);

    if (!buildGroupCombinations()) {
        outputLog("Failed to build group combinations\n");
        return LOAD_OUT_OF_MEMORY;
    }

    initColChannels();
    setColorChannels();

    currentPulserodBallImage = gameObjects[15].children[0].texture + (rand() % 3);

    C2D_SpriteFromSheet(&spriteTemplates[15].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[15].childTemplates[0], 0.5f, 0.5f);
    C2D_SpriteFromSheet(&spriteTemplates[16].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[16].childTemplates[0], 0.5f, 0.5f);
    C2D_SpriteFromSheet(&spriteTemplates[17].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[17].childTemplates[0], 0.5f, 0.5f);

    return LOAD_NO_ERROR;
}

void setColorChannels() {
    for (int i = 0; i < channelCount; i++) {
        GDColorChannel colorChannel = colorChannels[i];
        int id = colorChannel.channelID;

        if (id == CHANNEL_BLACK) continue;

        switch (id) {
            case CHANNEL_P1:
            case CHANNEL_P2:
                break;

            default:
                if (id >= 0 && id < COL_CHANNEL_NUM) {
                    int chan = getColChannelIndex(id);

                    Color color;
                    color.r = colorChannel.fromRed;
                    color.g = colorChannel.fromGreen;
                    color.b = colorChannel.fromBlue;

                    channels[chan].blending = colorChannel.blending;
                    channels[chan].color = color;
                    channels[chan].nonPulseColor = color;
                    channels[chan].alpha = colorChannel.fromOpacity < 0.0f ? 0.0f : (colorChannel.fromOpacity > 1.0f ? 1.0f : colorChannel.fromOpacity);

                    if (colorChannel.inheritedChannelID > 0) {
                        channels[chan].copyColorID = colorChannel.inheritedChannelID;
                        channels[chan].hsv = colorChannel.hsv;
                    } else {
                        channels[chan].copyColorID = 0;
                    }

                    if (colorChannel.playerColor == 1) {
                        channels[chan].color = getP2IfBlack(p1Color);
                        channels[chan].nonPulseColor = channels[chan].color;
                    }
                    if (colorChannel.playerColor == 2) {
                        channels[chan].color = getP1IfBlack(p2Color);
                        channels[chan].nonPulseColor = channels[chan].color;
                    }

                    if (id == CHANNEL_OBJ) {
                        channels[getColChannelIndex(CHANNEL_OBJ_BLENDING)].color = color;
                        channels[getColChannelIndex(CHANNEL_OBJ_BLENDING)].nonPulseColor = color;
                    }
                }
        }
    }

    for (int i = 0; i < COL_CHANNEL_NUM; i++)
        channels[i].nonPulseColor = channels[i].color;

    loadUsedChannels();
}

const char *bgSheetPaths[] = {
    "romfs:/gfx/bg_sheet_01.t3x",
    "romfs:/gfx/bg_sheet_02.t3x",
    "romfs:/gfx/bg_sheet_03.t3x",
    "romfs:/gfx/bg_sheet_04.t3x",
    "romfs:/gfx/bg_sheet_05.t3x"
};

const char *levelFontPaths[LEVEL_FONT_COUNT] = {
    NULL,
    "romfs:/gfx/gjFont01.t3x",
    "romfs:/gfx/gjFont02.t3x",
    "romfs:/gfx/gjFont03.t3x",
    "romfs:/gfx/gjFont04.t3x",
    "romfs:/gfx/gjFont05.t3x",
    "romfs:/gfx/gjFont06.t3x",
    "romfs:/gfx/gjFont07.t3x",
    "romfs:/gfx/gjFont08.t3x",
    "romfs:/gfx/gjFont09.t3x",
    "romfs:/gfx/gjFont10.t3x",
    "romfs:/gfx/gjFont11.t3x",
    "romfs:/gfx/gjFont12.t3x"
};

int loadedLevelFont = 0;
C2D_SpriteSheet levelFontSheet = NULL;
const Charset *levelFont = &bigFontFontCharset;

void loadLevelStringInfo(char *levelString) {
    levelInfo.lowDetail = false;

    char *gmdSongOffset = getMetadataValue(levelString, "kA13");
    if (gmdSongOffset) {
        levelInfo.songOffset = atof(gmdSongOffset);
        free(gmdSongOffset);
    } else {
        levelInfo.songOffset = 0;
    }

    char *backgroundData = getMetadataValue(levelString, "kA6");
    if (backgroundData) {
        levelInfo.backgroundID = CLAMP(atoi(backgroundData) - 1, 0, BG_COUNT - 1);
        free(backgroundData);
    } else {
        levelInfo.backgroundID = 0;
    }

    // load the background sheet on demand
    int newSheet = levelInfo.backgroundID / 4;
    if (loadedBgSheet != newSheet) {
        C2D_SpriteSheetFree(bgSheet);
        bgSheet = C2D_SpriteSheetLoad(bgSheetPaths[newSheet]);
        loadedBgSheet = newSheet;
    }

    char *groundData = getMetadataValue(levelString, "kA7");
    if (groundData) {
        levelInfo.groundID = CLAMP(atoi(groundData) - 1, 0, G_COUNT - 1);
        free(groundData);
    } else {
        levelInfo.groundID = 0;
    }
    
    char *gamemodeData = getMetadataValue(levelString, "kA2");
    if (gamemodeData) {
        levelInfo.initialGamemode = CLAMP(atoi(gamemodeData), 0, GAMEMODE_COUNT - 1);
        free(gamemodeData);
    } else {
        levelInfo.initialGamemode = GAMEMODE_PLAYER;
    }

    char *miniData = getMetadataValue(levelString, "kA3");
    if (miniData) {
        levelInfo.initialMini = atoi(miniData) != 0;    
        free(miniData);
    } else {
        levelInfo.initialMini = 0; 
    }

    char *speedData = getMetadataValue(levelString, "kA4");
    if (speedData) {
        levelInfo.initialSpeed = CLAMP(atoi(speedData), 0, SPEED_COUNT - 1);
        if (levelInfo.initialSpeed == 0) levelInfo.initialSpeed = SPEED_NORMAL;
        else if (levelInfo.initialSpeed == 1) levelInfo.initialSpeed = SPEED_SLOW;
        free(speedData);
    } else {
        levelInfo.initialSpeed = SPEED_NORMAL;
    }

    char *dualData = getMetadataValue(levelString, "kA8");
    if (dualData) {
        levelInfo.initialDual = atoi(dualData) != 0;
        free(dualData);
    } else {
        levelInfo.initialDual = 0; 
    }
    
    char *twoPlayerModeData = getMetadataValue(levelString, "kA10");
    if (twoPlayerModeData) {
        levelInfo.twoPlayerMode = atoi(twoPlayerModeData) != 0;
        free(twoPlayerModeData);
    } else {
        levelInfo.twoPlayerMode = 0; 
    }

    char *upsidedownData = getMetadataValue(levelString, "kA11");
    if (upsidedownData) {
        levelInfo.initialUpsidedown = atoi(upsidedownData) != 0;
        free(upsidedownData);
    } else {
        levelInfo.initialUpsidedown = 0; 
    }

    // load the level font on demand (0 = bigFont)
    char *fontData = getMetadataValue(levelString, "kA18");
    int fontIndex = 0;
    if (fontData) {
        fontIndex = CLAMP(atoi(fontData), 0, LEVEL_FONT_COUNT - 1);
        free(fontData);
    }

    if (loadedLevelFont != fontIndex) {
        if (levelFontSheet) {
            C2D_SpriteSheetFree(levelFontSheet);
            levelFontSheet = NULL;
        }
        if (fontIndex > 0) {
            levelFontSheet = C2D_SpriteSheetLoad(levelFontPaths[fontIndex]);
            if (!levelFontSheet) fontIndex = 0;
        }
        loadedLevelFont = fontIndex;
    }
    levelFont = levelFontCharsets[fontIndex];
    
}

const char *defaultName = "Unknown";

void loadOnlineLevelInfo(char *levelString) {
    loadLevelStringInfo(levelString);

    levelInfo.lowDetail = currentLevelEntry ? currentLevelEntry->data.lowDetail : false;

    levelInfo.songID = currentSearchEntry->mainSongId;
    levelInfo.customSongID = currentSearchEntry->songId;
    snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", currentSearchEntry->name);
    snprintf(levelInfo.creatorName, sizeof(levelInfo.levelName), "%s", (currentCreatorEntry ? currentCreatorEntry->creatorName : "-"));
}

static void generateOrangePortals(int orangeStart) {
    int nextOrange = orangeStart;
    for (int i = 0; i < orangeStart; i++) {
        if (objects.id[i] != BLUE_TP_PORTAL) continue;

        float angleRad = objects.rotation[i] * (M_PI / 180.0f);
        float xOff = 10.0f * fabsf(cosf(angleRad));

        int oi = nextOrange++;
        objects.id[oi]           = ORANGE_TP_PORTAL;
        objects.x[oi]            = objects.x[i] - xOff;
        objects.y[oi]            = objects.y[i] + objects.tpYOffset[i];
        objects.lastY[oi]       = objects.y[oi];
        objects.rotation[oi]     = adjustAngleY(objects.rotation[i], objects.flippedH[i]) + 180.0f;
        objects.visualRotation[oi] = objects.rotation[oi];
        objects.flippedH[oi]     = false;
        objects.flippedV[oi]     = false;
        objects.opacity[oi]      = 1.0f;
        objects.flags[oi]        &= ~FLAG_TOGGLED;
        objects.activated[oi]    = 0;
        objects.collided[oi]     = 0;
        objects.childObject[oi] = -1;
        objects.tpYOffset[oi]  = 0.0f;
        objects.originalX[oi]   = objects.x[oi];
        objects.originalY[oi]   = objects.y[oi];
        objects.zlayer[oi]       = objects.zlayer[i];
        objects.zorder[oi]       = objects.zorder[i];
        objects.childObject[i]  = oi;

        // Copy groups from blue portal to orange portal
        objects.groupCount[oi] = objects.groupCount[i];
        for (int g = 0; g < objects.groupCount[i] && g < MAX_GROUPS_PER_OBJECT; g++) {
            objects.groups[oi][g] = objects.groups[i][g];
            if (objects.groups[i][g] > 0) {
                addToGroup(oi, objects.groups[i][g]);
            }
        }

        assignObjectToSection(oi);
    }
    objects.count = nextOrange;
}

int loadOnlineLevel(char *levelString) {
    arenaReset(&levelArena);

    LevelStreamInput input = {0};
    unsigned char *decoded = NULL;
    if (strchr(levelString, ';')) {
        // String is not even compressed
        input.rawData = levelString;
    } else {
        fixBase64URL(levelString);
        removeChar(levelString, ',');

        size_t encodedLen = strlen(levelString);
        decoded = (unsigned char *)malloc(encodedLen);
        if (!decoded) return LOAD_OUT_OF_MEMORY;

        int decodedLen = base64Decode(levelString, decoded);
        if (decodedLen <= 0) {
            outputLog("Failed to decode base64\n");
            free(decoded);
            return LOAD_INVALID_BASE64;
        }
        input.compressedData = decoded;
        input.compressedLen = (size_t)decodedLen;
    }

    int result = loadLevelStream(&input, NULL, true);
    free(decoded);
    return result;
}

void loadLevelInfo(char *data, char *levelString) {
    loadLevelStringInfo(levelString);

    levelInfo.lowDetail = currentLevelEntry ? currentLevelEntry->data.lowDetail : false;

    char *gmdSongID = extractGMDKey((const char *) data, "k8", "i");
    if (!gmdSongID) {
        levelInfo.songID = 0; // Stereo Madness
    } else {
        levelInfo.songID = atoi(gmdSongID); // Official song id
        free(gmdSongID);
    }

    char *gmdCustomSongID = extractGMDKey((const char *) data, "k45", "i");
    if (!gmdCustomSongID) {
        levelInfo.customSongID = -1;
    } else {
        levelInfo.customSongID = atoi(gmdCustomSongID); // Custom song id
        free(gmdCustomSongID);
    }

    char *levelNameData = extractGMDKey((const char *) data, "k2", "s");
    if (levelNameData) {
        snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", levelNameData);
        free(levelNameData);
    } else {
        snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", defaultName);
    }

    char *creatorNameData = extractGMDKey((const char *) data, "k5", "s");
    if (creatorNameData) {
        snprintf(levelInfo.creatorName, sizeof(levelInfo.creatorName), "%s", creatorNameData);
        free(creatorNameData);
    } else {
        snprintf(levelInfo.creatorName, sizeof(levelInfo.creatorName), "%s", defaultName);
    }
}

int loadLevel(const char *path) {
    int outCode = LOAD_NO_ERROR;

    arenaReset(&levelArena);

    size_t out;
    char *level = readFile(path, &out);
    if (!level) return 1;

    level = convertIfGdlvl(level, path);
    if (!level) {
        return LOAD_OUT_OF_MEMORY;
    }

    char *b64 = extractGMDKey((const char *)level, "k4", "s");
    if (b64) {
        LevelStreamInput input = {0};
        unsigned char *decoded = NULL;

        if (strchr(b64, ';')) {
            // Not even compressed
            input.rawData = b64;
        } else {
            fixBase64URL(b64);
            size_t encodedLen = strlen(b64);
            decoded = (unsigned char *)malloc(encodedLen);
            if (!decoded) {
                free(b64);
                free(level);
                return LOAD_OUT_OF_MEMORY;
            }

            int decodedLen = base64Decode(b64, decoded);
            if (decodedLen <= 0) {
                outputLog("Failed to decode base64\n");
                free(decoded);
                free(b64);
                free(level);
                return LOAD_INVALID_BASE64;
            }
            input.compressedData = decoded;
            input.compressedLen = (size_t)decodedLen;
        }

        int result = loadLevelStream(&input, level, false);
        free(decoded);
        free(b64);
        free(level);
        return result;
    }

    // Empty level

    char *data = decompressLevel(level, &outCode);
    if (!data) {
        free(level);
        return outCode;
    }

    // Get level starting colors
    char *metaStr = getMetadataValue(data, "kS38");
    if (metaStr) {
        channelCount = parseColorChannels(metaStr, &colorChannels, &outCode);
        if (outCode) {
            free(level);
            free(metaStr);
            return outCode;
        }
    }
    // Fallback to pre 2.0 color keys
    else {
        channelCount = parseOldChannels(data, &colorChannels, &outCode);
        if (outCode) {
            free(level);
            return outCode;
        }
    }

    loadLevelInfo(level, data);
    free(level);

    // Minimum size
    levelInfo.lastObjX = 570.f;

    // Count blue teleport portals to reserve extra object slots
    int extraSlots = 0;
    const char *scan = data;
    while ((scan = strstr(scan, "1,747,")) != NULL) {
        extraSlots++;
        scan++;
    }
    int orangeStart = 0;

    int returned = parseString(data, extraSlots, &orangeStart);

    free(metaStr);

    if (returned) return returned;

    generateOrangePortals(orangeStart);
    
    if (!buildGroupCombinations()) {
        outputLog("Failed to build group combinations\n");
        return LOAD_OUT_OF_MEMORY;
    }

    initColChannels();
    setColorChannels();

    // Set pulserod pulse ball image
    currentPulserodBallImage = gameObjects[15].children[0].texture + (rand() % 3);

    C2D_SpriteFromSheet(&spriteTemplates[15].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[15].childTemplates[0], 0.5f, 0.5f);
    
    C2D_SpriteFromSheet(&spriteTemplates[16].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[16].childTemplates[0], 0.5f, 0.5f);

    C2D_SpriteFromSheet(&spriteTemplates[17].childTemplates[0], spriteSheet, currentPulserodBallImage);
    C2D_SpriteSetCenter(&spriteTemplates[17].childTemplates[0], 0.5f, 0.5f);

    return LOAD_NO_ERROR;
}

void reloadLevel() {
    clearOnDeathTriggers();

    freeTriggerBuffers();

    for (int i = 1; i < MAX_GROUPS; i++) {
        resetGroupData(i);
    }

    for (int i = 0; i < objects.count; i++) {
        objects.activated[i] = false;
        objects.collided[i] = false;
        objects.hitboxCounter[i] = 0;
        objects.transitionApplied[i] = FADE_NONE;
        objects.flags[i] &= ~FLAG_TOGGLED;
        objects.opacity[i] = 1.f;
        objects.x[i] = objects.originalX[i];
        objects.y[i] = objects.originalY[i];
        objects.lastY[i] = objects.originalY[i];
        objects.alphaTriggerOpacity[i] = 1.0f;
        objects.scaleX[i] = objects.originalScaleX[i];
        objects.scaleY[i] = objects.originalScaleY[i];
        objects.numMainPulses[i] = 0;
        objects.numDetailPulses[i] = 0;
        objects.mainBeingPulsed[i] = false;
        objects.detailBeingPulsed[i] = false;
        objects.mainColor[i] = (Color){255, 255, 255};
        objects.detailColor[i] = (Color){255, 255, 255};
        objects.childObject[i] = -1;
        objects.tpYOffset[i] = 0.0f;
        objects.cachedMainHSVValid[i] = false;
        objects.cachedDetailHSVValid[i] = false;
    }

    for (int i = 0; i < objects.count; i++) {
        updateObjectSection(i);
    }

    int nextOrange = 0;
    for (int i = 0; i < objects.count; i++) {
        if (objects.id[i] == BLUE_TP_PORTAL) {
            while (nextOrange < objects.count && objects.id[nextOrange] != ORANGE_TP_PORTAL)
                nextOrange++;
            if (nextOrange < objects.count) {
                objects.childObject[i] = nextOrange;
                nextOrange++;
            }
        }
    }

    accumulator = 0.f;
    fixedDt = true;

    initColChannels();
    setColorChannels();
    
    // Run triggers behind the start
    handleTriggers();
}

void unloadLevel() {
    clearOnDeathTriggers();
    clearGroups();
    freeArrays();
    freeTriggerBuffers();
    freeSections();
    freeObjectParticles();
    initColChannels();

    arenaReset(&levelArena);
    
    channelCount = 0;
    free(colorChannels);
    colorChannels = NULL;

    if (levelFontSheet) {
        C2D_SpriteSheetFree(levelFontSheet);
        levelFontSheet = NULL;
    }
    loadedLevelFont = 0;
    levelFont = &bigFontFontCharset;

    stopMP3();
}

char *getLevelName(char *dataPtr) {
    return extractGMDKey((const char *) dataPtr, "k2", "s");
}

char *loadUserSong(int id, size_t *outSize) {
    char fullPath[273];
    snprintf(fullPath, sizeof(fullPath), "%s%d.mp3", USER_SONGS_DIR, id);
    return readFileArena(&levelArena, fullPath, outSize);
}

bool checkSong(int id) {
    char fullPath[273];
    snprintf(fullPath, sizeof(fullPath), "%s%d.mp3", USER_SONGS_DIR, id);
    return access(fullPath, F_OK) == 0;
}

char *convertIfGdlvl(char *gmd, const char *path) {
    const char *extension = getFilenameExt(path);
    if (strcmp(extension, "gdlvl") == 0) {
        unsigned char *decoded = (unsigned char *)malloc(strlen(gmd));
        if (!decoded) {
            free(gmd);
            return NULL;
        }

        fixBase64URL(gmd);
        int decodedLen = base64Decode(gmd, decoded);
        if (decodedLen <= 0) {
            outputLog("Failed to decode base64\n");
            free(gmd);
            free(decoded);
            return NULL;
        }

        size_t decompressedLen;
        int outCode;
        char *decompressed = decompressData(decoded, decodedLen, &decompressedLen, &outCode);
        if (!decompressed) {
            outputLog("Decompression failed (check zlib error above)\n");
            free(gmd);
            free(decoded);
            return NULL;
        }
        free(gmd);
        free(decoded);
        return decompressed;
    }
    return gmd;
}