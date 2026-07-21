#include "json_config.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct json_object* get_path_object(struct json_object* root, const char* path) {
    char buffer[256];
    strncpy(buffer, path, sizeof(buffer) - 1);

    char* token = strtok(buffer, CONFIG_OBJECT_DELIMITER);
    struct json_object* current = root;

    // Iterate through JSON objects
    while (token) {
        struct json_object* next;

        if (!json_object_object_get_ex(current, token, &next))
            return NULL;

        current = next;
        token = strtok(NULL, CONFIG_OBJECT_DELIMITER);
    }

    return current;
}

static struct json_object* create_path_parent(struct json_object* root, const char* path, char* last_key) {
    char buffer[256];
    strncpy(buffer, path, sizeof(buffer) - 1);

    char* token = strtok(buffer, CONFIG_OBJECT_DELIMITER);
    struct json_object* current = root;
    char* next = strtok(NULL, CONFIG_OBJECT_DELIMITER);

    while (next) {
        struct json_object* obj;

        // If this object doesn't exist, make it
        if (!json_object_object_get_ex(current, token, &obj)) {
            obj = json_object_new_object();
            json_object_object_add(current, token, obj);
        }

        current = obj;

        token = next;
        next = strtok(NULL, CONFIG_OBJECT_DELIMITER);
    }

    strcpy(last_key, token);
    return current;
}

int config_load(Config* cfg, const char* path) {
    if (cfg->root && strcmp(cfg->path, path) == 0)
        return 1;

    if (cfg->root) {
        json_object_put(cfg->root);
        cfg->root = NULL;
    }

    strncpy(cfg->path, path, sizeof(cfg->path) - 1);
    cfg->path[sizeof(cfg->path) - 1] = '\0';

    // Check if it exists
    FILE* f = fopen(path, "r");
    if (!f) {
        cfg->root = json_object_new_object();
        return 0;
    }

    // It exists, so read it
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = 0;
    fclose(f);

    cfg->root = json_tokener_parse(data);
    free(data);

    if (!cfg->root)
        cfg->root = json_object_new_object();

    return 1;
}

void config_save(Config* cfg) {
    const char* json = json_object_to_json_string_ext(
        cfg->root,
        JSON_C_TO_STRING_PRETTY
    );

    FILE* f = fopen(cfg->path, "w");
    if (!f) return;

    fputs(json, f);
    fclose(f);
}

const char* config_get_string(Config* cfg, const char* path, const char* def) {
    struct json_object* obj = get_path_object(cfg->root, path);
    if (!obj) return def;

    return json_object_get_string(obj);
}

int config_get_int(Config* cfg, const char* path, int def) {
    struct json_object* obj = get_path_object(cfg->root, path);
    if (!obj) return def;

    return json_object_get_int(obj);
}

float config_get_float(Config* cfg, const char* path, float def) {
    struct json_object* obj = get_path_object(cfg->root, path);
    if (!obj) return def;

    return json_object_get_double(obj);
}

int config_get_bool(Config* cfg, const char* path, int def) {
    struct json_object* obj = get_path_object(cfg->root, path);
    if (!obj) return def;

    return json_object_get_boolean(obj);
}

void config_set_string(Config* cfg, const char* path, const char* value) {
    char key[128];
    struct json_object* parent = create_path_parent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_string(value));
}

void config_set_int(Config* cfg, const char* path, int value) {
    char key[128];
    struct json_object* parent = create_path_parent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_int(value));
}

void config_set_float(Config* cfg, const char* path, float value) {
    char key[128];
    struct json_object* parent = create_path_parent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_double(value));
}

void config_set_bool(Config* cfg, const char* path, int value) {
    char key[128];
    struct json_object* parent = create_path_parent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_boolean(value));
}

void config_init_bool(Config* cfg, const char* path, bool def) {
    config_set_bool(cfg, path, config_get_bool(cfg, path, def));
}

void config_init_int(Config* cfg, const char* path, int def) {
    config_set_int(cfg, path, config_get_int(cfg, path, def));
}

void config_init_float(Config* cfg, const char* path, float def) {
    config_set_float(cfg, path, config_get_float(cfg, path, def));
}

void config_init_string(Config* cfg, const char* path, char* def) {
    config_set_string(cfg, path, config_get_string(cfg, path, def));
}

void config_free(Config* cfg) {
    json_object_put(cfg->root);
    cfg->root = NULL;
}