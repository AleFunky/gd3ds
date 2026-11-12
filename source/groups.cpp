#include "groups.hpp"
#include "arena.hpp"
#include "graphics.hpp"
#include "level_loading.hpp"
#include "math_helpers.hpp"
#include <stdlib.h>
#include "main.hpp"
#include "triggers.hpp"

typedef struct GroupCombination {
    short groups[MAX_GROUPS_PER_OBJECT];
    unsigned char group_count;
    int *objects;

    int object_count;

    int collision_object_count;
    int collision_object_write_count;

    int visual_object_write_count;

    float offset_x;
    float offset_y;

    Matrix2x2 rotation_matrix;
    Vec2D rotation_offset;
    bool rotation_touched;
    
    Matrix2x2 deferred_matrix;
    Vec2D deferred_offset;
    Matrix2x2 deferred_no_x_matrix;
    Vec2D deferred_no_x_offset;
    
    bool touched;
    bool deferred_touched;
} GroupCombination;

static GroupNode groups[MAX_GROUPS];

static GroupCombination *group_combinations = NULL;
static int group_combination_count = 0;

static int *touched_combinations = NULL;
static int touched_combination_count = 0;

static int *rotation_combinations = NULL;
static int rotation_combination_count = 0;

static int *deferred_combinations = NULL;
static int deferred_combination_count = 0;

// Group combinations

static void clear_group_combinations(void) {

    group_combinations = NULL;
    touched_combinations = NULL;
    rotation_combinations = NULL;
    deferred_combinations = NULL;
    group_combination_count = 0;
    touched_combination_count = 0;
    rotation_combination_count = 0;
    deferred_combination_count = 0;

    for (int i = 0; i < MAX_GROUPS; i++) {
        groups[i].combinations = NULL;  
        groups[i].combination_count = 0;  
        groups[i].combination_capacity = 0;  
        groups[i].collision_object_count = 0;
    }
}

static bool object_is_240hz(int object) {
    int id = objects.id[object];

    return objects.touch_triggered[object] || is_trigger_object(id) || (is_valid_object(id) && game_objects[id].hitbox != NULL);
}

static int collect_groups(int object, short *group_ids) {
    int count = 0;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (group <= 0 || group >= MAX_GROUPS) continue;

        group_ids[count++] = group;
    }

    qsort(group_ids, count, sizeof(short), compare_ids);

    return count;
}

static bool is_same_group_combination(int count, short *group_a, short *group_b) {
    for (int i = 0; i < count; i++) {
        if (group_a[i] != group_b[i]) return false;
    }
    return true;
}

static void prepare_deferred_transform(GroupCombination *combination) {
    if (!combination->deferred_touched) {
        combination->deferred_matrix = matrix2x2_identity();
        combination->deferred_offset = (Vec2D){ 0.f, 0.f };
        combination->deferred_no_x_matrix = matrix2x2_identity();
        combination->deferred_no_x_offset = (Vec2D){ 0.f, 0.f };
        combination->deferred_touched = true;
        deferred_combinations[deferred_combination_count++] = (int)(combination - group_combinations);
    }
}

static void defer_group_combination_transform(GroupCombination *combination,
    Matrix2x2 matrix, Vec2D offset) {
    prepare_deferred_transform(combination);
    compose_affine_transform(&combination->deferred_matrix, &combination->deferred_offset, matrix, offset);
    compose_affine_transform(&combination->deferred_no_x_matrix, &combination->deferred_no_x_offset, matrix, offset);
}

static void defer_group_combination_move(GroupCombination *combination, float delta_x, float delta_y) {
    prepare_deferred_transform(combination);
    combination->deferred_offset.x += delta_x;
    combination->deferred_offset.y += delta_y;
    combination->deferred_no_x_offset.y += delta_y;
}

static Vec2D apply_deferred_transform(const Matrix2x2 *matrix, Vec2D offset, Vec2D point) {
    if (matrix->m[0][0] == 1.f && matrix->m[0][1] == 0.f &&
        matrix->m[1][0] == 0.f && matrix->m[1][1] == 1.f) {
        return (Vec2D){ point.x + offset.x, point.y + offset.y };
    }

    return affine_transform_point(*matrix, offset, point);
}

bool build_group_combinations(void) {
    clear_group_combinations();

    if (objects.count <= 0) return true;

    for (int i = 0; i < objects.count; i++)
        objects.group_combination_index[i] = -1;

    GroupCombination *new_combinations = (GroupCombination *)arena_calloc(&level_arena, objects.count, sizeof(GroupCombination));
    int *new_touched_combinations = (int *)arena_alloc(&level_arena, objects.count * sizeof(int));
    int *new_rotation_combinations = (int *)arena_alloc(&level_arena, objects.count * sizeof(int));
    int *new_deferred_combinations = (int *)arena_alloc(&level_arena, objects.count * sizeof(int));

    if (!new_combinations || !new_touched_combinations || !new_rotation_combinations || !new_deferred_combinations) {
        return false;
    }

    group_combinations = new_combinations;
    touched_combinations = new_touched_combinations;
    rotation_combinations = new_rotation_combinations;
    deferred_combinations = new_deferred_combinations;

    // Build the unique combinations
    for (int obj = 0; obj < objects.count; obj++) {
        short object_groups[MAX_GROUPS_PER_OBJECT];
        int group_count = collect_groups(obj, object_groups);

        int combination_index = -1;

        for (int i = 0; i < group_combination_count; i++) {
            GroupCombination *combination = &group_combinations[i];

            if (combination->group_count == group_count && is_same_group_combination(group_count, combination->groups, object_groups)) {
                combination_index = i;
                break;
            }
        }
        
        if (combination_index < 0) {
            // New combination
            combination_index = group_combination_count++;

            GroupCombination *combination = &group_combinations[combination_index];
            
            combination->group_count = group_count;

            // Copy groups
            for (int i = 0; i < group_count; i++) {
                combination->groups[i] = object_groups[i];
            }
        }

        objects.group_combination_index[obj] = combination_index;

        group_combinations[combination_index].object_count++;

        if (object_is_240hz(obj)) {
            group_combinations[combination_index].collision_object_count++;
        }
    }

    // Count group combinations
    for (int i = 0; i < group_combination_count; i++) {
        GroupCombination *combination = &group_combinations[i];

        combination->objects = (int *)arena_alloc(&level_arena, combination->object_count * sizeof(int));

        if (!combination->objects) {
            clear_group_combinations();
            return false;
        }

        for (int group = 0; group < combination->group_count; group++) {
            groups[combination->groups[group]].combination_count++;
        }

    }

    // Allocate combinations for each group
    for (int group = 1; group < MAX_GROUPS; group++) {
        GroupNode *node = &groups[group];

        if (node->combination_count == 0) continue;

        node->combinations = (int *)arena_alloc(&level_arena, node->combination_count * sizeof(int));

        if (!node->combinations) {
            clear_group_combinations();
            return false;
        }

        node->combination_capacity = node->combination_count;
        node->combination_count = 0;
    }
    
    // Fill object arrays
    for (int obj = 0; obj < objects.count; obj++) {
        int combination_index = objects.group_combination_index[obj];

        GroupCombination *combination = &group_combinations[combination_index];

        if (object_is_240hz(obj)) {
            combination->objects[combination->collision_object_write_count++] = obj;

            for (int i = 0; i < combination->group_count; i++)
                groups[combination->groups[i]].collision_object_count++;
        } else {
            int visual_index = combination->collision_object_count + combination->visual_object_write_count++;

            combination->objects[visual_index] = obj;
        }
    }

    // Fill combination arrays
    for (int i = 0; i < group_combination_count; i++) {
        GroupCombination *combination = &group_combinations[i];

        for (int group = 0; group < combination->group_count; group++) {
            GroupNode *g = &groups[combination->groups[group]];
            g->combinations[g->combination_count++] = i;
        }
    }

    // FINALLY DONE
    return true;
}

void begin_group_combination_move_step(void) {
    for (int i = 0; i < touched_combination_count; i++) {
        GroupCombination *combination = &group_combinations[touched_combinations[i]];
        combination->offset_x = 0.f;
        combination->offset_y = 0.f;
        combination->touched = false;
    }
    touched_combination_count = 0;
}

void begin_group_combination_rotation_step(void) {
    for (int i = 0; i < rotation_combination_count; i++) {
        GroupCombination *combination = &group_combinations[rotation_combinations[i]];
        combination->rotation_matrix = matrix2x2_identity();
        combination->rotation_offset = (Vec2D){ 0.f, 0.f };
        combination->rotation_touched = false;
    }
    rotation_combination_count = 0;
}

void add_move_to_group_combinations(int group, float delta_x, float delta_y) {
    GroupNode *node = get_group(group);
    if (!node) return;

    if (node->collision_object_count == 0) {
        if (delta_x != 0.f || delta_y != 0.f) {
            for (int i = 0; i < node->combination_count; i++) {
                GroupCombination *combination = &group_combinations[node->combinations[i]];
                defer_group_combination_move(combination, delta_x, delta_y);
            }
        }
        return;
    }

    for (int i = 0; i < node->combination_count; i++) {
        int combination_index = node->combinations[i];
        GroupCombination *combination = &group_combinations[combination_index];
        if (!combination->touched) {
            combination->touched = true;
            touched_combinations[touched_combination_count++] = combination_index;
        }
        combination->offset_x += delta_x;
        combination->offset_y += delta_y;
    }
}

bool add_rotation_to_group_combinations(int group, const Matrix2x2 *matrix, float pivot_x, float pivot_y) {
    GroupNode *node = get_group(group);
    if (!node) return false;

    Vec2D pivot = { pivot_x, pivot_y };
    Vec2D offset = sub_vec(pivot, matrix2x2_transform(*matrix, pivot));
    bool has_transform = matrix->m[0][0] != 1.f || matrix->m[0][1] != 0.f || matrix->m[1][0] != 0.f || matrix->m[1][1] != 1.f || offset.x != 0.f || offset.y != 0.f;

    for (int i = 0; i < node->combination_count; i++) {
        int combination_index = node->combinations[i];
        GroupCombination *combination = &group_combinations[combination_index];
        if (has_transform && combination->collision_object_count > 0) {
            if (!combination->rotation_touched) {
                combination->rotation_matrix = matrix2x2_identity();
                combination->rotation_offset = (Vec2D){ 0.f, 0.f };
                combination->rotation_touched = true;
                rotation_combinations[rotation_combination_count++] = combination_index;
            }
            compose_affine_transform(&combination->rotation_matrix, &combination->rotation_offset, *matrix, offset);
        }

        if (has_transform && combination->object_count > combination->collision_object_count) {
            defer_group_combination_transform(combination, *matrix, offset);
        }
    }
    return true;
}

void apply_group_combination_move(void) {
    for (int i = 0; i < touched_combination_count; i++) {
        int combination_index = touched_combinations[i];

        GroupCombination *combination = &group_combinations[combination_index];

        float delta_x = combination->offset_x;
        float delta_y = combination->offset_y;
        bool has_position_delta = delta_x != 0.f || delta_y != 0.f;

        for (int j = 0; j < combination->collision_object_count; j++) {
            int object = combination->objects[j];
            objects.last_y[object] = objects.y[object];

            if (has_position_delta) {
                int old_sx = objects.section_x[object];
                int old_sy = objects.section_y[object];

                if (delta_x != 0.f && (objects.flags[object] & FLAG_CAN_BE_X_MOVED)) objects.x[object] += delta_x;
                if (delta_y != 0.f) objects.y[object] += delta_y;

                if ((int)(objects.x[object] / SECTION_SIZE) != old_sx || (int)(objects.y[object] / SECTION_SIZE) != old_sy)
                    update_object_section(object);
            }
        }

        if (has_position_delta && combination->object_count > combination->collision_object_count) {
            defer_group_combination_move(combination, delta_x, delta_y);
        }
    }
}

void apply_group_combination_rotation(void) {
    for (int i = 0; i < rotation_combination_count; i++) {
        int combination_index = rotation_combinations[i];
        GroupCombination *combination = &group_combinations[combination_index];

        for (int j = 0; j < combination->collision_object_count; j++) {
            int object = combination->objects[j];
            objects.last_y[object] = objects.y[object];

            float x = objects.x[object];
            float y = objects.y[object];

            Vec2D transformed = affine_transform_point(combination->rotation_matrix, combination->rotation_offset, (Vec2D){ x, y });
            objects.x[object] = transformed.x;
            objects.y[object] = transformed.y;

            if ((int)(objects.x[object] / SECTION_SIZE) != objects.section_x[object] || (int)(objects.y[object] / SECTION_SIZE) != objects.section_y[object])
                update_object_section(object);
        }

        combination->rotation_matrix = matrix2x2_identity();
        combination->rotation_offset = (Vec2D){ 0.f, 0.f };
        combination->rotation_touched = false;
    }
    rotation_combination_count = 0;
}

void get_group_object_position(int object, float *x, float *y) {
    if (object < 0 || object >= objects.count) return;
    *x = objects.x[object];
    *y = objects.y[object];

    int combination_index = objects.group_combination_index[object];
    if (combination_index < 0 || combination_index >= group_combination_count) return;

    GroupCombination *combination = &group_combinations[combination_index];
    if (object_is_240hz(object)) {
        if (!combination->rotation_touched) return;

        float current_x = *x;
        float current_y = *y;
        Vec2D transformed = affine_transform_point(combination->rotation_matrix, combination->rotation_offset, (Vec2D){ current_x, current_y });
        *x = transformed.x;
        *y = transformed.y;
        return;
    }

    if (!combination->deferred_touched) return;

    float current_x = *x;
    float current_y = *y;

    bool can_move_x = (objects.flags[object] & FLAG_CAN_BE_X_MOVED) != 0;

    Matrix2x2 matrix = can_move_x ? combination->deferred_matrix : combination->deferred_no_x_matrix;
    Vec2D offset = can_move_x ? combination->deferred_offset : combination->deferred_no_x_offset;
    Vec2D transformed = apply_deferred_transform(&matrix, offset, (Vec2D){ current_x, current_y });
    *x = transformed.x;
    *y = transformed.y;
}

void flush_group_combination_visual_moves(void) {
    for (int i = 0; i < deferred_combination_count; i++) {
        int combination_index = deferred_combinations[i];
        GroupCombination *combination = &group_combinations[combination_index];
        for (int j = combination->collision_object_count; j < combination->object_count; j++) {
            int object = combination->objects[j];

            int old_sx = objects.section_x[object];
            int old_sy = objects.section_y[object];

            float x = objects.x[object];
            float y = objects.y[object];

            bool can_move_x = (objects.flags[object] & FLAG_CAN_BE_X_MOVED) != 0;

            Matrix2x2 matrix = can_move_x ? combination->deferred_matrix : combination->deferred_no_x_matrix;
            Vec2D offset = can_move_x ? combination->deferred_offset : combination->deferred_no_x_offset;
            Vec2D transformed = apply_deferred_transform(&matrix, offset, (Vec2D){ x, y });
            objects.x[object] = transformed.x;
            objects.y[object] = transformed.y;

            if ((int)(objects.x[object] / SECTION_SIZE) != old_sx || (int)(objects.y[object] / SECTION_SIZE) != old_sy)
                update_object_section(object);
        }
        combination->deferred_offset = (Vec2D){ 0.f, 0.f };
        combination->deferred_matrix = matrix2x2_identity();
        combination->deferred_touched = false;
        combination->deferred_no_x_matrix = matrix2x2_identity();
        combination->deferred_no_x_offset = (Vec2D){ 0.f, 0.f };
    }
    deferred_combination_count = 0;
}

void discard_group_combination_visual_moves(void) {
    for (int i = 0; i < deferred_combination_count; i++) {
        GroupCombination *combination = &group_combinations[deferred_combinations[i]];
        combination->deferred_offset = (Vec2D){ 0.f, 0.f };
        combination->deferred_matrix = matrix2x2_identity();
        combination->deferred_touched = false;
        combination->deferred_no_x_matrix = matrix2x2_identity();
        combination->deferred_no_x_offset = (Vec2D){ 0.f, 0.f };
    }
    deferred_combination_count = 0;
}

// Groups

void reset_group_data(int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    
    GroupNode *gr = &groups[g]; 

    gr->toggled = false;
    gr->alpha = 1.0f;
}

void add_to_group(int obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    GroupNode *gr = &groups[g]; 
    
    // Initialize
    if (gr->objects == NULL) {
        reset_group_data(g);
    }

    if (gr->count >= gr->capacity) {
        size_t new_capacity = gr->capacity == 0 ? 32 : gr->capacity + 32;

        int *new_list = (int *)realloc(gr->objects, new_capacity * sizeof(int));
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
