#pragma once

#define MAX_GROUPS 1000

#include <stdbool.h>

typedef struct GroupNode {
    int obj;
    float alpha;
    bool toggled;
    struct GroupNode *next;
} GroupNode;

void clear_groups(void);
GroupNode *get_group(int g);
void add_to_group(int obj, int g);

void sort_group(int g);