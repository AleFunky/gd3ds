#include <stdlib.h>
#include <stdbool.h>
#include "level_loading.h"
#include "defines.h"
#include <zlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "color_channels.h"
#include "objects.h"
#include "mp3_player.h"
#include "graphics.h"
#include "math_helpers.h"
#include "utils/json_config.h"
#include "state.h"

#include "player/collision.h"

ObjectsArray objects = { 0 };

Section *section_hash[SECTION_HASH_SIZE] = {0};

int channelCount = 0;
GDColorChannel *colorChannels = NULL;

LoadedLevelInfo level_info;

static inline unsigned int section_hash_func(unsigned int x, unsigned int y) {
    return ((unsigned int)x * 73856093u ^ (unsigned int)y * 19349663u) & (SECTION_HASH_SIZE - 1);
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
}

char *read_file(const char *filepath, size_t *out_size) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("Failed to open file: %s\n", filepath);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        printf("Failed to allocate file\n");
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
        printf("Expected start tag '%s' not found after key\n", type_start_tag);
        return NULL;
    }

    // Move past the type start tag
    start += strlen(type_start_tag);

    // Find the end tag
    char type_end_tag[16];
    snprintf(type_end_tag, sizeof(type_end_tag), "</%s>", type);
    char *end = strstr(start, type_end_tag);
    if (!end) {
        printf("Could not find end tag '%s'\n", type_end_tag);
        return NULL;
    }

    // Allocate and copy value
    int len = end - start;
    char *value = malloc(len + 1);
    if (!value) {
        printf("malloc for gmd key %s failed\n", key);
        return NULL;
    }
    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

bool is_ascii(const unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        if (data[i] > 0x7F) {
            return FALSE; // Non-ASCII byte found
        }
    }
    return TRUE;
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
            printf("Invalid base64 character at position %d\n", i);
            return -1;
        }

        out[len++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[len++] = (b << 4) | (c >> 2);
        if (in[i+3] != '=') out[len++] = (c << 6) | d;
    }
    return len;
}

uLongf get_uncompressed_size(unsigned char *data, int data_len) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {  // auto-detect gzip/zlib
        return 0;
    }

    uLongf total_out = 0;
    unsigned char buf[4096];

    do {
        strm.next_out = buf;
        strm.avail_out = sizeof(buf);
        int ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return 0;
        }
        total_out += sizeof(buf) - strm.avail_out;
        if (ret == Z_STREAM_END) break;
    } while (strm.avail_in > 0);

    inflateEnd(&strm);
    return total_out;
}


char *decompress_data(unsigned char *data, int data_len, uLongf *out_len) {
    uLongf final_size = get_uncompressed_size(data, data_len);
    printf("Decompressing to a final size of %lu bytes...\n", (unsigned long)final_size);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        return NULL;
    }

    // Allocate exactly enough memory (+1 for null terminator if needed)
    char *out = malloc(final_size + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)final_size);
        inflateEnd(&strm);
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = final_size;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        return NULL;
    }

    *out_len = strm.total_out;
    out[*out_len] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*out_len);
    return out;
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

char *decompress_level(char *data) {
    printf("Loading level data...\n");

    char *b64 = extract_gmd_key((const char *) data, "k4", "s");
    if (!b64) {
        // Empty level
        return data;
    }

    fix_base64_url(b64);

    unsigned char *decoded = malloc(strlen(b64));
    int decoded_len = base64_decode(b64, decoded);
    if (decoded_len <= 0) {
        printf("Failed to decode base64\n");
        free(b64);
        free(decoded);
        return NULL;
    }

    uLongf decompressed_len;
    char *decompressed = decompress_data(decoded, decoded_len, &decompressed_len);
    if (!decompressed) {
        printf("Decompression failed (check zlib error above)\n");
        free(decoded);
        free(b64);
        return NULL;
    }

    free(decoded);
    free(b64);
    
    return decompressed;
}

char **split_string(const char *str, char delimiter, int *outCount) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr == delimiter) {
            int len = ptr - start;
            if (len > 0) {
                char *token = (char *)malloc(len + 1);
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
        char *token = (char *)malloc(len + 1);
        strncpy(token, start, len);
        token[len] = '\0';
        result = (char **)realloc(result, sizeof(char*) * (count + 1));
        result[count++] = token;
    }

    *outCount = count;
    return result;
}

void free_string_array(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

bool parse_bool(const char *str) {
    return (str[0] == '1' && str[1] == '\0');
}


void parse_color_channel(GDColorChannel *channels, int i, char *channel_string) {
    GDColorChannel channel = {0};  // Zero-initialize
    int kvCount = 0;
    char **kvs = split_string(channel_string, '_', &kvCount);

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
            case 11: channel.toRed = atoi(valStr); break;
            case 12: channel.toGreen = atoi(valStr); break;
            case 13: channel.toBlue = atoi(valStr); break;
        }
    }

    channels[i] = channel;
    free_string_array(kvs, kvCount);
}

int parse_old_channels(char *level_string, GDColorChannel **outArray) {
    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * 2);
    if (!channels) {
        printf("Couldn't alloc initial pre 2.0 color channels\n");
        return 0;
    }

    char *v19_bg = get_metadata_value(level_string, "kS29");

    int i = 0;

    if (v19_bg) { // 1.9 only
        parse_color_channel(channels, i, v19_bg);
        channels[i].channelID = CHANNEL_BG;
        i++;

        parse_color_channel(channels, i, get_metadata_value(level_string, "kS30"));
        channels[i].channelID = CHANNEL_GROUND;
        i++;

        char *line = get_metadata_value(level_string, "kS31");
        if (line) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, line);
            channels[i].channelID = CHANNEL_LINE;
            i++;
        }
        
        char *obj = get_metadata_value(level_string, "kS32");
        if (obj) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, obj);
            channels[i].channelID = CHANNEL_OBJ;
            i++;
        }

        char *col1 = get_metadata_value(level_string, "kS33");
        if (col1) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col1);
            channels[i].channelID = 1;
            i++;
        }

        char *col2 = get_metadata_value(level_string, "kS34");
        if (col2) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col2);
            channels[i].channelID = 2;
            i++;
        }

        char *col3 = get_metadata_value(level_string, "kS35");
        if (col3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col3);
            channels[i].channelID = 3;
            i++;
        }

        char *col4 = get_metadata_value(level_string, "kS36");
        if (col4) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col4);
            channels[i].channelID = 4;
            i++;
        }

        char *dl3 = get_metadata_value(level_string, "kS37");
        if (dl3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, dl3);
            channels[i].channelID = CHANNEL_3DL;
            i++;
        }
        
        *outArray = channels;
        return i;
    }

    // Pre 1.9
    int bg_r = atoi(get_metadata_value(level_string, "kS1"));
    int bg_g = atoi(get_metadata_value(level_string, "kS2"));
    int bg_b = atoi(get_metadata_value(level_string, "kS3"));

    GDColorChannel bg_channel = {0};
    bg_channel.channelID = CHANNEL_BG;
    bg_channel.fromRed = bg_r;
    bg_channel.fromGreen = bg_g;
    bg_channel.fromBlue = bg_b;

    char *bg_player_color = get_metadata_value(level_string, "kS16");
    if (bg_player_color) {
       bg_channel.playerColor = atoi(bg_player_color);
    }

    channels[i] = bg_channel;

    i++;

    int g_r = atoi(get_metadata_value(level_string, "kS4"));
    int g_g = atoi(get_metadata_value(level_string, "kS5"));
    int g_b = atoi(get_metadata_value(level_string, "kS6"));

    GDColorChannel g_channel = {0};
    g_channel.channelID = CHANNEL_GROUND;
    g_channel.fromRed = g_r;
    g_channel.fromGreen = g_g;
    g_channel.fromBlue = g_b;

    char *g_player_color = get_metadata_value(level_string, "kS17");
    if (g_player_color) {
        g_channel.playerColor = atoi(g_player_color);
    }
    
    channels[i] = g_channel;
    i++;

    char *line_r = get_metadata_value(level_string, "kS7");
    char *line_g = get_metadata_value(level_string, "kS8");
    char *line_b = get_metadata_value(level_string, "kS9");

    if (line_r && line_g && line_b) {
        GDColorChannel line_channel = {0};
        line_channel.channelID = CHANNEL_LINE;
        line_channel.fromRed = atoi(line_r);
        line_channel.fromGreen = atoi(line_g);
        line_channel.fromBlue = atoi(line_b);
        
        char *line_player_color = get_metadata_value(level_string, "kS18");
        if (line_player_color) {
            line_channel.playerColor = atoi(line_player_color);
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
        obj_channel.channelID = CHANNEL_OBJ;
        obj_channel.fromRed = atoi(obj_r);
        obj_channel.fromGreen = atoi(obj_g);
        obj_channel.fromBlue = atoi(obj_b);

        char *obj_player_color = get_metadata_value(level_string, "kS19");
        if (obj_player_color) {
            obj_channel.playerColor = atoi(obj_player_color);
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
        obj_2_channel.channelID = 1;
        obj_2_channel.fromRed = atoi(obj_2_r);
        obj_2_channel.fromGreen = atoi(obj_2_g);
        obj_2_channel.fromBlue = atoi(obj_2_b);
        
        char *obj_2_player_color = get_metadata_value(level_string, "kS20");
        if (obj_2_player_color) {
            obj_2_channel.playerColor = atoi(obj_2_player_color);
        }

        char *obj_2_blending = get_metadata_value(level_string, "kA5");
        if (obj_2_blending) {
            obj_2_channel.blending = atoi(obj_2_blending) != 0;
        }

        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = obj_2_channel;
        i++;
    }

    *outArray = channels;

    return i;
}

int parse_color_channels(const char *colorString, GDColorChannel **outArray) {
    if (!colorString || !outArray) return 0;

    int count = 0;
    // Split string into each channel
    char **entries = split_string(colorString, '|', &count);
    if (!entries) return 0;

    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * count);
    if (!channels) {
        printf("Couldn't alloc color channels\n");
        free_string_array(entries, count);
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
//        case 28: return GD_VAL_INT;    // (Move trigger) Offset X
//        case 29: return GD_VAL_INT;    // (Move trigger) Offset Y
//        case 30: return GD_VAL_INT;    // (Various) Easing
//        case 31: return GD_VAL_STRING; // (Text obj) Text
//        case 32: return GD_VAL_FLOAT;  // Scale
//        case 35: return GD_VAL_FLOAT;  // (Color trigger) Opacity
//        case 41: return GD_VAL_BOOL;   // Main col HSV enabled
//        case 42: return GD_VAL_BOOL;   // Detail col HSV enabled
//        case 43: return GD_VAL_HSV;    // Main col HSV
//        case 44: return GD_VAL_HSV;    // Detail col HSV
//        case 45: return GD_VAL_FLOAT;  // (Pulse trigger) Fade in
//        case 46: return GD_VAL_FLOAT;  // (Pulse trigger) Hold
//        case 47: return GD_VAL_FLOAT;  // (Pulse trigger) Fade out
//        case 48: return GD_VAL_INT;    // (Pulse trigger) Pulse mode
//        case 49: return GD_VAL_HSV;    // (Color/Pulse trigger) Copy color HSV
//        case 50: return GD_VAL_INT;    // (Color/Pulse trigger) Copy color id
//        case 51: return GD_VAL_INT;    // (Triggers) Target group id
//        case 52: return GD_VAL_INT;    // (Pulse trigger) Pulse target type
//        case 54: return GD_VAL_FLOAT;  // (Teleport portal) Y offset
//        case 56: return GD_VAL_BOOL;   // (Toggle trigger) Activate trigger
//        case 57: return GD_VAL_INT_ARRAY; // Groups
//        case 58: return GD_VAL_BOOL;   // (Move trigger) Lock to player x
//        case 59: return GD_VAL_BOOL;   // (Move trigger) Lock to player y
//        case 62: return GD_VAL_BOOL;   // (Triggers) Spawn triggered
//        case 63: return GD_VAL_FLOAT;  // (Spawn trigger) Spawn delay
//        case 64: return GD_VAL_BOOL;   // Don't fade
//        case 65: return GD_VAL_BOOL;   // (Pulse trigger) Main only
//        case 66: return GD_VAL_BOOL;   // (Pulse trigger) Detail only
//        case 67: return GD_VAL_BOOL;   // Don't enter
//        case 87: return GD_VAL_BOOL;   // (Triggers) Multi triggered
//        case 128: return GD_VAL_FLOAT; // Scale x
//        case 129: return GD_VAL_FLOAT; // Scale y
        default:
            return GD_VAL_INT; // Default fallback
    }
}

// Convert some 2.1 objects into the 1.9 ones, blame robtop for making GD convert those to 2.1
int convert_object(int id) {
    switch (id) {
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
            if (type == GD_VAL_FLOAT) objects.x[object] = val.f;
            break;
        case 3:  // Y
            if (type == GD_VAL_FLOAT) objects.y[object] = val.f;
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
    return id >= 1 && id < 745;
}

int parse_gd_object(const char *objStr, int obj) {
    int count = 0;
    // Split object into each key
    char **tokens = split_string(objStr, ',', &count);
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
            default:
                break;
        }
    }
    
    if (is_valid_object(objects.id[obj])) {
        const GameObject *game_object = &game_objects[objects.id[obj]];

        if (game_object->swap_base_detail) {
            if (!obj_has_main(game_object)) {
                if (!objects.col_channel[obj]) objects.col_channel[obj] = game_object->base_color;
            } else {
                if (!objects.detail_col_channel[obj]) objects.detail_col_channel[obj] = game_object->base_color;
            }
        } else {
            if (!objects.col_channel[obj]) objects.col_channel[obj] = game_object->base_color;
            if (!objects.detail_col_channel[obj]) objects.detail_col_channel[obj] = 1;
        }

        objects.random[obj] = rand();

        const ObjectHitbox *hitbox = game_object->hitbox;
        if (hitbox) {
            objects.width[obj] = hitbox->width;
            objects.height[obj] = hitbox->height;
            
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
        }

        // Modify level ending pos
        if (objects.x[obj] > level_info.last_obj_x) {
            level_info.last_obj_x = objects.x[obj];
        }
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
    
    objects.activated = malloc(sizeof(u8) * count);
    if (!objects.activated) return false;
    
    objects.collided = malloc(sizeof(u8) * count);
    if (!objects.collided) return false;


    memset(objects.random,             0, sizeof(int) * count);
    memset(objects.id,                 0, sizeof(int) * count);
    memset(objects.x,                  0, sizeof(float) * count);
    memset(objects.y,                  0, sizeof(float) * count);
    memset(objects.rotation,           0, sizeof(float) * count);
    memset(objects.zlayer,             0, sizeof(int) * count);
    memset(objects.zorder,             0, sizeof(int) * count);
    memset(objects.trig_duration,      0, sizeof(float) * count);
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
    memset(objects.activated,          0, sizeof(u8) * count);
    memset(objects.collided,           0, sizeof(u8) * count);

    return true;
}

bool parse_string(const char *levelString) {
    int sectionCount = 0;

    // Split the string in object sections
    char **sections = split_string(levelString, ';', &sectionCount);

    if (sectionCount < 3) {
        printf("Level string missing sections!\n");
        free_string_array(sections, sectionCount);
        return false;
    }
    
    int objectCount = sectionCount - 1;

    printf("%d\n", objectCount);
    
    if (!init_arrays(objectCount)) {
        printf("Failed to allocate object array\n");
        return false;
    }

    objects.count = objectCount;

    printf("Parsing string and converting objects...\n");
    printf("Aproximately %d bytes of pure objects\n", sizeof(ObjectsArray) * objectCount);

    for (int i = 0; i < objectCount; i++) {
        // Parse
        if (!parse_gd_object(sections[i + 1], i)) {
            printf("Failed to parse object %d\n", i);
            free_string_array(sections, sectionCount);
            return false;
        }

        assign_object_to_section(i);
    }
    
    level_info.last_obj_x += (11 * 30.f);

    free_string_array(sections, sectionCount);

    return true;
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
                if (id < CHANNEL_P1) {
                    memset(&channels[id], 0, sizeof(ColorChannel));
                    Color color;
                    color.r = colorChannel.fromRed;
                    color.g = colorChannel.fromGreen;
                    color.b = colorChannel.fromBlue;

                    channels[id].blending = colorChannel.blending;

                    channels[id].color = color;

                    if (colorChannel.playerColor == 1) channels[id].color = p1_color;
                    if (colorChannel.playerColor == 2) channels[id].color = p2_color; 

                    if (id == CHANNEL_OBJ) {
                        channels[CHANNEL_OBJ_BLENDING].color = color;
                    }
                }
        }
    }
}

const char *default_name = "Unknown";

void load_level_info(char *data, char *level_string) {
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

    char *upsidedown_data = get_metadata_value(level_string, "kA11");
    if (upsidedown_data) {
        level_info.initial_upsidedown = atoi(upsidedown_data) != 0;
        free(upsidedown_data);
    } else {
        level_info.initial_upsidedown = 0; 
    }

    char *level_name_data = extract_gmd_key((const char *) data, "k2", "s");
    if (level_name_data) {
        level_info.level_name = level_name_data;
    } else {
        level_info.level_name = (char *) default_name;
    }
}

int load_level(char *path) {
    size_t out;
    char *level = read_file(path, &out);
    if (!level) return 1;

    char *data = decompress_level(level);
    if (!data) return 2;

    // Get level starting colors
    char *metaStr = get_metadata_value(data, "kS38");
    channelCount = parse_color_channels(metaStr, &colorChannels);

    // Fallback to pre 2.0 color keys
    if (!channelCount) {
        channelCount = parse_old_channels(data, &colorChannels);
    }

    load_level_info(level, data);

    level_info.last_obj_x = 570.f;

    bool returned = parse_string(data);

    free(data);
    free(metaStr);

    if (!returned) return 3;
    
    free(level);

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

    return 0;
}

void reload_level() {
    for (int i = 0; i < objects.count; i++) {
        objects.activated[i] = false;
        objects.collided[i] = false;
        objects.hitbox_counter[i] = 0;
        objects.transition_applied[i] = FADE_NONE;
        objects.toggled[i] = false;
    }

    init_col_channels();
    set_color_channels();
}

void unload_level() {
    free_arrays();
    free_sections();
    
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