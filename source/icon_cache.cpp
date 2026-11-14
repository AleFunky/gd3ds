#include "icon_cache.hpp"

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "icons.hpp"
#include "menus/icon_kit.hpp"
#include "main.hpp"

#define ICON_CACHE_CAPACITY 64
#define ICON_ARCHIVE_MAGIC 0x4E434947u

namespace gd3ds
{

struct IconCacheEntry {
    int key;
    C2D_SpriteSheet sheet;
    bool pinned;
    bool used;
};

struct IconArchiveEntry {
    u32 offset;
    u32 length;
};

static IconCacheEntry cache[ICON_CACHE_CAPACITY];
static int iconPrefix[ICON_GAMEMODE_COUNT];
static IconArchiveEntry *iconCacheEntries;
static int entryCount;
static s64 archiveSize;
static FILE *iconCacheArchive;

static int iconKey(int gamemode, int id) {
    if (gamemode < 0 || gamemode >= ICON_GAMEMODE_COUNT) return -1;
    if (id < 0 || id >= gamemodeIconCount[gamemode]) return -1;
    return iconPrefix[gamemode] + id;
}

static int iconCacheFind(int key) {
    if (key < 0) return -1;
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (cache[i].sheet && cache[i].key == key) return i;
    }
    return -1;
}

static C2D_SpriteSheet iconCacheLoad(int key) {
    if (!iconCacheArchive || !iconCacheEntries || key < 0 || key >= entryCount) return NULL;

    u32 offset = iconCacheEntries[key].offset;
    u32 length = iconCacheEntries[key].length;
    if (!offset || !length) return NULL;
    if ((long) offset + (long) length > archiveSize) return NULL;

    unsigned char *data = (unsigned char *) malloc(length);
    if (!data) return NULL;

    if (fseek(iconCacheArchive, offset, SEEK_SET) != 0 ||
        fread(data, 1, length, iconCacheArchive) != length) {
        free(data);
        return NULL;
    }

    C2D_SpriteSheet sheet = C2D_SpriteSheetLoadFromMem(data, length);
    free(data);
    return sheet;
}

static void iconCacheCloseArchive(void) {
    if (iconCacheArchive) fclose(iconCacheArchive);
    iconCacheArchive = NULL;

    free(iconCacheEntries);
    iconCacheEntries = NULL;
    entryCount = 0;
    archiveSize = 0;
}

void iconCacheInit(void) {
    memset(cache, 0, sizeof(cache));

    int total = 0;
    for (int i = 0; i < ICON_GAMEMODE_COUNT; i++) {
        iconPrefix[i] = total;
        total += gamemodeIconCount[i];
    }

    iconCacheArchive = fopen("romfs:/gfx/icons.bin", "rb");
    if (!iconCacheArchive) {
        outputLog("Could not open romfs:/gfx/icons.bin\n");
        return;
    }

    u32 header[4];
    if (fread(header, sizeof(header), 1, iconCacheArchive) != 1 ||
        header[0] != ICON_ARCHIVE_MAGIC ||
        header[2] != (u32) total) {
        outputLog("Invalid romfs:/gfx/icons.bin header\n");
        iconCacheCloseArchive();
        return;
    }

    entryCount = total;
    iconCacheEntries = (IconArchiveEntry *) malloc(sizeof(IconArchiveEntry) * entryCount);
    if (!iconCacheEntries) {
        outputLog("Could not allocate icon archive table\n");
        iconCacheCloseArchive();
        return;
    }

    if (fread(iconCacheEntries, sizeof(IconArchiveEntry), entryCount, iconCacheArchive) != (size_t) entryCount) {
        outputLog("Could not read icon archive table\n");
        iconCacheCloseArchive();
        return;
    }

    fseek(iconCacheArchive, 0, SEEK_END);
    archiveSize = ftell(iconCacheArchive);
}

void iconCacheShutdown(void) {
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (cache[i].sheet) C2D_SpriteSheetFree(cache[i].sheet);
        cache[i].sheet = NULL;
        cache[i].key = -1;
        cache[i].pinned = false;
        cache[i].used = false;
    }

    iconCacheCloseArchive();
}

void iconCacheBeginFrame(void) {
    int pinnedKeys[ICON_GAMEMODE_COUNT];
    for (int g = 0; g < ICON_GAMEMODE_COUNT; g++) {
        pinnedKeys[g] = currentIcons[g] ? iconKey(g, *currentIcons[g]) : -1;
    }

    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        bool pinned = false;
        for (int g = 0; g < ICON_GAMEMODE_COUNT; g++) {
            if (cache[i].sheet && cache[i].key == pinnedKeys[g]) { pinned = true; break; }
        }
        cache[i].pinned = pinned;

        if (cache[i].sheet && !pinned && !cache[i].used) {
            C2D_SpriteSheetFree(cache[i].sheet);
            cache[i].sheet = NULL;
            cache[i].key = -1;
        }
        cache[i].used = false;
    }
}

C2D_SpriteSheet iconCacheGet(int gamemode, int id) {
    int key = iconKey(gamemode, id);
    if (key < 0) return NULL;

    int index = iconCacheFind(key);
    if (index >= 0) {
        cache[index].used = true;
        return cache[index].sheet;
    }

    index = -1;
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (!cache[i].sheet) { index = i; break; }
    }
    if (index < 0) return NULL;

    C2D_SpriteSheet sheet = iconCacheLoad(key);
    if (!sheet) return NULL;

    cache[index].key = key;
    cache[index].sheet = sheet;
    cache[index].pinned = false;
    cache[index].used = true;
    return sheet;
}

}
