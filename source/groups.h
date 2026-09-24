#pragma once

#define MAX_GROUPS 1000

typedef struct GroupNode {
    int obj;
    struct GroupNode *next;
} GroupNode;

void clear_groups(void);
GroupNode *get_group(int g);
void add_to_group(int obj, int g);
