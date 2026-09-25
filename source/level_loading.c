#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include "level_loading.h"
#include <zlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "color_channels.h"
#include "main.h"
#include "menus/creator_menu/online/online_menu.h"
#include "objects.h"
#include "groups.h"
#include "mp3_player.h"
#include "graphics.h"
#include "math_helpers.h"
#include "particles/object_particles.h"
#include "utils/json_config.h"
#include "state.h"

#include "utils/server_utils.h"
#include "utils/string_helpers.h"

#include "player/collision.h"
#include "utils/utils.h"

ObjectsArray objects = { 0 };

Section empty_section = { 0 };

Section *section_hash[SECTION_HASH_SIZE] = {0};

static int coin_count;
static int coin_ids[3];

int channelCount = 0;
GDColorChannel *colorChannels = NULL;

LoadedLevelInfo level_info;

char *curr_level_string = NULL;

const char *level_lengths[] = {
    "Tiny",
    "Short",
    "Medium",
    "Long",
    "XL",
};

static inline unsigned int section_hash_func(unsigned int x, unsigned int y) {
    return ((unsigned int)x * 73856093u ^ (unsigned int)y * 19349663u) & (SECTION_HASH_SIZE - 1);
}

Section *get_section(int x, int y) {
    unsigned int h = section_hash_func(x, y);
    Section *sec = section_hash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    return &empty_section;
}

Section *get_or_create_section(int x, int y) {
    unsigned int h = section_hash_func(x, y);
    Section *sec = section_hash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    sec = malloc(sizeof(Section));
    sec->objects = malloc(sizeof(int) * 8);
    sec->object_count = 0;
    sec->object_capacity = 8;
    sec->x = x;
    sec->y = y;
    sec->next = section_hash[h];
    section_hash[h] = sec;
    return sec;
}

void free_sections(void) {
    for (int i = 0; i < SECTION_HASH_SIZE; i++) {
        Section *sec = section_hash[i];
        while (sec) {
            Section *next = sec->next;
            free(sec->objects);
            free(sec);
            sec = next;
        }
        section_hash[i] = NULL;
    }
}

void assign_object_to_section(int obj) {
    int sx = (int)(objects.x[obj] / SECTION_SIZE);
    int sy = (int)(objects.y[obj] / SECTION_SIZE);
    Section *sec = get_or_create_section(sx, sy);
    if (sec->object_count >= sec->object_capacity) {
        sec->object_capacity *= 2;
        sec->objects = realloc(sec->objects, sizeof(int) * sec->object_capacity);
    }
    sec->objects[sec->object_count++] = obj;
    objects.section_x[obj] = sx;
    objects.section_y[obj] = sy;
}

void update_object_section(int obj) {
    int new_sx = (int)(objects.x[obj] / SECTION_SIZE);
    int new_sy = (int)(objects.y[obj] / SECTION_SIZE);
    int old_sx = objects.section_x[obj];
    int old_sy = objects.section_y[obj];

    if (new_sx == old_sx && new_sy == old_sy) return;

    Section *old_sec = get_section(old_sx, old_sy);
    if (old_sec) {
        for (int i = 0; i < old_sec->object_count; i++) {
            if (old_sec->objects[i] == obj) {
                old_sec->objects[i] = old_sec->objects[--old_sec->object_count];
                break;
            }
        }
    }

    assign_object_to_section(obj);
}

char *read_file(const char *filepath, size_t *out_size) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        output_log("Failed to open file: %s (%x)\n", filepath, errno);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        output_log("Failed to allocate file\n");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Null-terminate for text files
    fclose(f);

    if (out_size) *out_size = size;
    return buffer;
}

char *extract_gmd_key(const char *data, const char *key, const char *type) {
    char key_tag[32];
    snprintf(key_tag, sizeof(key_tag), "<k>%s</k>", key);
    
    char *key_pos = strstr(data, key_tag);
    if (!key_pos) {
        return NULL;
    }

    // Move past the key tag
    char *start = key_pos + strlen(key_tag);

    // Skip whitespace (spaces, tabs, newlines, etc.)
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char type_start_tag[16];
    snprintf(type_start_tag, sizeof(type_start_tag), "<%s>", type);

    // Confirm that the type start tag is here
    if (strncmp(start, type_start_tag, strlen(type_start_tag)) != 0) {
        output_log("Expected start tag '%s' not found after key\n", type_start_tag);
        return NULL;
    }

    // Move past the type start tag
    start += strlen(type_start_tag);

    // Find the end tag
    char type_end_tag[16];
    snprintf(type_end_tag, sizeof(type_end_tag), "</%s>", type);
    char *end = strstr(start, type_end_tag);
    if (!end) {
        output_log("Could not find end tag '%s'\n", type_end_tag);
        return NULL;
    }

    // Allocate and copy value
    int len = end - start;
    char *value = malloc(len + 1);
    if (!value) {
        output_log("malloc for gmd key %s failed\n", key);
        return NULL;
    }
    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

bool is_ascii(const unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        if (data[i] > 0x7F) {
            return false; // Non-ASCII byte found
        }
    }
    return true;
}

int b64_char(char c) {
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

void fix_base64_url(char *b64) {
    for (int i = 0; b64[i]; i++) {
        if (b64[i] == '-') b64[i] = '+';
        else if (b64[i] == '_') b64[i] = '/';
    }
}

int base64_decode(const char *in, unsigned char *out) {
    int len = 0;
    for (int i = 0; in[i] && in[i+1] && in[i+2] && in[i+3]; i += 4) {
        int a = b64_char(in[i]);
        int b = b64_char(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64_char(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64_char(in[i+3]);

        if (a == -1 || b == -1 || c == -1 || d == -1) {
            return -1;
        }

        out[len++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[len++] = (b << 4) | (c >> 2);
        if (in[i+3] != '=') out[len++] = (c << 6) | d;
    }
    return len;
}


char *get_metadata_value(const char *levelString, const char *key) {
    if (!levelString || !key) return NULL;

    // Find the first semicolon, which separates metadata from objects
    const char *end = strchr(levelString, ';');
    if (!end) return NULL;

    // We'll scan only the metadata portion
    size_t metadataLen = end - levelString;
    char *metadata = malloc(metadataLen + 1);
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

char *decompress_online_level(char *data, int *out_code) {
    printf("Loading level data...\n");

    fix_base64_url(data);

    // Bro theres some levels that randomly have , at the end of the base64 string wtf
    remove_char(data, ',');

    //output_log("BRO len %d lol %s\n", strlen(data), data);

    unsigned char *decoded = malloc(strlen(data));

    if (!decoded) {
        *out_code = LOAD_OUT_OF_MEMORY;
        return NULL;
    }
    
    int decoded_len = base64_decode(data, decoded);
    if (decoded_len <= 0) {
        output_log("Failed to decode base64\n");
        free(decoded);
        *out_code = LOAD_INVALID_BASE64;
        return NULL;
    }

    size_t decompressed_len;
    char *decompressed = decompress_data(decoded, decoded_len, &decompressed_len, out_code);
    if (!decompressed) {
        output_log("Decompression failed (check zlib error above)\n");
        free(decoded);
        return NULL;
    }

    free(decoded);
    
    return decompressed;
}

char *decompress_level(char *data, int *out_code) {
    printf("Loading level data...\n");
    
    char *b64 = extract_gmd_key((const char *) data, "k4", "s");
    if (!b64) {
        // Empty level
        char *temp = strdup(data);
        if (!temp) {
            *out_code = LOAD_OUT_OF_MEMORY;
        }
        return temp;
    }

    const char *semicolon = strchr(b64, ';');
    if (semicolon) return b64; // Rare uncompressed gmd found

    fix_base64_url(b64);

    unsigned char *decoded = malloc(strlen(b64));
    if (!decoded) {
        *out_code = LOAD_OUT_OF_MEMORY;
        return NULL;
    }
    int decoded_len = base64_decode(b64, decoded);
    if (decoded_len <= 0) {
        output_log("Failed to decode base64\n");
        free(b64);
        free(decoded);
        *out_code = LOAD_OUT_OF_MEMORY;
        return NULL;
    }

    size_t decompressed_len;
    char *decompressed = decompress_data(decoded, decoded_len, &decompressed_len, out_code);
    if (!decompressed) {
        output_log("Decompression failed (check zlib error above)\n");
        free(decoded);
        free(b64);
        return NULL;
    }

    free(decoded);
    free(b64);
    
    return decompressed;
}

char **split_string_str_del(const char *str, const char *delimiter, int *outCount, bool ignoreZeroLength) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;
    size_t delimiterLen = strlen(delimiter);

    while (*ptr) {
        if (strncmp(ptr, delimiter, delimiterLen) == 0) {
            int len = ptr - start;

            if (len > (ignoreZeroLength ? -1 : 0)) {
                char *token = malloc(len + 1);
                strncpy(token, start, len);
                token[len] = '\0';
                result = (char **)realloc(result, sizeof(char *) * (count + 1));
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
            char *token = malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            result = realloc(result, sizeof(char *) * (count + 1));
            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

char **split_string(const char *str, char delimiter, int *outCount, bool ignoreZeroLength) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr == delimiter) {
            int len = ptr - start;
            if (len > (ignoreZeroLength ? -1 : 0)) {
                char *token = malloc(len + 1);
                strncpy(token, start, len);
                token[len] = '\0';

                result = (char **)realloc(result, sizeof(char*) * (count + 1));
                result[count++] = token;
            }
            start = ptr + 1;
        }
        ptr++;
    }
    if (ptr > start) {
        int len = ptr - start;
        if (len > (ignoreZeroLength ? -1 : 0)) {
            char *token = malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            result = realloc(result, sizeof(char*) * (count + 1));
            result[count++] = token;
        }
    }

    *outCount = count;
    return result;
}

void free_string_array(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

void parse_ints(short *int_array, const char *string) {
    int count = 0;
    char **ints = split_string(string, '.', &count, false);

    for (int j = 0; j < MAX_GROUPS_PER_OBJECT; j++) {
        if (j < count) int_array[j] = (short) atoi(ints[j]);
        else int_array[j] = 0;
    }

    free_string_array(ints, count);
}

HSV parse_hsv_string(const char *string) {
    int count = 0;
    char **hsv_string = split_string(string, 'a', &count, false);
    HSV obtained = { 0 };

    if (count >= 5) {
        obtained.h = atoi(hsv_string[0]);
        obtained.s = atof(hsv_string[1]);
        obtained.v = atof(hsv_string[2]);
        obtained.sChecked = parse_bool(hsv_string[3]);
        obtained.vChecked = parse_bool(hsv_string[4]);
    }

    free_string_array(hsv_string, count);
    return obtained;
}

void parse_color_channel(GDColorChannel *channels, int i, char *channel_string) {
    GDColorChannel channel = {0};  // Zero-initialize
    channel.fromOpacity = 1.0f;
    int kvCount = 0;
    char **kvs = split_string(channel_string, '_', &kvCount, false);

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
            case 10: channel.hsv = parse_hsv_string(valStr); break;
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
    free_string_array(kvs, kvCount);
}

int parse_old_channels(char *level_string, GDColorChannel **outArray, int *out_code) {
    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * 2);
    if (!channels) {
        output_log("Couldn't alloc initial pre 2.0 color channels\n");
        *out_code = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    char *v19_bg = get_metadata_value(level_string, "kS29");

    int i = 0;

    if (v19_bg) { // 1.9 only
        parse_color_channel(channels, i, v19_bg);
        channels[i].channelID = CHANNEL_BG;
        i++;
        free(v19_bg);

        char *ground = get_metadata_value(level_string, "kS30");
        if (ground) {
            parse_color_channel(channels, i, ground);
            channels[i].channelID = CHANNEL_GROUND;
            i++;
            free(ground);
        }

        char *line = get_metadata_value(level_string, "kS31");
        if (line) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, line);
            channels[i].channelID = CHANNEL_LINE;
            i++;
            free(line);
        }
        
        char *obj = get_metadata_value(level_string, "kS32");
        if (obj) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, obj);
            channels[i].channelID = CHANNEL_OBJ;
            i++;
            free(obj);
        }

        char *col1 = get_metadata_value(level_string, "kS33");
        if (col1) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col1);
            channels[i].channelID = 1;
            i++;
            free(col1);
        }

        char *col2 = get_metadata_value(level_string, "kS34");
        if (col2) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col2);
            channels[i].channelID = 2;
            i++;
            free(col2);
        }

        char *col3 = get_metadata_value(level_string, "kS35");
        if (col3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col3);
            channels[i].channelID = 3;
            i++;
            free(col3);
        }

        char *col4 = get_metadata_value(level_string, "kS36");
        if (col4) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col4);
            channels[i].channelID = 4;
            i++;
            free(col4);
        }

        char *dl3 = get_metadata_value(level_string, "kS37");
        if (dl3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, dl3);
            channels[i].channelID = CHANNEL_3DL;
            i++;
            free(dl3);
        }
        
        *outArray = channels;
        return i;
    }

    // Pre 1.9
    int bg_r = atoi(get_metadata_value(level_string, "kS1"));
    int bg_g = atoi(get_metadata_value(level_string, "kS2"));
    int bg_b = atoi(get_metadata_value(level_string, "kS3"));

    GDColorChannel bg_channel = {0};
    bg_channel.fromOpacity = 1.0f;
    bg_channel.channelID = CHANNEL_BG;
    bg_channel.fromRed = bg_r;
    bg_channel.fromGreen = bg_g;
    bg_channel.fromBlue = bg_b;

    char *bg_player_color = get_metadata_value(level_string, "kS16");
    if (bg_player_color) {
        bg_channel.playerColor = atoi(bg_player_color);
        free(bg_player_color);
    }

    channels[i] = bg_channel;

    i++;

    int g_r = atoi(get_metadata_value(level_string, "kS4"));
    int g_g = atoi(get_metadata_value(level_string, "kS5"));
    int g_b = atoi(get_metadata_value(level_string, "kS6"));

    GDColorChannel g_channel = {0};
    g_channel.fromOpacity = 1.0f;
    g_channel.channelID = CHANNEL_GROUND;
    g_channel.fromRed = g_r;
    g_channel.fromGreen = g_g;
    g_channel.fromBlue = g_b;

    char *g_player_color = get_metadata_value(level_string, "kS17");
    if (g_player_color) {
        g_channel.playerColor = atoi(g_player_color);
        free(g_player_color);
    }
    
    channels[i] = g_channel;
    i++;

    char *line_r = get_metadata_value(level_string, "kS7");
    char *line_g = get_metadata_value(level_string, "kS8");
    char *line_b = get_metadata_value(level_string, "kS9");

    if (line_r && line_g && line_b) {
        GDColorChannel line_channel = {0};
        line_channel.fromOpacity = 1.0f;
        line_channel.channelID = CHANNEL_LINE;
        line_channel.fromRed = atoi(line_r);
        line_channel.fromGreen = atoi(line_g);
        line_channel.fromBlue = atoi(line_b);
        
        char *line_player_color = get_metadata_value(level_string, "kS18");
        if (line_player_color) {
            line_channel.playerColor = atoi(line_player_color);
            free(line_player_color);
        }

        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = line_channel;
        i++;
    }

    char *obj_r = get_metadata_value(level_string, "kS10");
    char *obj_g = get_metadata_value(level_string, "kS11");
    char *obj_b = get_metadata_value(level_string, "kS12");

    if (obj_r && obj_g && obj_b) {
        GDColorChannel obj_channel = {0};
        obj_channel.fromOpacity = 1.0f;
        obj_channel.channelID = CHANNEL_OBJ;
        obj_channel.fromRed = atoi(obj_r);
        obj_channel.fromGreen = atoi(obj_g);
        obj_channel.fromBlue = atoi(obj_b);

        char *obj_player_color = get_metadata_value(level_string, "kS19");
        if (obj_player_color) {
            obj_channel.playerColor = atoi(obj_player_color);
            free(obj_player_color);
        }
        
        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = obj_channel;
        i++;
    }

    char *obj_2_r = get_metadata_value(level_string, "kS13");
    char *obj_2_g = get_metadata_value(level_string, "kS14");
    char *obj_2_b = get_metadata_value(level_string, "kS15");

    if (obj_2_r && obj_2_g && obj_2_b) {
        GDColorChannel obj_2_channel = {0};
        obj_2_channel.fromOpacity = 1.0f;
        obj_2_channel.channelID = 1;
        obj_2_channel.fromRed = atoi(obj_2_r);
        obj_2_channel.fromGreen = atoi(obj_2_g);
        obj_2_channel.fromBlue = atoi(obj_2_b);
        
        char *obj_2_player_color = get_metadata_value(level_string, "kS20");
        if (obj_2_player_color) {
            obj_2_channel.playerColor = atoi(obj_2_player_color);
            free(obj_2_player_color);
        }

        char *obj_2_blending = get_metadata_value(level_string, "kA5");
        if (obj_2_blending) {
            obj_2_channel.blending = atoi(obj_2_blending) != 0;
            free(obj_2_blending);
        }

        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = obj_2_channel;
        i++;
    }

    *outArray = channels;

    return i;
}

int parse_color_channels(const char *colorString, GDColorChannel **outArray, int *out_code) {
    if (!colorString || !outArray) return 0;

    int count = 0;
    // Split string into each channel
    char **entries = split_string(colorString, '|', &count, false);
    if (!entries) {
        *out_code = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * count);
    if (!channels) {
        output_log("Couldn't alloc color channels\n");
        free_string_array(entries, count);
        *out_code = LOAD_OUT_OF_MEMORY;
        return 0;
    }

    for (int i = 0; i < count; i++) {
        parse_color_channel(channels, i, entries[i]);
    }

    *outArray = channels;
    free_string_array(entries, count);
    return count;
}

GDValueType get_value_type_for_key(int key) {
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
        case 65: return GD_VAL_BOOL;   // pulse main_only
        case 66: return GD_VAL_BOOL;   // pulse detail_only
        case 87: return GD_VAL_BOOL;   // multi_triggered
        case 128: return GD_VAL_FLOAT; // scale X
        case 129: return GD_VAL_FLOAT; // scale Y
        case 57: return GD_VAL_INT_ARRAY; // groups
        default:
            return GD_VAL_INT; // Default fallback
    }
}

// Convert some 2.1 objects into the 1.9 ones, blame robtop for making GD convert those to 2.1
int convert_object(int id) {
    switch (id) {
        case V2_0_LINE_TRIGGER:
            return LINE_TRIGGER;
        // Saws
        case 1734:
            return 675;
        case 1735:
            return 676;
        case 1736:
            return 677;
        case 1705:
            return 88;
        case 1706:
            return 89;
        case 1707:
            return 98;
        case 1708:
            return 397;
        case 1709:
            return 398;
        case 1710:
            return 399;

        // User coin
        case 1329:
            if (state.custom_level) return 0;
            else return SECRET_COIN;

        // Slopes
        case 1743:
            return 289;
        case 1744:
            return 291;

        case 1745:
            return 299;
        case 1746:
            return 301;

        case 1747:
            return 309;
        case 1748:
            return 311;

        case 1749:
            return 315;
        case 1750:
            return 317;
        
        case 1338:
            return 665;
        case 1339:
            return 666;

        // Ground spikes

        case 1715:
            return 9;

        case 1719:
            return 61;

        case 1720:
            return 243;
        case 1721:
            return 244;
        
        case 1716:
            return 365;
        case 1717:
            return 363;
        case 1718:
            return 364;

        case 1722:
            return 368;
        case 1723:
            return 366;
        case 1724:
            return 367;

        case 1725:
            return 421;
        case 1726:
            return 422;
        
        case 1728:
            return 446;
        case 1729:
            return 447;
        
        case 1730:
            return 667;
        case 1731:
            return 720;

        // Fake spikes
        case 1889:
            return 191;
        case 1890:
            return 198;
        case 1891:
            return 199;
        case 1892:
            return 393;

        // Default slabs
        case 1903:
            return 40;

        case 1904:
            return 369;

        case 1905:
            return 370;

        case 1910:
            return 195;

        case 1911:
            return 196;
        
    }
    return id;
}

void fill_object_data(int object, int key, GDValueType type, GDValue val) {
    // Default members
    switch (key) {
        case 1:  // ID
            if (type == GD_VAL_INT) objects.id[object] = convert_object(val.i);
            break;
        case 2:  // X
            if (type == GD_VAL_FLOAT) {
                objects.x[object] = val.f;
                objects.original_x[object] = val.f;
            }
            break;
        case 3:  // Y
            if (type == GD_VAL_FLOAT) {
                objects.y[object] = val.f;
                objects.original_y[object] = val.f;
            }
            break;
        case 4:  // FlippedH
            if (type == GD_VAL_BOOL) objects.flippedH[object] = val.b;
            break;
        case 5:  // FlippedV
            if (type == GD_VAL_BOOL) objects.flippedV[object] = val.b;
            break;
        case 6:  // Rotation
            if (type == GD_VAL_FLOAT) objects.rotation[object] = val.f;
            break;
        case 7:  // Color R
            if (type == GD_VAL_INT) objects.trig_colorR[object] = val.i;
            break;
        case 8:  // Color G
            if (type == GD_VAL_INT) objects.trig_colorG[object] = val.i;
            break;
        case 9:  // Color B
            if (type == GD_VAL_INT) objects.trig_colorB[object] = val.i;
            break;
        case 10: // Duration
            if (type == GD_VAL_FLOAT) objects.trig_duration[object] = val.f;
            break;
        case 11: // Touch triggered
            if (type == GD_VAL_BOOL) objects.touch_triggered[object] = val.b;
            break;
        case 14: // Tint Ground
            if (type == GD_VAL_BOOL) objects.tintGround[object] = val.b;
            break;
        case 15: // Player 1 color
            if (type == GD_VAL_BOOL) objects.p1_color[object] = val.b;
            break;
        case 16: // Player 2 color
            if (type == GD_VAL_BOOL) objects.p2_color[object] = val.b;
            break;
        case 17: // Blending
            if (type == GD_VAL_BOOL) objects.blending[object] = val.b;
            break;
        case 19: // 1.9 channel id
            if (type == GD_VAL_INT) objects.v1p9_col_channel[object] = convert_one_point_nine_channel(val.i);
            break;
        case 21: // Main col channel
            if (type == GD_VAL_INT) objects.col_channel[object] = val.i;
            break;
        case 22: // Detail col channel
            if (type == GD_VAL_INT) objects.detail_col_channel[object] = val.i;
            break;
        case 23: // Target color ID
            if (type == GD_VAL_INT) objects.target_color_id[object] = val.i;
            break;
        case 24: // Z layer
            if (type == GD_VAL_INT) objects.zlayer[object] = val.i;
            break;
        case 25: // Z order
            if (type == GD_VAL_INT) objects.zorder[object] = val.i;
            break;
        case 28: // Move offset X
            if (type == GD_VAL_FLOAT) objects.move_offset_x[object] = val.f;
            break;
        case 29: // Move offset Y
            if (type == GD_VAL_FLOAT) objects.move_offset_y[object] = val.f;
            break;
        case 30: // Move easing
            if (type == GD_VAL_INT) objects.move_easing[object] = val.i;
            break;
        case 32: // Scale (uniform)
            if (type == GD_VAL_FLOAT) {
                objects.scale_x[object] = objects.scale_y[object] = val.f;
                objects.original_scale_x[object] = objects.original_scale_y[object] = val.f;
            }
            break;
        case 35: // Opacity (alpha trigger)
            if (type == GD_VAL_FLOAT) objects.trigger_opacity[object] = val.f;
            break;
        case 41: // main_col_HSV_enabled
            if (type == GD_VAL_BOOL) objects.main_col_HSV_enabled[object] = val.b;
            break;
        case 42: // detail_col_HSV_enabled
            if (type == GD_VAL_BOOL) objects.detail_col_HSV_enabled[object] = val.b;
            break;
        case 43: // main_col_HSV
            if (type == GD_VAL_HSV) objects.main_col_HSV[object] = val.hsv;
            break;
        case 44: // detail_col_HSV
            if (type == GD_VAL_HSV) objects.detail_col_HSV[object] = val.hsv;
            break;
        case 45: // pulse fade_in
            if (type == GD_VAL_FLOAT) objects.pulse_fade_in[object] = val.f;
            break;
        case 46: // pulse hold
            if (type == GD_VAL_FLOAT) objects.pulse_hold[object] = val.f;
            break;
        case 47: // pulse fade_out
            if (type == GD_VAL_FLOAT) objects.pulse_fade_out[object] = val.f;
            break;
        case 48: // pulse_mode
            if (type == GD_VAL_INT) objects.pulse_mode[object] = val.i;
            break;
        case 49: // copied_hsv
            if (type == GD_VAL_HSV) objects.copied_hsv[object] = val.hsv;
            break;
        case 50: // copied_color_id
            if (type == GD_VAL_INT) objects.copied_color_id[object] = val.i;
            break;
        case 51: // Target group
            if (type == GD_VAL_INT) objects.target_group[object] = val.i;
            break;
        case 52: // pulse_target_type
            if (type == GD_VAL_INT) objects.pulse_target_type[object] = val.i;
            break;
        case 54: // teleport portal Y offset
            if (type == GD_VAL_FLOAT) objects.tp_y_offset[object] = val.f;
            break;
        case 56: // Activate group
            if (type == GD_VAL_BOOL) objects.activate_group[object] = val.b;
            break;
        case 58: // Lock to player X
            if (type == GD_VAL_BOOL) objects.lock_to_player_x[object] = val.b;
            break;
        case 59: // Lock to player Y
            if (type == GD_VAL_BOOL) objects.lock_to_player_y[object] = val.b;
            break;
        case 62: // spawn_triggered
            if (type == GD_VAL_BOOL) objects.spawn_triggered[object] = val.b;
            break;
        case 63: // spawn_delay
            if (type == GD_VAL_FLOAT) objects.spawn_delay[object] = val.f;
            break;
        case 65: // pulse main_only
            if (type == GD_VAL_BOOL) objects.pulse_main_only[object] = val.b;
            break;
        case 66: // pulse detail_only
            if (type == GD_VAL_BOOL) objects.pulse_detail_only[object] = val.b;
            break;
        case 87: // multi_triggered
            if (type == GD_VAL_BOOL) objects.multi_triggered[object] = val.b;
            break;
        case 128: // Scale X
            if (type == GD_VAL_FLOAT) {
                objects.scale_x[object] = val.f;
                objects.original_scale_x[object] = val.f;
            }
            break;
        case 129: // Scale Y
            if (type == GD_VAL_FLOAT) {
                objects.scale_y[object] = val.f;
                objects.original_scale_y[object] = val.f;
            }
            break;
        case 57: // Groups
            if (type == GD_VAL_INT_ARRAY) {
                int gcount = 0;
                for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
                    objects.groups[object][i] = val.int_array[i];
                    if (val.int_array[i] != 0) gcount++;
                }
                objects.group_count[object] = gcount;
            }
            break;
    }
}

bool obj_has_main(const GameObject *obj) {
    if (obj->color_type == COLOR_TYPE_BASE) return true;
    
    for (int i = 0; i < obj->child_count; i++) {
        if (obj->children[i].color_type == COLOR_TYPE_BASE) return true;
    }
    return false;
}

bool obj_has_detail(const GameObject *obj) {
    if (obj->color_type == COLOR_TYPE_DETAIL) return true;
    
    for (int i = 0; i < obj->child_count; i++) {
        if (obj->children[i].color_type == COLOR_TYPE_DETAIL) return true;
    }
    return false;
}

bool is_valid_object(int id) {
    return id >= 1 && id < GAME_OBJECT_COUNT;
}

bool is_trigger_object(int id) {
    return id == COL_TRIGGER
        || id == MOVE_TRIGGER
        || id == ALPHA_TRIGGER
        || id == TOGGLE_TRIGGER
        || id == PULSE_TRIGGER
        || id == SPAWN_TRIGGER;
}

int parse_gd_object(const char *objStr, int obj) {
    int count = 0;
    // Split object into each key
    char **tokens = split_string(objStr, ',', &count, false);
    if (count < 1) {
        free_string_array(tokens, count);
        return 0;
    }

    // Iterate through all keys
    for (int i = 0; i + 1 < count; i += 2) {
        int key = atoi(tokens[i]);
        const char *valStr = tokens[i + 1];
        GDValueType type = get_value_type_for_key(key);
        GDValue val;

        switch (type) {
            case GD_VAL_INT:
                val.i = atoi(valStr);
                fill_object_data(obj, key, GD_VAL_INT, val);
                break;
            case GD_VAL_FLOAT:
                val.f = atof(valStr);
                fill_object_data(obj, key, GD_VAL_FLOAT, val);
                break;
            case GD_VAL_BOOL:
                val.b = parse_bool(valStr);
                fill_object_data(obj, key, GD_VAL_BOOL, val);
                break;
            case GD_VAL_HSV:
                val.hsv = parse_hsv_string(valStr);
                fill_object_data(obj, key, GD_VAL_HSV, val);
                break;
            case GD_VAL_INT_ARRAY:
                parse_ints(val.int_array, valStr);
                fill_object_data(obj, key, GD_VAL_INT_ARRAY, val);
                break;
            default:
                break;
        }
    }

    int obj_id = objects.id[obj];
    
    if (is_valid_object(obj_id)) {
        // Get coins
        if (obj_id == SECRET_COIN) {
            if (coin_count < 3) {
                coin_ids[coin_count++] = obj;
            }
        }
        
        const GameObject *game_object = &game_objects[objects.id[obj]];

        // Get proper color channels
        if (game_object->swap_base_detail) {
            if (!obj_has_main(game_object)) {
                if (!objects.col_channel[obj]) objects.col_channel[obj] = game_object->base_color;
            } else {
                if (!objects.detail_col_channel[obj]) objects.detail_col_channel[obj] = game_object->base_color;
            }
        } else {
            if (!objects.col_channel[obj]) objects.col_channel[obj] = game_object->base_color;
            if (!objects.detail_col_channel[obj]) objects.detail_col_channel[obj] = game_object->detail_color ? game_object->detail_color : 1;
        }

        // Give each object its own random value
        objects.random[obj] = rand();

        const ObjectHitbox *hitbox = game_object->hitbox;
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

            if (hitbox->collision_type == HITBOX_SOLID) {
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
                float s = fmaxf(fabsf(objects.scale_x[obj]), fabsf(objects.scale_y[obj]));
                objects.width[obj] *= s;
            } else if (hitbox->type == COLLISION_SLOPE) {
                objects.width[obj] *= fabsf(objects.scale_x[obj]);
                objects.height[obj] *= fabsf(objects.scale_y[obj]);
            } else if (hitbox->collision_type == HITBOX_SOLID) {
                if ((int) fabsf(objects.rotation[obj]) % 180 != 0) {
                    objects.width[obj] *= fabsf(objects.scale_y[obj]);
                    objects.height[obj] *= fabsf(objects.scale_x[obj]);
                } else {
                    objects.width[obj] *= fabsf(objects.scale_x[obj]);
                    objects.height[obj] *= fabsf(objects.scale_y[obj]);
                }
            } else {
                objects.width[obj] *= fabsf(objects.scale_x[obj]);
                objects.height[obj] *= fabsf(objects.scale_y[obj]);
            }
        }

        // Modify level ending pos
        if (objects.x[obj] > level_info.last_obj_x) {
            level_info.last_obj_x = objects.x[obj];
        }
    } else if (!is_trigger_object(obj_id)) { // Keep trigger objects even if outside the table
        // Invalid object
        objects.id[obj] = 0;
    }

    free_string_array(tokens, count);
    return 1;
}

void free_arrays() {
    if (objects.random)             { free(objects.random);             objects.random = NULL; }
    if (objects.id)                 { free(objects.id);                 objects.id = NULL; }
    if (objects.x)                  { free(objects.x);                  objects.x = NULL; }
    if (objects.y)                  { free(objects.y);                  objects.y = NULL; }
    if (objects.rotation)           { free(objects.rotation);           objects.rotation = NULL; }
    if (objects.zlayer)             { free(objects.zlayer);             objects.zlayer = NULL; }
    if (objects.zorder)             { free(objects.zorder);             objects.zorder = NULL; }
    if (objects.trig_duration)      { free(objects.trig_duration);      objects.trig_duration = NULL; }
    if (objects.opacity)            { free(objects.opacity);            objects.opacity = NULL; }
    if (objects.width)              { free(objects.width);              objects.width = NULL; }
    if (objects.height)             { free(objects.height);             objects.height = NULL; }
    if (objects.v1p9_col_channel)   { free(objects.v1p9_col_channel);   objects.v1p9_col_channel = NULL; }
    if (objects.col_channel)        { free(objects.col_channel);        objects.col_channel = NULL; }
    if (objects.detail_col_channel) { free(objects.detail_col_channel); objects.detail_col_channel = NULL; }
    if (objects.target_color_id)    { free(objects.target_color_id);    objects.target_color_id = NULL; }
    if (objects.hitbox_counter)     { free(objects.hitbox_counter);     objects.hitbox_counter = NULL; }
    if (objects.transition_applied) { free(objects.transition_applied); objects.transition_applied = NULL; }
    if (objects.trig_colorR)        { free(objects.trig_colorR);        objects.trig_colorR = NULL; }
    if (objects.trig_colorG)        { free(objects.trig_colorG);        objects.trig_colorG = NULL; }
    if (objects.trig_colorB)        { free(objects.trig_colorB);        objects.trig_colorB = NULL; }
    if (objects.orientation)        { free(objects.orientation);        objects.orientation = NULL; }
    if (objects.tintGround)         { free(objects.tintGround);         objects.tintGround = NULL; }
    if (objects.p1_color)           { free(objects.p1_color);           objects.p1_color = NULL; }
    if (objects.p2_color)           { free(objects.p2_color);           objects.p2_color = NULL; }
    if (objects.blending)           { free(objects.blending);           objects.blending = NULL; }
    if (objects.touch_triggered)    { free(objects.touch_triggered);    objects.touch_triggered = NULL; }
    if (objects.flippedH)           { free(objects.flippedH);           objects.flippedH = NULL; }
    if (objects.flippedV)           { free(objects.flippedV);           objects.flippedV = NULL; }
    if (objects.toggled)            { free(objects.toggled);            objects.toggled = NULL; }
    if (objects.groups)             { free(objects.groups);             objects.groups = NULL; }
    if (objects.group_count)        { free(objects.group_count);        objects.group_count = NULL; }
    if (objects.spawn_triggered)        { free(objects.spawn_triggered);        objects.spawn_triggered = NULL; }
    if (objects.multi_triggered)        { free(objects.multi_triggered);        objects.multi_triggered = NULL; }
    if (objects.spawn_delay)            { free(objects.spawn_delay);            objects.spawn_delay = NULL; }
    if (objects.target_group)           { free(objects.target_group);           objects.target_group = NULL; }
    if (objects.activate_group)         { free(objects.activate_group);         objects.activate_group = NULL; }
    if (objects.alpha_trigger_opacity)  { free(objects.alpha_trigger_opacity);  objects.alpha_trigger_opacity = NULL; }
    if (objects.trigger_opacity)        { free(objects.trigger_opacity);        objects.trigger_opacity = NULL; }
    if (objects.move_offset_x)          { free(objects.move_offset_x);          objects.move_offset_x = NULL; }
    if (objects.move_offset_y)          { free(objects.move_offset_y);          objects.move_offset_y = NULL; }
    if (objects.move_easing)            { free(objects.move_easing);            objects.move_easing = NULL; }
    if (objects.lock_to_player_x)       { free(objects.lock_to_player_x);       objects.lock_to_player_x = NULL; }
    if (objects.lock_to_player_y)       { free(objects.lock_to_player_y);       objects.lock_to_player_y = NULL; }
    if (objects.original_x)             { free(objects.original_x);             objects.original_x = NULL; }
    if (objects.original_y)             { free(objects.original_y);             objects.original_y = NULL; }
    if (objects.main_pulses)            { free(objects.main_pulses);            objects.main_pulses = NULL; }
    if (objects.detail_pulses)          { free(objects.detail_pulses);          objects.detail_pulses = NULL; }
    if (objects.num_main_pulses)        { free(objects.num_main_pulses);        objects.num_main_pulses = NULL; }
    if (objects.num_detail_pulses)      { free(objects.num_detail_pulses);      objects.num_detail_pulses = NULL; }
    if (objects.main_being_pulsed)      { free(objects.main_being_pulsed);      objects.main_being_pulsed = NULL; }
    if (objects.detail_being_pulsed)    { free(objects.detail_being_pulsed);    objects.detail_being_pulsed = NULL; }
    if (objects.main_non_pulse_color)   { free(objects.main_non_pulse_color);   objects.main_non_pulse_color = NULL; }
    if (objects.detail_non_pulse_color) { free(objects.detail_non_pulse_color); objects.detail_non_pulse_color = NULL; }
    if (objects.main_color)             { free(objects.main_color);             objects.main_color = NULL; }
    if (objects.detail_color)           { free(objects.detail_color);           objects.detail_color = NULL; }
    if (objects.pulse_fade_in)          { free(objects.pulse_fade_in);          objects.pulse_fade_in = NULL; }
    if (objects.pulse_hold)             { free(objects.pulse_hold);             objects.pulse_hold = NULL; }
    if (objects.pulse_fade_out)         { free(objects.pulse_fade_out);         objects.pulse_fade_out = NULL; }
    if (objects.pulse_mode)             { free(objects.pulse_mode);             objects.pulse_mode = NULL; }
    if (objects.copied_color_id)        { free(objects.copied_color_id);        objects.copied_color_id = NULL; }
    if (objects.copied_hsv)             { free(objects.copied_hsv);             objects.copied_hsv = NULL; }
    if (objects.main_col_HSV_enabled)   { free(objects.main_col_HSV_enabled);   objects.main_col_HSV_enabled = NULL; }
    if (objects.detail_col_HSV_enabled) { free(objects.detail_col_HSV_enabled); objects.detail_col_HSV_enabled = NULL; }
    if (objects.main_col_HSV)           { free(objects.main_col_HSV);           objects.main_col_HSV = NULL; }
    if (objects.detail_col_HSV)         { free(objects.detail_col_HSV);         objects.detail_col_HSV = NULL; }
    if (objects.pulse_target_type)      { free(objects.pulse_target_type);      objects.pulse_target_type = NULL; }
    if (objects.pulse_main_only)        { free(objects.pulse_main_only);        objects.pulse_main_only = NULL; }
    if (objects.pulse_detail_only)      { free(objects.pulse_detail_only);      objects.pulse_detail_only = NULL; }
    if (objects.cached_main_hsv_src_color)   { free(objects.cached_main_hsv_src_color);   objects.cached_main_hsv_src_color = NULL; }
    if (objects.cached_detail_hsv_src_color) { free(objects.cached_detail_hsv_src_color); objects.cached_detail_hsv_src_color = NULL; }
    if (objects.cached_main_hsv_color)       { free(objects.cached_main_hsv_color);       objects.cached_main_hsv_color = NULL; }
    if (objects.cached_detail_hsv_color)     { free(objects.cached_detail_hsv_color);     objects.cached_detail_hsv_color = NULL; }
    if (objects.cached_main_hsv_valid)       { free(objects.cached_main_hsv_valid);       objects.cached_main_hsv_valid = NULL; }
    if (objects.cached_detail_hsv_valid)     { free(objects.cached_detail_hsv_valid);     objects.cached_detail_hsv_valid = NULL; }
    if (objects.scale_x)                { free(objects.scale_x);                objects.scale_x = NULL; }
    if (objects.scale_y)                { free(objects.scale_y);                objects.scale_y = NULL; }
    if (objects.original_scale_x)       { free(objects.original_scale_x);       objects.original_scale_x = NULL; }
    if (objects.original_scale_y)       { free(objects.original_scale_y);       objects.original_scale_y = NULL; }
    if (objects.child_object)           { free(objects.child_object);           objects.child_object = NULL; }
    if (objects.tp_y_offset)            { free(objects.tp_y_offset);            objects.tp_y_offset = NULL; }
    if (objects.section_x)              { free(objects.section_x);              objects.section_x = NULL; }
    if (objects.section_y)              { free(objects.section_y);              objects.section_y = NULL; }
    if (objects.dirty)              { free(objects.dirty);              objects.dirty = NULL; }
    if (objects.render_visible)     { free(objects.render_visible);     objects.render_visible = NULL; }
    if (objects.render_seen)        { free(objects.render_seen);        objects.render_seen = NULL; }
    if (objects.activated)          { free(objects.activated);          objects.activated = NULL; }
    if (objects.collided)           { free(objects.collided);           objects.collided = NULL; }
}

bool init_arrays(int count) {
    objects.random = malloc(sizeof(int) * count);
    if (!objects.random) return false;

    objects.id = malloc(sizeof(int) * count);
    if (!objects.id) return false;
    
    objects.x = malloc(sizeof(float) * count);
    if (!objects.x) return false;
    
    objects.y = malloc(sizeof(float) * count);
    if (!objects.y) return false;

    objects.rotation = malloc(sizeof(float) * count);
    if (!objects.rotation) return false;

    objects.zlayer = malloc(sizeof(int) * count);
    if (!objects.zlayer) return false;
    
    objects.zorder = malloc(sizeof(int) * count);
    if (!objects.zorder) return false;
    
    objects.trig_duration = malloc(sizeof(float) * count);
    if (!objects.trig_duration) return false;
    
    objects.opacity = malloc(sizeof(float) * count);
    if (!objects.opacity) return false;
    
    objects.width = malloc(sizeof(float) * count);
    if (!objects.width) return false;
    
    objects.height = malloc(sizeof(float) * count);
    if (!objects.height) return false;

    objects.v1p9_col_channel = malloc(sizeof(unsigned short) * count);
    if (!objects.v1p9_col_channel) return false;
    
    objects.col_channel = malloc(sizeof(unsigned short) * count);
    if (!objects.col_channel) return false;
    
    objects.detail_col_channel = malloc(sizeof(unsigned short) * count);
    if (!objects.detail_col_channel) return false;
    
    objects.target_color_id = malloc(sizeof(unsigned short) * count);
    if (!objects.target_color_id) return false;
    
    objects.hitbox_counter = malloc(sizeof(unsigned short) * count);
    if (!objects.hitbox_counter) return false;

    objects.transition_applied = malloc(sizeof(unsigned char) * count);
    if (!objects.transition_applied) return false;
    
    objects.trig_colorR = malloc(sizeof(unsigned char) * count);
    if (!objects.trig_colorR) return false;
    
    objects.trig_colorG = malloc(sizeof(unsigned char) * count);
    if (!objects.trig_colorG) return false;
    
    objects.trig_colorB = malloc(sizeof(unsigned char) * count);
    if (!objects.trig_colorB) return false;
    
    objects.orientation = malloc(sizeof(unsigned char) * count);
    if (!objects.orientation) return false;
    
    objects.tintGround = malloc(sizeof(bool) * count);
    if (!objects.tintGround) return false;
    
    objects.p1_color = malloc(sizeof(bool) * count);
    if (!objects.p1_color) return false;
    
    objects.p2_color = malloc(sizeof(bool) * count);
    if (!objects.p2_color) return false;
    
    objects.blending = malloc(sizeof(bool) * count);
    if (!objects.blending) return false;
    
    objects.touch_triggered = malloc(sizeof(bool) * count);
    if (!objects.touch_triggered) return false;
    
    objects.flippedH = malloc(sizeof(bool) * count);
    if (!objects.flippedH) return false;
    
    objects.flippedV = malloc(sizeof(bool) * count);
    if (!objects.flippedV) return false;
    
    objects.toggled = malloc(sizeof(bool) * count);
    if (!objects.toggled) return false;

    objects.groups = malloc(sizeof(short[MAX_GROUPS_PER_OBJECT]) * count);
    if (!objects.groups) return false;

    objects.group_count = malloc(sizeof(u8) * count);
    if (!objects.group_count) return false;

    objects.dirty = malloc(sizeof(bool) * count);
    if (!objects.dirty) return false;

    objects.render_visible = malloc(sizeof(bool) * count);
    if (!objects.render_visible) return false;

    objects.render_seen = malloc(sizeof(bool) * count);
    if (!objects.render_seen) return false;
    
    objects.activated = malloc(sizeof(u8) * count);
    if (!objects.activated) return false;
    
    objects.collided = malloc(sizeof(u8) * count);
    if (!objects.collided) return false;

    objects.original_x = malloc(sizeof(float) * count);
    if (!objects.original_x) return false;

    objects.original_y = malloc(sizeof(float) * count);
    if (!objects.original_y) return false;

    objects.spawn_triggered = malloc(sizeof(bool) * count);
    if (!objects.spawn_triggered) return false;

    objects.multi_triggered = malloc(sizeof(bool) * count);
    if (!objects.multi_triggered) return false;

    objects.spawn_delay = malloc(sizeof(float) * count);
    if (!objects.spawn_delay) return false;

    objects.target_group = malloc(sizeof(int) * count);
    if (!objects.target_group) return false;

    objects.activate_group = malloc(sizeof(bool) * count);
    if (!objects.activate_group) return false;

    objects.alpha_trigger_opacity = malloc(sizeof(float) * count);
    if (!objects.alpha_trigger_opacity) return false;

    objects.trigger_opacity = malloc(sizeof(float) * count);
    if (!objects.trigger_opacity) return false;

    objects.move_offset_x = malloc(sizeof(float) * count);
    if (!objects.move_offset_x) return false;

    objects.move_offset_y = malloc(sizeof(float) * count);
    if (!objects.move_offset_y) return false;

    objects.move_easing = malloc(sizeof(int) * count);
    if (!objects.move_easing) return false;

    objects.lock_to_player_x = malloc(sizeof(bool) * count);
    if (!objects.lock_to_player_x) return false;

    objects.lock_to_player_y = malloc(sizeof(bool) * count);
    if (!objects.lock_to_player_y) return false;

    objects.main_pulses = malloc(sizeof(Color[MAX_PULSES_PER_GROUP]) * count);
    if (!objects.main_pulses) return false;

    objects.detail_pulses = malloc(sizeof(Color[MAX_PULSES_PER_GROUP]) * count);
    if (!objects.detail_pulses) return false;

    objects.num_main_pulses = malloc(sizeof(u8) * count);
    if (!objects.num_main_pulses) return false;

    objects.num_detail_pulses = malloc(sizeof(u8) * count);
    if (!objects.num_detail_pulses) return false;

    objects.main_being_pulsed = malloc(sizeof(bool) * count);
    if (!objects.main_being_pulsed) return false;

    objects.detail_being_pulsed = malloc(sizeof(bool) * count);
    if (!objects.detail_being_pulsed) return false;

    objects.main_non_pulse_color = malloc(sizeof(Color) * count);
    if (!objects.main_non_pulse_color) return false;

    objects.detail_non_pulse_color = malloc(sizeof(Color) * count);
    if (!objects.detail_non_pulse_color) return false;

    objects.main_color = malloc(sizeof(Color) * count);
    if (!objects.main_color) return false;

    objects.detail_color = malloc(sizeof(Color) * count);
    if (!objects.detail_color) return false;

    objects.pulse_fade_in = malloc(sizeof(float) * count);
    if (!objects.pulse_fade_in) return false;

    objects.pulse_hold = malloc(sizeof(float) * count);
    if (!objects.pulse_hold) return false;

    objects.pulse_fade_out = malloc(sizeof(float) * count);
    if (!objects.pulse_fade_out) return false;

    objects.pulse_mode = malloc(sizeof(int) * count);
    if (!objects.pulse_mode) return false;

    objects.copied_color_id = malloc(sizeof(int) * count);
    if (!objects.copied_color_id) return false;

    objects.copied_hsv = malloc(sizeof(HSV) * count);
    if (!objects.copied_hsv) return false;

    objects.main_col_HSV_enabled = malloc(sizeof(bool) * count);
    if (!objects.main_col_HSV_enabled) return false;

    objects.detail_col_HSV_enabled = malloc(sizeof(bool) * count);
    if (!objects.detail_col_HSV_enabled) return false;

    objects.main_col_HSV = malloc(sizeof(HSV) * count);
    if (!objects.main_col_HSV) return false;

    objects.detail_col_HSV = malloc(sizeof(HSV) * count);
    if (!objects.detail_col_HSV) return false;

    objects.cached_main_hsv_src_color = malloc(sizeof(Color) * count);
    if (!objects.cached_main_hsv_src_color) return false;

    objects.cached_detail_hsv_src_color = malloc(sizeof(Color) * count);
    if (!objects.cached_detail_hsv_src_color) return false;

    objects.cached_main_hsv_color = malloc(sizeof(Color) * count);
    if (!objects.cached_main_hsv_color) return false;

    objects.cached_detail_hsv_color = malloc(sizeof(Color) * count);
    if (!objects.cached_detail_hsv_color) return false;

    objects.cached_main_hsv_valid = malloc(sizeof(bool) * count);
    if (!objects.cached_main_hsv_valid) return false;

    objects.cached_detail_hsv_valid = malloc(sizeof(bool) * count);
    if (!objects.cached_detail_hsv_valid) return false;

    objects.pulse_target_type = malloc(sizeof(int) * count);
    if (!objects.pulse_target_type) return false;

    objects.pulse_main_only = malloc(sizeof(bool) * count);
    if (!objects.pulse_main_only) return false;

    objects.pulse_detail_only = malloc(sizeof(bool) * count);
    if (!objects.pulse_detail_only) return false;

    objects.scale_x = malloc(sizeof(float) * count);
    if (!objects.scale_x) return false;

    objects.scale_y = malloc(sizeof(float) * count);
    if (!objects.scale_y) return false;

    objects.original_scale_x = malloc(sizeof(float) * count);
    if (!objects.original_scale_x) return false;

    objects.original_scale_y = malloc(sizeof(float) * count);
    if (!objects.original_scale_y) return false;

    objects.child_object = malloc(sizeof(int) * count);
    if (!objects.child_object) return false;

    objects.tp_y_offset = malloc(sizeof(float) * count);
    if (!objects.tp_y_offset) return false;

    objects.section_x = malloc(sizeof(int) * count);
    if (!objects.section_x) return false;

    objects.section_y = malloc(sizeof(int) * count);
    if (!objects.section_y) return false;

    // Initialize the values
    memset(objects.random,             0, sizeof(int) * count);
    memset(objects.id,                 0, sizeof(int) * count);
    memset(objects.x,                  0, sizeof(float) * count);
    memset(objects.y,                  0, sizeof(float) * count);
    memset(objects.rotation,           0, sizeof(float) * count);
    memset(objects.zlayer,             0, sizeof(int) * count);
    memset(objects.zorder,             0, sizeof(int) * count);
    memset(objects.trig_duration,      0, sizeof(float) * count);
    memset(objects.opacity,            0, sizeof(float) * count);
    memset(objects.width,              0, sizeof(float) * count);
    memset(objects.height,             0, sizeof(float) * count);
    memset(objects.v1p9_col_channel,   0, sizeof(unsigned short) * count);
    memset(objects.col_channel,        0, sizeof(unsigned short) * count);
    memset(objects.detail_col_channel, 0, sizeof(unsigned short) * count);
    memset(objects.target_color_id,    0, sizeof(unsigned short) * count);
    memset(objects.hitbox_counter,     0, sizeof(unsigned short) * count);
    memset(objects.transition_applied, 0, sizeof(unsigned char) * count);
    memset(objects.trig_colorR,        0, sizeof(unsigned char) * count);
    memset(objects.trig_colorG,        0, sizeof(unsigned char) * count);
    memset(objects.trig_colorB,        0, sizeof(unsigned char) * count);
    memset(objects.orientation,        0, sizeof(unsigned char) * count);
    memset(objects.tintGround,         0, sizeof(bool) * count);
    memset(objects.p1_color,           0, sizeof(bool) * count);
    memset(objects.p2_color,           0, sizeof(bool) * count);
    memset(objects.blending,           0, sizeof(bool) * count);
    memset(objects.touch_triggered,    0, sizeof(bool) * count);
    memset(objects.flippedH,           0, sizeof(bool) * count);
    memset(objects.flippedV,           0, sizeof(bool) * count);
    memset(objects.toggled,            0, sizeof(bool) * count);
    memset(objects.groups,             0, sizeof(short[MAX_GROUPS_PER_OBJECT]) * count);
    memset(objects.group_count,        0, sizeof(u8) * count);
    memset(objects.dirty,              1, sizeof(bool) * count); // Dirty by default (needs to be created lol)
    memset(objects.render_visible,     0, sizeof(bool) * count);
    memset(objects.render_seen,        0, sizeof(u8) * count);
    memset(objects.activated,          0, sizeof(u8) * count);
    memset(objects.collided,           0, sizeof(u8) * count);

    for (int i = 0; i < count; i++) {
        objects.alpha_trigger_opacity[i] = 1.0f;
        objects.trigger_opacity[i] = 1.0f;
        objects.scale_x[i] = 1.0f;
        objects.scale_y[i] = 1.0f;
        objects.original_scale_x[i] = 1.0f;
        objects.original_scale_y[i] = 1.0f;
    }

    memset(objects.spawn_triggered,    0, sizeof(bool) * count);
    memset(objects.multi_triggered,    0, sizeof(bool) * count);
    memset(objects.spawn_delay,        0, sizeof(float) * count);
    memset(objects.target_group,       0, sizeof(int) * count);
    memset(objects.activate_group,     0, sizeof(bool) * count);
    memset(objects.move_offset_x,      0, sizeof(float) * count);
    memset(objects.move_offset_y,      0, sizeof(float) * count);
    memset(objects.move_easing,        0, sizeof(int) * count);
    memset(objects.lock_to_player_x,   0, sizeof(bool) * count);
    memset(objects.lock_to_player_y,   0, sizeof(bool) * count);

    memset(objects.main_pulses,        0, sizeof(Color[MAX_PULSES_PER_GROUP]) * count);
    memset(objects.detail_pulses,      0, sizeof(Color[MAX_PULSES_PER_GROUP]) * count);
    memset(objects.num_main_pulses,    0, sizeof(u8) * count);
    memset(objects.num_detail_pulses,  0, sizeof(u8) * count);
    memset(objects.main_being_pulsed,  0, sizeof(bool) * count);
    memset(objects.detail_being_pulsed,0, sizeof(bool) * count);

    for (int i = 0; i < count; i++) {
        objects.main_non_pulse_color[i].r = 255;
        objects.main_non_pulse_color[i].g = 255;
        objects.main_non_pulse_color[i].b = 255;
        objects.detail_non_pulse_color[i].r = 255;
        objects.detail_non_pulse_color[i].g = 255;
        objects.detail_non_pulse_color[i].b = 255;
        objects.main_color[i].r = 255;
        objects.main_color[i].g = 255;
        objects.main_color[i].b = 255;
        objects.detail_color[i].r = 255;
        objects.detail_color[i].g = 255;
        objects.detail_color[i].b = 255;
    }

    memset(objects.pulse_fade_in,      0, sizeof(float) * count);
    memset(objects.pulse_hold,         0, sizeof(float) * count);
    memset(objects.pulse_fade_out,     0, sizeof(float) * count);
    memset(objects.pulse_mode,         0, sizeof(int) * count);
    memset(objects.copied_color_id,    0, sizeof(int) * count);
    memset(objects.copied_hsv,             0, sizeof(HSV) * count);
    memset(objects.main_col_HSV_enabled,   0, sizeof(bool) * count);
    memset(objects.detail_col_HSV_enabled, 0, sizeof(bool) * count);
    memset(objects.main_col_HSV,           0, sizeof(HSV) * count);
    memset(objects.detail_col_HSV,         0, sizeof(HSV) * count);
    memset(objects.cached_main_hsv_src_color,  0, sizeof(Color) * count);
    memset(objects.cached_detail_hsv_src_color, 0, sizeof(Color) * count);
    memset(objects.cached_main_hsv_color,       0, sizeof(Color) * count);
    memset(objects.cached_detail_hsv_color,     0, sizeof(Color) * count);
    memset(objects.cached_main_hsv_valid,       0, sizeof(bool) * count);
    memset(objects.cached_detail_hsv_valid,     0, sizeof(bool) * count);
    memset(objects.pulse_target_type,      0, sizeof(int) * count);
    memset(objects.pulse_main_only,    0, sizeof(bool) * count);
    memset(objects.pulse_detail_only,  0, sizeof(bool) * count);

    for (int i = 0; i < count; i++) {
        objects.child_object[i] = -1;
        objects.tp_y_offset[i] = 0.0f;
    }

    memset(objects.section_x,           0, sizeof(int) * count);
    memset(objects.section_y,           0, sizeof(int) * count);

    return true;
}

int compare_coins(const void *a, const void *b) {
    const int *c1 = (const int *)a;
    const int *c2 = (const int *)b;

    return objects.x[*c1] - objects.x[*c2];
}

int parse_string(const char *levelString, int extra_slots, int *out_object_count) {
    int sectionCount = 0;

    // Split the string in object sections
    char **sections = split_string(levelString, ';', &sectionCount, false);

    if (sectionCount < 1) {
        output_log("Level string missing sections!\n");
        free_string_array(sections, sectionCount);
        return LOAD_LEVEL_STRING_MISSING_SECTIONS;
    }
    
    int objectCount = sectionCount - 1;

    printf("%d\n", objectCount);
    
    if (!init_arrays(objectCount + extra_slots)) {
        free_arrays();
        output_log("Failed to allocate object array\n");
        return LOAD_OUT_OF_MEMORY;
    }

    objects.count = objectCount + extra_slots;

    if (out_object_count) *out_object_count = objectCount;

    printf("Parsing string and converting objects...\n");
    printf("Aproximately %d bytes of pure objects\n", sizeof(ObjectsArray) * objectCount);
    
    coin_count = 0;

    for (int i = 0; i < objectCount; i++) {
        // Parse
        if (!parse_gd_object(sections[i + 1], i)) {
            output_log("Failed to parse object %d\n", i);
            free_string_array(sections, sectionCount);
            return LOAD_COULDNT_PARSE_OBJECTS;
        }

        assign_object_to_section(i);
    }

    for (int i = 0; i < objectCount; i++) {
        for (int g = 0; g < objects.group_count[i]; g++) {
            int group_id = objects.groups[i][g];
            if (group_id > 0) {
                add_to_group(i, group_id);
            }
        }
    }

    qsort(coin_ids, coin_count, sizeof(int), compare_coins);

    // Set IDs here
    for (int i = 0; i < coin_count; i++) {
        objects.coin_id[coin_ids[i]] = i;
        //output_log("Le coin %d on objeto %d\n", i, coin_ids[i]);
    }
    
    level_info.last_obj_x += (11 * 30.f);

    free_string_array(sections, sectionCount);

    return LOAD_NO_ERROR;
}

void set_color_channels() {
    for (int i = 0; i < channelCount; i++) {
        GDColorChannel colorChannel = colorChannels[i];
        int id = colorChannel.channelID;

        switch (id) {
            case CHANNEL_P1:
            case CHANNEL_P2:
                break;

            default:
                if (id >= 0 && id < COL_CHANNEL_NUM) {
                    int chan = get_col_channel_index(id);

                    Color color;
                    color.r = colorChannel.fromRed;
                    color.g = colorChannel.fromGreen;
                    color.b = colorChannel.fromBlue;

                    channels[chan].blending = colorChannel.blending;
                    channels[chan].color = color;
                    channels[chan].non_pulse_color = color;
                    channels[chan].alpha = colorChannel.fromOpacity < 0.0f ? 0.0f : (colorChannel.fromOpacity > 1.0f ? 1.0f : colorChannel.fromOpacity);

                    if (colorChannel.inheritedChannelID > 0) {
                        channels[chan].copy_color_id = colorChannel.inheritedChannelID;
                        channels[chan].hsv = colorChannel.hsv;
                    } else {
                        channels[chan].copy_color_id = 0;
                    }

                    if (colorChannel.playerColor == 1) {
                        channels[chan].color = get_p2_if_black(p1_color);
                        channels[chan].non_pulse_color = channels[chan].color;
                    }
                    if (colorChannel.playerColor == 2) {
                        channels[chan].color = get_p1_if_black(p2_color);
                        channels[chan].non_pulse_color = channels[chan].color;
                    }

                    if (id == CHANNEL_OBJ) {
                        channels[get_col_channel_index(CHANNEL_OBJ_BLENDING)].color = color;
                        channels[get_col_channel_index(CHANNEL_OBJ_BLENDING)].non_pulse_color = color;
                    }
                }
        }
    }

    for (int i = 0; i < COL_CHANNEL_NUM; i++)
        channels[i].non_pulse_color = channels[i].color;
}

const char *bg_sheet_paths[] = {
    "romfs:/gfx/bg_sheet_01.t3x",
    "romfs:/gfx/bg_sheet_02.t3x",
    "romfs:/gfx/bg_sheet_03.t3x",
    "romfs:/gfx/bg_sheet_04.t3x"
};

void load_level_string_info(char *level_string) {
    char *gmd_song_offset = get_metadata_value(level_string, "kA13");
    if (gmd_song_offset) {
        level_info.song_offset = atof(gmd_song_offset);
        free(gmd_song_offset);
    } else {
        level_info.song_offset = 0;
    }

    char *background_data = get_metadata_value(level_string, "kA6");
    if (background_data) {
        level_info.background_id = CLAMP(atoi(background_data) - 1, 0, BG_COUNT - 1);
        free(background_data);
    } else {
        level_info.background_id = 0;
    }

    // load the background sheet on demand
    int new_sheet = level_info.background_id / 4;
    if (loaded_bg_sheet != new_sheet) {
        C2D_SpriteSheetFree(bgSheet);
        bgSheet = C2D_SpriteSheetLoad(bg_sheet_paths[new_sheet]);
        loaded_bg_sheet = new_sheet;
    }

    char *ground_data = get_metadata_value(level_string, "kA7");
    if (ground_data) {
        level_info.ground_id = CLAMP(atoi(ground_data) - 1, 0, G_COUNT - 1);
        free(ground_data);
    } else {
        level_info.ground_id = 0;
    }
    
    char *gamemode_data = get_metadata_value(level_string, "kA2");
    if (gamemode_data) {
        level_info.initial_gamemode = CLAMP(atoi(gamemode_data), 0, GAMEMODE_COUNT - 1);
        free(gamemode_data);
    } else {
        level_info.initial_gamemode = GAMEMODE_PLAYER;
    }

    char *mini_data = get_metadata_value(level_string, "kA3");
    if (mini_data) {
        level_info.initial_mini = atoi(mini_data) != 0;    
        free(mini_data);
    } else {
        level_info.initial_mini = 0; 
    }

    char *speed_data = get_metadata_value(level_string, "kA4");
    if (speed_data) {
        level_info.initial_speed = CLAMP(atoi(speed_data), 0, SPEED_COUNT - 1);
        if (level_info.initial_speed == 0) level_info.initial_speed = SPEED_NORMAL;
        else if (level_info.initial_speed == 1) level_info.initial_speed = SPEED_SLOW;
        free(speed_data);
    } else {
        level_info.initial_speed = SPEED_NORMAL;
    }

    char *dual_data = get_metadata_value(level_string, "kA8");
    if (dual_data) {
        level_info.initial_dual = atoi(dual_data) != 0;
        free(dual_data);
    } else {
        level_info.initial_dual = 0; 
    }
    
    char *two_player_mode_data = get_metadata_value(level_string, "kA10");
    if (two_player_mode_data) {
        level_info.two_player_mode = atoi(two_player_mode_data) != 0;
        free(two_player_mode_data);
    } else {
        level_info.two_player_mode = 0; 
    }

    char *upsidedown_data = get_metadata_value(level_string, "kA11");
    if (upsidedown_data) {
        level_info.initial_upsidedown = atoi(upsidedown_data) != 0;
        free(upsidedown_data);
    } else {
        level_info.initial_upsidedown = 0; 
    }
    
}

const char *default_name = "Unknown";

void load_online_level_info(char *level_string) {
    load_level_string_info(level_string);
    
    level_info.song_id = search_entries[curr_search_id].mainSongId;
    level_info.custom_song_id = search_entries[curr_search_id].songId;
    snprintf(level_info.level_name, sizeof(level_info.level_name), "%s", search_entries[curr_search_id].name);
    snprintf(level_info.creator_name, sizeof(level_info.level_name), "%s", creator_entries[search_entries[curr_search_id].creatorIndex].creatorName);
}

static void generate_orange_portals(int orange_start) {
    int next_orange = orange_start;
    for (int i = 0; i < orange_start; i++) {
        if (objects.id[i] != BLUE_TP_PORTAL) continue;

        float angle_rad = objects.rotation[i] * (M_PI / 180.0f);
        float x_off = 10.0f * fabsf(cosf(angle_rad));

        int oi = next_orange++;
        objects.id[oi]           = ORANGE_TP_PORTAL;
        objects.x[oi]            = objects.x[i] - x_off;
        objects.y[oi]            = objects.y[i] + objects.tp_y_offset[i];
        objects.rotation[oi]     = objects.rotation[i] + 180.0f;
        objects.flippedH[oi]     = false;
        objects.flippedV[oi]     = false;
        objects.opacity[oi]      = 1.0f;
        objects.toggled[oi]      = false;
        objects.activated[oi]    = 0;
        objects.collided[oi]     = 0;
        objects.child_object[oi] = -1;
        objects.tp_y_offset[oi]  = 0.0f;
        objects.original_x[oi]   = objects.x[oi];
        objects.original_y[oi]   = objects.y[oi];
        objects.child_object[i]  = oi;

        // Copy groups from blue portal to orange portal
        objects.group_count[oi] = objects.group_count[i];
        for (int g = 0; g < objects.group_count[i] && g < MAX_GROUPS_PER_OBJECT; g++) {
            objects.groups[oi][g] = objects.groups[i][g];
            if (objects.groups[i][g] > 0) {
                add_to_group(oi, objects.groups[i][g]);
            }
        }

        assign_object_to_section(oi);
    }
    objects.count = next_orange;
}

int load_online_level(char *level_string) {
    bool compressed = true;
    int out_code = LOAD_NO_ERROR;

    // Base64 doesn't allow semicolons, so if theres one, its not compressed
    if (strchr(level_string, ';')) compressed = false;
    char *data;
    if (compressed) {
        data = decompress_online_level(level_string, &out_code);
        if (!data) return out_code;
    } else {
        data = strdup(level_string);
        if (!data) return LOAD_OUT_OF_MEMORY;
    }

    // Get level starting colors
    char *metaStr = get_metadata_value(data, "kS38");
    if (metaStr) {
        channelCount = parse_color_channels(metaStr, &colorChannels, &out_code);
        if (out_code) {
            return out_code;
        }
    }
    // Fallback to pre 2.0 color keys
    else {
        channelCount = parse_old_channels(data, &colorChannels, &out_code);
        if (out_code) {
            return out_code;
        }
    }

    load_online_level_info(data);

    // Minimum size
    level_info.last_obj_x = 570.f;

    // Count blue teleport portals to reserve extra object slots
    int extra_slots = 0;
    const char *scan = data;
    while ((scan = strstr(scan, "1,747,")) != NULL) {
        extra_slots++;
        scan++;
    }
    int orange_start = 0;

    int returned = parse_string(data, extra_slots, &orange_start);

    free(data);
    free(metaStr);

    if (returned) return returned;

    generate_orange_portals(orange_start);

    init_col_channels();
    set_color_channels();

    // Set pulserod pulse ball image
    current_pulserod_ball_image = game_objects[15].children[0].texture + (rand() % 3);

    C2D_SpriteFromSheet(&sprite_templates[15].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[15].child_templates[0], 0.5f, 0.5f);
    
    C2D_SpriteFromSheet(&sprite_templates[16].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[16].child_templates[0], 0.5f, 0.5f);

    C2D_SpriteFromSheet(&sprite_templates[17].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[17].child_templates[0], 0.5f, 0.5f);

    return LOAD_NO_ERROR;
}

void load_level_info(char *data, char *level_string) {
    load_level_string_info(level_string);

    char *gmd_song_id = extract_gmd_key((const char *) data, "k8", "i");
    if (!gmd_song_id) {
        level_info.song_id = 0; // Stereo Madness
    } else {
        level_info.song_id = atoi(gmd_song_id); // Official song id
        free(gmd_song_id);
    }

    char *gmd_custom_song_id = extract_gmd_key((const char *) data, "k45", "i");
    if (!gmd_custom_song_id) {
        level_info.custom_song_id = -1;
    } else {
        level_info.custom_song_id = atoi(gmd_custom_song_id); // Custom song id
        free(gmd_custom_song_id);
    }

    char *level_name_data = extract_gmd_key((const char *) data, "k2", "s");
    if (level_name_data) {
        snprintf(level_info.level_name, sizeof(level_info.level_name), "%s", level_name_data);
        free(level_name_data);
    } else {
        snprintf(level_info.level_name, sizeof(level_info.level_name), "%s", default_name);
    }

    char *creator_name_data = extract_gmd_key((const char *) data, "k5", "s");
    if (creator_name_data) {
        snprintf(level_info.creator_name, sizeof(level_info.creator_name), "%s", creator_name_data);
        free(creator_name_data);
    } else {
        snprintf(level_info.creator_name, sizeof(level_info.creator_name), "%s", default_name);
    }
}

int load_level(char *path) {
    int out_code = LOAD_NO_ERROR;

    size_t out;
    char *level = read_file(path, &out);
    if (!level) return 1;

    char *data = decompress_level(level, &out_code);
    if (!data) {
        free(level);
        return out_code;
    }

    // Get level starting colors
    char *metaStr = get_metadata_value(data, "kS38");
    if (metaStr) {
        channelCount = parse_color_channels(metaStr, &colorChannels, &out_code);
        if (out_code) {
            return out_code;
        }
    }
    // Fallback to pre 2.0 color keys
    else {
        channelCount = parse_old_channels(data, &colorChannels, &out_code);
        if (out_code) {
            return out_code;
        }
    }

    load_level_info(level, data);

    // Minimum size
    level_info.last_obj_x = 570.f;

    // Count blue teleport portals to reserve extra object slots
    int extra_slots = 0;
    const char *scan = data;
    while ((scan = strstr(scan, "1,747,")) != NULL) {
        extra_slots++;
        scan++;
    }
    int orange_start = 0;

    int returned = parse_string(data, extra_slots, &orange_start);

    free(data);
    free(metaStr);
    free(level);

    if (returned) return returned;

    generate_orange_portals(orange_start);

    init_col_channels();
    set_color_channels();

    // Set pulserod pulse ball image
    current_pulserod_ball_image = game_objects[15].children[0].texture + (rand() % 3);

    C2D_SpriteFromSheet(&sprite_templates[15].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[15].child_templates[0], 0.5f, 0.5f);
    
    C2D_SpriteFromSheet(&sprite_templates[16].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[16].child_templates[0], 0.5f, 0.5f);

    C2D_SpriteFromSheet(&sprite_templates[17].child_templates[0], spriteSheet, current_pulserod_ball_image);
    C2D_SpriteSetCenter(&sprite_templates[17].child_templates[0], 0.5f, 0.5f);

    return LOAD_NO_ERROR;
}

void reload_level() {
    clear_groups();

    memset(alpha_trigger_buffer, 0, sizeof(alpha_trigger_buffer));

    for (int i = 0; i < objects.count; i++) {
        objects.activated[i] = false;
        objects.collided[i] = false;
        objects.hitbox_counter[i] = 0;
        objects.transition_applied[i] = FADE_NONE;
        objects.toggled[i] = false;
        objects.opacity[i] = 1.f;
        objects.x[i] = objects.original_x[i];
        objects.y[i] = objects.original_y[i];
        objects.alpha_trigger_opacity[i] = 1.0f;
        objects.scale_x[i] = objects.original_scale_x[i];
        objects.scale_y[i] = objects.original_scale_y[i];
        objects.num_main_pulses[i] = 0;
        objects.num_detail_pulses[i] = 0;
        objects.main_being_pulsed[i] = false;
        objects.detail_being_pulsed[i] = false;
        objects.main_color[i] = (Color){255, 255, 255};
        objects.detail_color[i] = (Color){255, 255, 255};
        objects.child_object[i] = -1;
        objects.tp_y_offset[i] = 0.0f;
        objects.cached_main_hsv_valid[i] = false;
        objects.cached_detail_hsv_valid[i] = false;
    }

    for (int i = 0; i < objects.count; i++) {
        update_object_section(i);
    }

    accumulator = 0.f;
    fixed_dt = true;

    init_col_channels();
    set_color_channels();

    for (int i = 0; i < objects.count; i++) {
        for (int g = 0; g < objects.group_count[i]; g++) {
            int group_id = objects.groups[i][g];
            if (group_id > 0) {
                add_to_group(i, group_id);
            }
        }
    }
}

void unload_level() {
    clear_groups();
    reset_render_cache();
    free_arrays();

    memset(alpha_trigger_buffer, 0, sizeof(alpha_trigger_buffer));
    free_sections();
    free_object_particles();
    
    channelCount = 0;
    if (colorChannels) {
        free(colorChannels);
        colorChannels = NULL;
    }

    stop_mp3();
}

char *get_level_name(char *data_ptr) {
    return extract_gmd_key((const char *) data_ptr, "k2", "s");
}

char *load_user_song(int id, size_t *out_size) {
    char full_path[273];
    snprintf(full_path, sizeof(full_path), "%s/%d.mp3", USER_SONGS_DIR, id);
    return read_file(full_path, out_size);
}

bool check_song(int id) {
    char full_path[273];
    snprintf(full_path, sizeof(full_path), "%s/%d.mp3", USER_SONGS_DIR, id);
    return access(full_path, F_OK) == 0;
}