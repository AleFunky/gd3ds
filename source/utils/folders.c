#include "folders.h"

#include <dirent.h>
#include <sys/types.h>
#include "json_config.h"
#include <string.h>
#include <stdio.h>

#include "main.h"

FileOrFolder sd_level_paths[MAX_SD_LEVELS];
int sd_level_count = 0;

char current_level_name[255];

char current_directory[256];
int dir_level = 0;

int compare_entries(const void *a, const void *b) {
    FileOrFolder *ea = (FileOrFolder*)a;
    FileOrFolder *eb = (FileOrFolder*)b;

    // Folders first
    if (ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir;

    // Alphabetical
    return strcasecmp(ea->name, eb->name);
}

FileOrFolder *load_folder(char *dir, int *count) {
    sd_level_count = 0;

    char directory[256];

    if (dir && strlen(dir) > 0) {
        snprintf(directory, sizeof(directory), "%s/%s", USER_LEVELS_DIR, dir);
    } else {
        strncpy(directory, USER_LEVELS_DIR, sizeof(directory));
    }

    strncpy(current_directory, directory, sizeof(current_directory));

    DIR *level_dir = opendir(directory);
    if (!level_dir) {
        output_log("Failed to open dir: %s\n", directory);
        return NULL;
    }

    output_log("Loaded folder: %s\n", directory);

    struct dirent *pent;
    struct stat statbuf;

    while ((pent = readdir(level_dir)) != NULL && sd_level_count < MAX_SD_LEVELS) {

        // Skip dot and dot dot
        if (strcmp(pent->d_name, ".") == 0 || strcmp(pent->d_name, "..") == 0)
            continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", directory, pent->d_name);

        if (stat(fullpath, &statbuf) != 0)
            continue;

        bool is_dir = S_ISDIR(statbuf.st_mode);

        if (is_dir) {
            // Folder
            snprintf(sd_level_paths[sd_level_count].name, MAX_PATH_LEN, "%s", pent->d_name);
            sd_level_paths[sd_level_count].is_dir = true;
            sd_level_count++;

            output_log("Folder: %s\n", pent->d_name);
        } else {
            // File
            const char *ext = strrchr(pent->d_name, '.');
            if (ext && strcmp(ext, ".gmd") == 0) {
                // Funny level
                snprintf(sd_level_paths[sd_level_count].name, MAX_PATH_LEN, "%s/%s", current_directory, pent->d_name);
                sd_level_paths[sd_level_count].is_dir = false;
                sd_level_count++;

                output_log("Level: %s (%llu)\n", pent->d_name, statbuf.st_size);
            }
        }
    }

    closedir(level_dir);

    qsort(sd_level_paths, sd_level_count, sizeof(FileOrFolder), compare_entries);

    *count = sd_level_count;

    return sd_level_paths;
}

void go_back_directory(char *path) {
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


char *strip_filename(char *path) {
    char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
};


void strip_extension(char *path) {
    char *dot = strrchr(path, '.');
    char *slash = strrchr(path, '/');

    // Make sure the dot is after the last slash
    if (dot && (!slash || dot > slash)) {
        *dot = '\0';
    }
}

void truncate_filename(char *name, size_t max_len) {
    size_t len = strlen(name);

    if (len > max_len) {
        name[max_len] = '\0';
        strcat(name, "...");
    }
}