#pragma once

#define MAX_GROUPS 1000

#include <stdbool.h>

struct Matrix2x2;

typedef struct GroupNode {
    float alpha;
    bool toggled;
    int *objects;
    int count;
    int capacity;

    int *combinations;
    int combination_count;
    int combination_capacity;

    int collision_object_count;
} GroupNode;

bool build_group_combinations(void);
void begin_group_combination_move_step(void);
void add_move_to_group_combinations(int group, float delta_x, float delta_y);
void apply_group_combination_move(void);
void begin_group_combination_rotation_step(void);
bool add_rotation_to_group_combinations(int group, const struct Matrix2x2 *matrix, float pivot_x, float pivot_y);
void apply_group_combination_rotation(void);
void get_group_object_position(int object, float *x, float *y);
void flush_group_combination_visual_moves(void);
void discard_group_combination_visual_moves(void);

void clear_groups(void);
GroupNode *get_group(int g);
void add_to_group(int obj, int g);
void reset_group_data(int g);

void sort_group(int g);