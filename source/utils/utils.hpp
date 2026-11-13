#pragma once
#include <3ds.h>

#define ZLIB_CHUNK_SIZE 4096

namespace gd3ds
{

u8 getModel();

size_t getUncompressedSize(unsigned char *data, int dataLen);
unsigned char *compressData(const unsigned char *data, size_t dataLen, size_t *outLen, int *outCode);
char *decompressData(unsigned char *data, size_t dataLen, size_t *outLen, int *outCode);

char *decompressDataLevel(unsigned char *data, size_t dataLen, size_t *outLen, int *outCode);


typedef struct GenericTask GenericTask;
typedef int (*GenericFunc)(GenericTask *);

typedef struct GenericTask {
    GenericFunc func;

    volatile int result;
    
    volatile bool running;
    volatile bool finished;
    volatile bool cancelled;
} GenericTask;

Thread createGenericThread(GenericTask *task);

int extractZip(const char *inputPath, const char *destinationPath);

}
