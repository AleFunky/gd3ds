#ifdef DEBUG_LEAKS

#include <stdlib.h>
#include <stdio.h>

extern int current_generation;

void *__real_malloc(size_t);
void __real_free(void *);
void *__real_realloc(void *, size_t);

void dump_leaks();

#endif