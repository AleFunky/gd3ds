#include <stdlib.h>

#include "arena.hpp"
#include "math_helpers.hpp"

bool arena_init(Arena *a, size_t capacity) {
    a->buffer = (uint8_t *)malloc(capacity);
    if (!a->buffer) return false;
    a->capacity = capacity;
    a->offset = 0;
    a->prev_offset = 0;
    return true;
}

void *arena_alloc(Arena *a, size_t size) {
    size_t aligned_size = (size + 3) & ~3;
    if (a->offset + aligned_size > a->capacity) return NULL; // Out of space
    
    void *ptr = &a->buffer[a->offset];
    a->prev_offset = a->offset; 
    a->offset += aligned_size;
    return ptr;
}

void *arena_calloc(Arena *a, size_t count, size_t size) {
    size_t aligned_size = ((count * size) + 3) & ~3;

    void *ptr = arena_alloc(a, aligned_size);
    if (ptr == NULL) return NULL;
    
    memset(ptr, 0, aligned_size);
    return ptr;
}

void *arena_realloc(Arena *a, void *old_ptr, size_t old_size, size_t new_size) {
    if (old_ptr == NULL) {
        return arena_alloc(a, new_size);
    }
    
    size_t aligned_new_size = (new_size + 3) & ~3;
    uint8_t *old_ptr_bytes = (uint8_t*)old_ptr;
    if (old_ptr_bytes == &a->buffer[a->prev_offset]) {
        if (a->prev_offset + aligned_new_size > a->capacity) {
            return NULL;
        }
        a->offset = a->prev_offset + aligned_new_size;
        return old_ptr;
    }

    void *new_ptr = arena_alloc(a, new_size);
    if (new_ptr != NULL) {
        size_t copy_size = (old_size < new_size) ? old_size : new_size;
        memcpy(new_ptr, old_ptr, copy_size);
    }

    return new_ptr;
}

// Kill it, kill it all
void arena_reset(Arena *a) {
    a->offset = 0;
    a->prev_offset = 0;
}

void arena_clear(Arena *a) {
    free(a->buffer);
    a->buffer = NULL;
}

size_t arena_total(Arena *a) {
    return a->capacity;
}

size_t arena_used(Arena *a) {
    return a->offset;
}

size_t arena_available(Arena *a) {
    return MAX(a->capacity - a->offset, 0);
}