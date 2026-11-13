#pragma once

#define MAX_SD_LEVELS 128
#define MAX_PATH_LEN 524

#include <3ds.h>

typedef struct {
    char name[MAX_PATH_LEN];
    bool isDir;
} FileOrFolder;


typedef struct GenericTask GenericTask;

FileOrFolder *loadFolder(GenericTask *task, char *dir, int *count);
void goBackDirectory(char *path);
char *stripFilename(char *path);
void stripExtension(char *path);
void truncateFilename(char *name, size_t maxLen);
void truncateFilenameStart(char *name, size_t maxLen, size_t bufferSize);