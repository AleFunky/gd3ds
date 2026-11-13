#include <stdlib.h>

#include "arena.hpp"
#include "math_helpers.hpp"

namespace gd3ds
{

bool arenaInit(Arena *a, size_t capacity) {
    a->buffer = (uint8_t *)malloc(capacity);
    if (!a->buffer) return false;
    a->capacity = capacity;
    a->offset = 0;
    a->prevOffset = 0;
    return true;
}

void *arenaAlloc(Arena *a, size_t size) {
    size_t alignedSize = (size + 3) & ~3;
    if (a->offset + alignedSize > a->capacity) return NULL; // Out of space
    
    void *ptr = &a->buffer[a->offset];
    a->prevOffset = a->offset; 
    a->offset += alignedSize;
    return ptr;
}

void *arenaCalloc(Arena *a, size_t count, size_t size) {
    size_t alignedSize = ((count * size) + 3) & ~3;

    void *ptr = arenaAlloc(a, alignedSize);
    if (ptr == NULL) return NULL;
    
    memset(ptr, 0, alignedSize);
    return ptr;
}

void *arenaRealloc(Arena *a, void *oldPtr, size_t oldSize, size_t newSize) {
    if (oldPtr == NULL) {
        return arenaAlloc(a, newSize);
    }
    
    size_t alignedNewSize = (newSize + 3) & ~3;
    uint8_t *oldPtrBytes = (uint8_t*)oldPtr;
    if (oldPtrBytes == &a->buffer[a->prevOffset]) {
        if (a->prevOffset + alignedNewSize > a->capacity) {
            return NULL;
        }
        a->offset = a->prevOffset + alignedNewSize;
        return oldPtr;
    }

    void *newPtr = arenaAlloc(a, newSize);
    if (newPtr != NULL) {
        size_t copySize = (oldSize < newSize) ? oldSize : newSize;
        memcpy(newPtr, oldPtr, copySize);
    }

    return newPtr;
}

// Kill it, kill it all
void arenaReset(Arena *a) {
    a->offset = 0;
    a->prevOffset = 0;
}

void arenaClear(Arena *a) {
    free(a->buffer);
    a->buffer = NULL;
}

size_t arenaTotal(Arena *a) {
    return a->capacity;
}

size_t arenaUsed(Arena *a) {
    return a->offset;
}

size_t arenaAvailable(Arena *a) {
    return MAX(a->capacity - a->offset, 0);
}

}
