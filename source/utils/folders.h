#pragma once

#define MAX_SD_LEVELS 128
#define MAX_PATH_LEN 524

#include <3ds.h>

typedef struct {
    char name[MAX_PATH_LEN];
    bool is_dir;
} FileOrFolder;

FileOrFolder *load_folder(char *dir, int *count);
void go_back_directory(char *path);
char *strip_filename(char *path);
void strip_extension(char *path);
void truncate_filename(char *name, size_t max_len);