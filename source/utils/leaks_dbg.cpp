#ifdef DEBUG_LEAKS

#include "main.hpp"
#include "leaks_dbg.hpp"

typedef struct Allocation {
    void *ptr;
    size_t size;
    void *caller;
    int generation;

    struct Allocation *next;
} Allocation;

int current_generation = 0;

static Allocation *head = NULL;

static int tracking = 0;

void track_alloc(void *ptr, size_t size, void *caller) {
    if (!ptr)
        return;

    Allocation *a = __real_malloc(sizeof(*a));

    if (!a)
        return;

    a->ptr = ptr;
    a->size = size;
    a->caller = caller;
    a->generation = current_generation;

    a->next = head;
    head = a;
}

void track_free(void *ptr) {
    Allocation **current = &head;

    while (*current) {
        if ((*current)->ptr == ptr) {
            Allocation *dead = *current;

            *current = dead->next;

            __real_free(dead);
            return;
        }

        current = &(*current)->next;
    }
}

void dump_leaks() {
    Allocation *a = head;

    if (!a) {
        outputLog("No leaks!\n");
        return;
    }

    outputLog("Leaks:\n");

    while (a) {
        outputLog("Gen %d :LEAK: %p (%u bytes) caller=%p\n",
            a->generation,
            a->ptr,
            (unsigned)a->size,
            a->caller);

        a = a->next;
    }
}

void *__wrap_malloc(size_t size) {
    void *ptr = __real_malloc(size);
    if (!tracking) {
        tracking++;
        void *caller = __builtin_return_address(0);
        track_alloc(ptr, size, caller);
        tracking--;
    }

    return ptr;
}

void __wrap_free(void *ptr) {
    if (!tracking) {
        tracking++;
        track_free(ptr);
        tracking--;
    }

    __real_free(ptr);
}

void *__wrap_realloc(void *ptr, size_t size) {
    void *caller = __builtin_return_address(0);
    void *newPtr = __real_realloc(ptr, size);

    if (!newPtr)
        return NULL;

    if (!ptr) {
        track_alloc(newPtr, size, caller);
        return newPtr;
    }

    if (size == 0) {
        track_free(ptr);
        return NULL;
    }

    track_free(ptr);
    track_alloc(newPtr, size, caller);

    return newPtr;
}

#endif