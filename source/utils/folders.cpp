#include "folders.hpp"

#include <dirent.h>
#include "json_config.hpp"
#include <string.h>
#include <stdio.h>

#include "utils.hpp"
#include "main.hpp"

namespace gd3ds
{

FileOrFolder sdLevelPaths[MAX_SD_LEVELS];
int sdLevelCount = 0;

char currentLevelName[255];

char currentDirectory[256];
int dirLevel = 0;

int compareEntries(const void *a, const void *b) {
    FileOrFolder *ea = (FileOrFolder*)a;
    FileOrFolder *eb = (FileOrFolder*)b;

    // Folders first
    if (ea->isDir != eb->isDir)
        return eb->isDir - ea->isDir;

    // Alphabetical
    return strcasecmp(ea->name, eb->name);
}

FileOrFolder *loadFolder(GenericTask *task, char *dir, int *count) {
    sdLevelCount = 0;

    char directory[256];

    if (dir && strlen(dir) > 0) {
        snprintf(directory, sizeof(directory), "%s/%s", USER_LEVELS_DIR, dir);
    } else {
        strncpy(directory, USER_LEVELS_DIR, sizeof(directory));
    }

    strncpy(currentDirectory, directory, sizeof(currentDirectory));

    DIR *levelDir = opendir(directory);
    if (!levelDir) {
        outputLog("Failed to open dir: %s\n", directory);
        return NULL;
    }

    outputLog("Loaded folder: %s\n", directory);

    struct dirent *pent;
    struct stat statbuf;

    while ((pent = readdir(levelDir)) != NULL && sdLevelCount < MAX_SD_LEVELS) {
        if (task->cancelled) {
            return NULL;
        }

        // Skip dot and dot dot
        if (strcmp(pent->d_name, ".") == 0 || strcmp(pent->d_name, "..") == 0)
            continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, pent->d_name);

        if (stat(fullpath, &statbuf) != 0)
            continue;

        bool isDir = S_ISDIR(statbuf.st_mode);

        if (isDir) {
            // Folder
            snprintf(sdLevelPaths[sdLevelCount].name, MAX_PATH_LEN, "%s", pent->d_name);
            sdLevelPaths[sdLevelCount].isDir = true;
            sdLevelCount++;

            outputLog("Folder: %s\n", pent->d_name);
        } else {
            // File
            const char *ext = strrchr(pent->d_name, '.');
            if (ext && (strcmp(ext, ".gmd") == 0 || strcmp(ext, ".gdlvl") == 0)) {
                // Funny level
                snprintf(sdLevelPaths[sdLevelCount].name, MAX_PATH_LEN, "%s/%s", currentDirectory, pent->d_name);
                sdLevelPaths[sdLevelCount].isDir = false;
                sdLevelCount++;

                outputLog("Level: %s (%llu)\n", pent->d_name, statbuf.st_size);
            }
        }
    }

    closedir(levelDir);

    qsort(sdLevelPaths, sdLevelCount, sizeof(FileOrFolder), compareEntries);

    *count = sdLevelCount;

    return sdLevelPaths;
}

void goBackDirectory(char *path) {
    size_t len = strlen(path);
    if (len == 0) return;

    while (len > 0 && path[len - 1] == '/') {
        path[--len] = '\0';
    }

    while (len > 0 && path[len - 1] != '/') {
        path[--len] = '\0';
    }

    if (len > 0 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
}


char *stripFilename(char *path) {
    char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
};


void stripExtension(char *path) {
    char *dot = strrchr(path, '.');
    char *slash = strrchr(path, '/');

    // Make sure the dot is after the last slash
    if (dot && (!slash || dot > slash)) {
        *dot = '\0';
    }
}

void truncateFilename(char *name, size_t maxLen) {
    size_t len = strlen(name);

    if (len > maxLen) {
        name[maxLen] = '\0';
        strcat(name, "...");
    }
}

void truncateFilenameStart(char *name, size_t maxLen, size_t bufferSize) {
    size_t len = strlen(name);

    if (len > maxLen) {
        const char *ellipsis = "...";
        size_t ellipsisLen = strlen(ellipsis);

        if (maxLen <= ellipsisLen) {
            // Not enough space, just truncate hard
            name[maxLen] = '\0';
            return;
        }

        size_t keepLen = maxLen - ellipsisLen;

        // Move the last part of the string to the front
        memmove(name, name + (len - keepLen), keepLen);

        // Add ellipsis at the start
        memmove(name + ellipsisLen, name, keepLen);
        memcpy(name, ellipsis, ellipsisLen);

        name[maxLen] = '\0';
    }
}

}
