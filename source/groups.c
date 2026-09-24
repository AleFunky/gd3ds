#include "groups.h"
#include <stdlib.h>

static GroupNode *group_buckets[MAX_GROUPS] = {NULL};

void add_to_group(int obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    GroupNode *n = malloc(sizeof(GroupNode));
    n->obj = obj;
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
