#include "utils.h"
#include "level_loading.h"
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <malloc.h>

u8 get_model() {
    u8 model;
    CFGU_GetSystemModel(&model);
    return model;
}

size_t get_uncompressed_size(unsigned char *data, int data_len) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {  // auto-detect gzip/zlib
        return 0;
    }

    size_t total_out = 0;
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


unsigned char *compress_data(const unsigned char *data, size_t data_len, size_t *out_len, int *out_code) {
    z_stream strm = {0};

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return NULL;
    }

    uLong bound = deflateBound(&strm, data_len);

    unsigned char *out = malloc(bound);
    if (!out) {
        deflateEnd(&strm);
        return NULL;
    }

    strm.next_in = (Bytef *)data;
    strm.avail_in = data_len;

    strm.next_out = out;
    strm.avail_out = bound;

    int ret = deflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END) {
        free(out);
        deflateEnd(&strm);
        *out_code = LOAD_INVALID_COMPRESSED_DATA;
        return NULL;
    }

    *out_len = strm.total_out;

    deflateEnd(&strm);

    *out_code = LOAD_NO_ERROR;

    return out;
}


char *decompress_data(unsigned char *data, size_t data_len, size_t *out_len, int *out_code) {
    uLongf final_size = get_uncompressed_size(data, data_len);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        *out_code = LOAD_INVALID_COMPRESSED_DATA;
        return NULL;
    }

    // Allocate exactly enough memory
    char *out = malloc(final_size + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)final_size);
        inflateEnd(&strm);
        *out_code = LOAD_OUT_OF_MEMORY;
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = final_size;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        *out_code = LOAD_INVALID_COMPRESSED_DATA;
        return NULL;
    }

    *out_len = strm.total_out;
    out[*out_len] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*out_len);

    *out_code = LOAD_NO_ERROR;
    return out;
}