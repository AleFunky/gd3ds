#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    size_t prev_offset;
} Arena;

bool arena_init(Arena *a, size_t capacity);
void *arena_alloc(Arena *a, size_t size);
void *arena_calloc(Arena *a, size_t count, size_t size);
void *arena_realloc(Arena *a, void *old_ptr, size_t old_size, size_t new_size);
void arena_reset(Arena *a);
void arena_clear(Arena *a);

size_t arena_total(Arena *a);
size_t arena_used(Arena *a);
size_t arena_available(Arena *a);