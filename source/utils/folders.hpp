#pragma once

#define MAX_SD_LEVELS 128
#define MAX_PATH_LEN 524

#include <3ds.h>

namespace gd3ds
{

struct FileOrFolder {
    char name[MAX_PATH_LEN];
    bool isDir;
};


struct GenericTask;

FileOrFolder *loadFolder(GenericTask *task, char *dir, int *count);
void goBackDirectory(char *path);
char *stripFilename(char *path);
void stripExtension(char *path);
void truncateFilename(char *name, size_t maxLen);
void truncateFilenameStart(char *name, size_t maxLen, size_t bufferSize);

}
