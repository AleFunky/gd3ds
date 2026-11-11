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

typedef struct {
    int key;
    C2D_SpriteSheet sheet;
    bool pinned;
    bool used;
} IconCacheEntry;

typedef struct {
    u32 offset;
    u32 length;
} IconArchiveEntry;

static IconCacheEntry cache[ICON_CACHE_CAPACITY];
static int icon_prefix[ICON_GAMEMODE_COUNT];
static IconArchiveEntry *icon_cache_entries;
static int entry_count;
static s64 archive_size;
static FILE *icon_cache_archive;

static int icon_key(int gamemode, int id) {
    if (gamemode < 0 || gamemode >= ICON_GAMEMODE_COUNT) return -1;
    if (id < 0 || id >= gamemode_icon_count[gamemode]) return -1;
    return icon_prefix[gamemode] + id;
}

static int icon_cache_find(int key) {
    if (key < 0) return -1;
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (cache[i].sheet && cache[i].key == key) return i;
    }
    return -1;
}

static C2D_SpriteSheet icon_cache_load(int key) {
    if (!icon_cache_archive || !icon_cache_entries || key < 0 || key >= entry_count) return NULL;

    u32 offset = icon_cache_entries[key].offset;
    u32 length = icon_cache_entries[key].length;
    if (!offset || !length) return NULL;
    if ((long) offset + (long) length > archive_size) return NULL;

    unsigned char *data = (unsigned char *) malloc(length);
    if (!data) return NULL;

    if (fseek(icon_cache_archive, offset, SEEK_SET) != 0 ||
        fread(data, 1, length, icon_cache_archive) != length) {
        free(data);
        return NULL;
    }

    C2D_SpriteSheet sheet = C2D_SpriteSheetLoadFromMem(data, length);
    free(data);
    return sheet;
}

static void icon_cache_close_archive(void) {
    if (icon_cache_archive) fclose(icon_cache_archive);
    icon_cache_archive = NULL;

    free(icon_cache_entries);
    icon_cache_entries = NULL;
    entry_count = 0;
    archive_size = 0;
}

void icon_cache_init(void) {
    memset(cache, 0, sizeof(cache));

    int total = 0;
    for (int i = 0; i < ICON_GAMEMODE_COUNT; i++) {
        icon_prefix[i] = total;
        total += gamemode_icon_count[i];
    }

    icon_cache_archive = fopen("romfs:/gfx/icons.bin", "rb");
    if (!icon_cache_archive) {
        output_log("Could not open romfs:/gfx/icons.bin\n");
        return;
    }

    u32 header[4];
    if (fread(header, sizeof(header), 1, icon_cache_archive) != 1 ||
        header[0] != ICON_ARCHIVE_MAGIC ||
        header[2] != (u32) total) {
        output_log("Invalid romfs:/gfx/icons.bin header\n");
        icon_cache_close_archive();
        return;
    }

    entry_count = total;
    icon_cache_entries = (IconArchiveEntry *) malloc(sizeof(IconArchiveEntry) * entry_count);
    if (!icon_cache_entries) {
        output_log("Could not allocate icon archive table\n");
        icon_cache_close_archive();
        return;
    }

    if (fread(icon_cache_entries, sizeof(IconArchiveEntry), entry_count, icon_cache_archive) != (size_t) entry_count) {
        output_log("Could not read icon archive table\n");
        icon_cache_close_archive();
        return;
    }

    fseek(icon_cache_archive, 0, SEEK_END);
    archive_size = ftell(icon_cache_archive);
}

void icon_cache_shutdown(void) {
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (cache[i].sheet) C2D_SpriteSheetFree(cache[i].sheet);
        cache[i].sheet = NULL;
        cache[i].key = -1;
        cache[i].pinned = false;
        cache[i].used = false;
    }

    icon_cache_close_archive();
}

void icon_cache_begin_frame(void) {
    int pinned_keys[ICON_GAMEMODE_COUNT];
    for (int g = 0; g < ICON_GAMEMODE_COUNT; g++) {
        pinned_keys[g] = current_icons[g] ? icon_key(g, *current_icons[g]) : -1;
    }

    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        bool pinned = false;
        for (int g = 0; g < ICON_GAMEMODE_COUNT; g++) {
            if (cache[i].sheet && cache[i].key == pinned_keys[g]) { pinned = true; break; }
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

C2D_SpriteSheet icon_cache_get(int gamemode, int id) {
    int key = icon_key(gamemode, id);
    if (key < 0) return NULL;

    int index = icon_cache_find(key);
    if (index >= 0) {
        cache[index].used = true;
        return cache[index].sheet;
    }

    index = -1;
    for (int i = 0; i < ICON_CACHE_CAPACITY; i++) {
        if (!cache[i].sheet) { index = i; break; }
    }
    if (index < 0) return NULL;

    C2D_SpriteSheet sheet = icon_cache_load(key);
    if (!sheet) return NULL;

    cache[index].key = key;
    cache[index].sheet = sheet;
    cache[index].pinned = false;
    cache[index].used = true;
    return sheet;
}
