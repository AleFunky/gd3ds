#include "groups.h"
#include "level_loading.h"
#include <stdlib.h>

static GroupNode *group_buckets[MAX_GROUPS] = {NULL};

void add_to_group(int obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    GroupNode *n = malloc(sizeof(GroupNode));
    n->obj = obj;
    n->alpha = 1.f;
    n->toggled = false;
    n->next = group_buckets[g];
    group_buckets[g] = n;
}

GroupNode *get_group(int g) {
    if (g < 1 || g >= MAX_GROUPS) return NULL;
    return group_buckets[g];
}

void clear_groups(void) {
    for (int g = 0; g < MAX_GROUPS; g++) {
        GroupNode *cur = group_buckets[g];
        while (cur) {
            GroupNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        group_buckets[g] = NULL;
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
    size_t count = 0;
    GroupNode *cur = group_buckets[g];
    while (cur) { count++; cur = cur->next; }
    if (count < 2) return;

    // Copy pointers to array
    int *arr = malloc(count * sizeof(int));
    cur = group_buckets[g];
    for (size_t i = 0; i < count; i++) {
        arr[i] = cur->obj;
        cur = cur->next;
    }

    // Sort array
    qsort(arr, count, sizeof(int), compare_objects);

    // Rebuild linked list
    cur = group_buckets[g];
    for (size_t i = 0; i < count; i++) {
        cur->obj = arr[i];
        cur = cur->next;
    }

    free(arr);
}
