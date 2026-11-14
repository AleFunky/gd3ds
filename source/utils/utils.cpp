#include "utils.hpp"
#include "level_loading.hpp"
#include "main.hpp"
#include "arena.hpp"
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <minizip/unzip.h>

namespace gd3ds
{

u8 getModel() {
    u8 model = 0;
    if (R_FAILED(CFGU_GetSystemModel(&model))) return CFG_MODEL_3DS;
    return model;
}

size_t getUncompressedSize(unsigned char *data, int dataLen) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = data;
    strm.avail_in = dataLen;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {  // auto-detect gzip/zlib
        return 0;
    }

    size_t totalOut = 0;
    unsigned char buf[4096];

    do {
        strm.next_out = buf;
        strm.avail_out = sizeof(buf);
        int ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return 0;
        }
        totalOut += sizeof(buf) - strm.avail_out;
        if (ret == Z_STREAM_END) break;
    } while (strm.avail_in > 0);

    inflateEnd(&strm);
    return totalOut;
}


unsigned char *compressData(const unsigned char *data, size_t dataLen, size_t *outLen, int *outCode) {
    z_stream strm = {0};

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return NULL;
    }

    uLong bound = deflateBound(&strm, dataLen);

    unsigned char *out = (unsigned char *)malloc(bound);
    if (!out) {
        deflateEnd(&strm);
        return NULL;
    }

    strm.next_in = (Bytef *)data;
    strm.avail_in = dataLen;

    strm.next_out = out;
    strm.avail_out = bound;

    int ret = deflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END) {
        free(out);
        deflateEnd(&strm);
        *outCode = static_cast<int>(LevelLoadError::LoadInvalidCompressedData);
        return NULL;
    }

    *outLen = strm.total_out;

    deflateEnd(&strm);

    *outCode = static_cast<int>(LevelLoadError::LoadNoError);

    return out;
}


char *decompressData(unsigned char *data, size_t dataLen, size_t *outLen, int *outCode) {
    uLongf finalSize = getUncompressedSize(data, dataLen);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = dataLen;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        *outCode = static_cast<int>(LevelLoadError::LoadInvalidCompressedData);
        return NULL;
    }

    // Allocate exactly enough memory
    char *out = (char *)malloc(finalSize + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)finalSize);
        inflateEnd(&strm);
        *outCode = static_cast<int>(LevelLoadError::LoadOutOfMemory);
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = finalSize;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        *outCode = static_cast<int>(LevelLoadError::LoadInvalidCompressedData);
        return NULL;
    }

    *outLen = strm.total_out;
    out[*outLen] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*outLen);

    *outCode = static_cast<int>(LevelLoadError::LoadNoError);
    return out;
}

char *decompressDataLevel(unsigned char *data, size_t dataLen, size_t *outLen, int *outCode) {
    uLongf finalSize = getUncompressedSize(data, dataLen);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = dataLen;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        *outCode = static_cast<int>(LevelLoadError::LoadInvalidCompressedData);
        return NULL;
    }

    // Allocate exactly enough memory
    char *out = (char *)arenaAlloc(&levelArena, finalSize + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)finalSize);
        inflateEnd(&strm);
        *outCode = static_cast<int>(LevelLoadError::LoadOutOfMemory);
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = finalSize;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        *outCode = static_cast<int>(LevelLoadError::LoadInvalidCompressedData);
        return NULL;
    }

    *outLen = strm.total_out;
    out[*outLen] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*outLen);

    *outCode = static_cast<int>(LevelLoadError::LoadNoError);
    return out;
}

static void genericThread(void *arg) {
    GenericTask *task = (GenericTask *)arg;

    task->result = task->func(task);

    task->running = false;
    task->finished = true;
}

Thread createGenericThread(GenericTask *task) {
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority += 1;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    task->finished = false;
    task->running = true;
    task->cancelled = false;
    
    return threadCreate(
        genericThread,
        task,
        32 * 1024,
        priority,
        (isN3Ds ? 2 : 0),
        true
   );
}

// adapted from https://stackoverflow.com/questions/10440113/simple-way-to-unzip-a-zip-file-using-zlib

// extracts zip file at input_path to folder destination_path
int extractZip(const char *inputPath, const char *destinationDir) {
    // open file at input path
    unzFile *zipFile = (unzFile *)unzOpen(inputPath);
    if (!zipFile) {
        outputLog("failed to open zip file\n");
        return 1;
    }
    // get zip file metadata
    unz_global_info globalInfo;
    if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK){
        outputLog("failed to get zip file metadata\n");
        return 1;
    }

    // buffer to hold data read from the zip file.
    char readBuffer[8192];

    for (uLong i = 0; i < globalInfo.number_entry; i++) {
        // get current file metadata
        unz_file_info fileInfo;
        char filename[256];
        char path[256 + sizeof(filename)];

        if (unzGetCurrentFileInfo(zipFile, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK) {
          outputLog("could not read file metadata\n");
          unzClose(zipFile);
          return 1;
        }

        const size_t filenameLength = strlen(filename);

        snprintf(path, sizeof(path), "%s%s", destinationDir, filename);

        if (filename[ filenameLength-1 ] == '/') {
            // entry is a directory, so create it
            mkdir(path, 0777);
        } else {
            // entry is a file, so extract it
            if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
                outputLog("could not open file\n");
                unzClose(zipFile);
                return 1;
            }

            // open a file to write out the data
            FILE *out = fopen(path, "wb");
            if (!out) {
                outputLog("could not open destination file\n");
                unzCloseCurrentFile(zipFile);
                unzClose(zipFile);
                return 1;
            }

            int error = UNZ_OK;
            do {
                error = unzReadCurrentFile(zipFile, readBuffer, sizeof(readBuffer));
                if (error < 0) {
                    outputLog("error %d\n", error);
                    unzCloseCurrentFile(zipFile);
                    unzClose(zipFile);
                    return 1;
                }

                // write data to file
                if (error > 0) {
                    fwrite(readBuffer, sizeof(readBuffer), 1, out);
                }
            } while (error > 0);

            fclose(out);

        }
        unzCloseCurrentFile(zipFile);

        // go the the next entry listed in the zip file
        if ((i+1) < globalInfo.number_entry) {
            if (unzGoToNextFile(zipFile) != UNZ_OK)
            {
                outputLog("could not read next file\n");
                unzClose(zipFile);
                return 1;
            }
        }
    }

    unzClose(zipFile);
    return 0;
};

}
