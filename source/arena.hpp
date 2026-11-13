#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace gd3ds
{

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    size_t prevOffset;
} Arena;

bool arenaInit(Arena *a, size_t capacity);
void *arenaAlloc(Arena *a, size_t size);
void *arenaCalloc(Arena *a, size_t count, size_t size);
void *arenaRealloc(Arena *a, void *oldPtr, size_t oldSize, size_t newSize);
void arenaReset(Arena *a);
void arenaClear(Arena *a);

size_t arenaTotal(Arena *a);
size_t arenaUsed(Arena *a);
size_t arenaAvailable(Arena *a);

}
