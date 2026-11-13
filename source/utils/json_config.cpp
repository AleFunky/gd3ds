#include "json_config.hpp"
#include "main.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace gd3ds
{

static struct json_object* getPathObject(struct json_object* root, const char* path) {
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

static struct json_object* createPathParent(struct json_object* root, const char* path, char* lastKey) {
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

    strcpy(lastKey, token);
    return current;
}

int configLoad(Config* cfg, const char* path) {
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

    char* data = (char *)malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = 0;
    fclose(f);

    cfg->root = json_tokener_parse(data);
    free(data);

    if (!cfg->root)
        cfg->root = json_object_new_object();

    return 1;
}

void configSave(Config* cfg) {
    const char* json = json_object_to_json_string_ext(
        cfg->root,
        JSON_C_TO_STRING_PRETTY
    );

    FILE* f = fopen(cfg->path, "w");
    if (!f) return;

    fputs(json, f);
    fclose(f);
}

const char* configGetString(Config* cfg, const char* path, const char* def) {
    struct json_object* obj = getPathObject(cfg->root, path);
    if (!obj) return def;

    return json_object_get_string(obj);
}

int configGetInt(Config* cfg, const char* path, int def) {
    struct json_object* obj = getPathObject(cfg->root, path);
    if (!obj) return def;

    return json_object_get_int(obj);
}

float configGetFloat(Config* cfg, const char* path, float def) {
    struct json_object* obj = getPathObject(cfg->root, path);
    if (!obj) return def;

    return json_object_get_double(obj);
}

int configGetBool(Config* cfg, const char* path, int def) {
    struct json_object* obj = getPathObject(cfg->root, path);
    if (!obj) return def;

    return json_object_get_boolean(obj);
}

void configSetString(Config* cfg, const char* path, const char* value) {
    char key[128];
    struct json_object* parent = createPathParent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_string(value));
}

void configSetInt(Config* cfg, const char* path, int value) {
    char key[128];
    struct json_object* parent = createPathParent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_int(value));
}

void configSetFloat(Config* cfg, const char* path, float value) {
    char key[128];
    struct json_object* parent = createPathParent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_double(value));
}

void configSetBool(Config* cfg, const char* path, int value) {
    char key[128];
    struct json_object* parent = createPathParent(cfg->root, path, key);

    json_object_object_add(parent, key, json_object_new_boolean(value));
}

void configInitBool(Config* cfg, const char* path, bool def) {
    configSetBool(cfg, path, configGetBool(cfg, path, def));
}

void configInitInt(Config* cfg, const char* path, int def) {
    configSetInt(cfg, path, configGetInt(cfg, path, def));
}

void configInitFloat(Config* cfg, const char* path, float def) {
    configSetFloat(cfg, path, configGetFloat(cfg, path, def));
}

void configInitString(Config* cfg, const char* path, const char* def) {
    configSetString(cfg, path, configGetString(cfg, path, def));
}

void configFree(Config* cfg) {
    json_object_put(cfg->root);
    cfg->root = NULL;
}

}
