#include "groups.h"
#include "level_loading.h"
#include <stdlib.h>

static GroupNode groups[MAX_GROUPS];

void add_to_group(int obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    GroupNode *gr = &groups[g]; 
    
    // Initialize
    if (gr->objects == NULL) {
        gr->toggled = false;
        gr->alpha = 1.0f;
    }

    if (gr->count >= gr->capacity) {
        size_t new_capacity = gr->capacity == 0 ? 32 : gr->capacity + 32;

        int *new_list = realloc(gr->objects, new_capacity * sizeof(int));
        if (!new_list) {
            return;
        }

        gr->objects = new_list;
        gr->capacity = new_capacity;
    }
    gr->objects[gr->count++] = obj;
}

GroupNode *get_group(int g) {
    if (g < 1 || g >= MAX_GROUPS) return NULL;
    return &groups[g];
}

void clear_groups(void) {
    for (int g = 0; g < MAX_GROUPS; g++) {
        groups[g].toggled = false;
        groups[g].alpha = 1.0f;

        groups[g].count = 0;
        groups[g].capacity = 0;
        
        free(groups[g].objects);
        groups[g].objects = NULL;
    }
}

int compare_objects(const void *a, const void *b) {
    int a_x = objects.x[*(int *)a];
    int b_x = objects.x[*(int *)b];
    int a_y = objects.y[*(int *)a];
    int b_y = objects.y[*(int *)b];

    if (a_x != b_x) return a_x - b_x;   // smaller x first
    return b_y - a_y;                   // if x same, bigger y first
}


void sort_group(int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    
    GroupNode *gr = &groups[g];
    // Sort array
    qsort(gr->objects, gr->count, sizeof(int), compare_objects);
}
