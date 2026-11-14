#include "groups.hpp"
#include "arena.hpp"
#include "graphics.hpp"
#include "level_loading.hpp"
#include "math_helpers.hpp"
#include <stdlib.h>
#include "main.hpp"
#include "triggers.hpp"

namespace gd3ds
{

struct GroupCombination {
    short groups[MAX_GROUPS_PER_OBJECT];
    unsigned char groupCount;
    int *objects;

    int objectCount;

    int collisionObjectCount;
    int collisionObjectWriteCount;

    int visualObjectWriteCount;

    float offsetX;
    float offsetY;

    Matrix2x2 rotationMatrix;
    Vec2D rotationOffset;
    bool rotationTouched;
    
    Matrix2x2 deferredMatrix;
    Vec2D deferredOffset;
    Matrix2x2 deferredNoXMatrix;
    Vec2D deferredNoXOffset;
    
    bool touched;
    bool deferredTouched;
};

static GroupNode groups[MAX_GROUPS];

static GroupCombination *group_combinations = NULL;
static int groupCombinationCount = 0;

static int *touched_combinations = NULL;
static int touchedCombinationCount = 0;

static int *rotation_combinations = NULL;
static int rotationCombinationCount = 0;

static int *deferred_combinations = NULL;
static int deferredCombinationCount = 0;

// Group combinations

static void clearGroupCombinations(void) {

    group_combinations = NULL;
    touched_combinations = NULL;
    rotation_combinations = NULL;
    deferred_combinations = NULL;
    groupCombinationCount = 0;
    touchedCombinationCount = 0;
    rotationCombinationCount = 0;
    deferredCombinationCount = 0;

    for (int i = 0; i < MAX_GROUPS; i++) {
        groups[i].combinations = NULL;  
        groups[i].combinationCount = 0;  
        groups[i].combinationCapacity = 0;  
        groups[i].collisionObjectCount = 0;
    }
}

static bool objectIs240hz(int object) {
    int id = objects.id[object];

    return objects.touchTriggered[object] || isTriggerObject(id) || (isValidObject(id) && gameObjects[id].hitbox != NULL);
}

static int collectGroups(int object, short *groupIds) {
    int count = 0;
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        int group = objects.groups[object][i];
        if (group <= 0 || group >= MAX_GROUPS) continue;

        groupIds[count++] = group;
    }

    qsort(groupIds, count, sizeof(short), compareIds);

    return count;
}

static bool isSameGroupCombination(int count, short *groupA, short *groupB) {
    for (int i = 0; i < count; i++) {
        if (groupA[i] != groupB[i]) return false;
    }
    return true;
}

static void prepareDeferredTransform(GroupCombination *combination) {
    if (!combination->deferredTouched) {
        combination->deferredMatrix = matrix2x2Identity();
        combination->deferredOffset = (Vec2D){ 0.f, 0.f };
        combination->deferredNoXMatrix = matrix2x2Identity();
        combination->deferredNoXOffset = (Vec2D){ 0.f, 0.f };
        combination->deferredTouched = true;
        deferred_combinations[deferredCombinationCount++] = (int)(combination - group_combinations);
    }
}

static void deferGroupCombinationTransform(GroupCombination *combination,
    Matrix2x2 matrix, Vec2D offset) {
    prepareDeferredTransform(combination);
    composeAffineTransform(&combination->deferredMatrix, &combination->deferredOffset, matrix, offset);
    composeAffineTransform(&combination->deferredNoXMatrix, &combination->deferredNoXOffset, matrix, offset);
}

static void deferGroupCombinationMove(GroupCombination *combination, float deltaX, float deltaY) {
    prepareDeferredTransform(combination);
    combination->deferredOffset.x += deltaX;
    combination->deferredOffset.y += deltaY;
    combination->deferredNoXOffset.y += deltaY;
}

static Vec2D applyDeferredTransform(const Matrix2x2 *matrix, Vec2D offset, Vec2D point) {
    if (matrix->m[0][0] == 1.f && matrix->m[0][1] == 0.f &&
        matrix->m[1][0] == 0.f && matrix->m[1][1] == 1.f) {
        return (Vec2D){ point.x + offset.x, point.y + offset.y };
    }

    return affineTransformPoint(*matrix, offset, point);
}

bool buildGroupCombinations(void) {
    clearGroupCombinations();

    if (objects.count <= 0) return true;

    for (int i = 0; i < objects.count; i++)
        objects.groupCombinationIndex[i] = -1;

    GroupCombination *newCombinations = (GroupCombination *)arenaCalloc(&levelArena, objects.count, sizeof(GroupCombination));
    int *newTouchedCombinations = (int *)arenaAlloc(&levelArena, objects.count * sizeof(int));
    int *newRotationCombinations = (int *)arenaAlloc(&levelArena, objects.count * sizeof(int));
    int *newDeferredCombinations = (int *)arenaAlloc(&levelArena, objects.count * sizeof(int));

    if (!newCombinations || !newTouchedCombinations || !newRotationCombinations || !newDeferredCombinations) {
        return false;
    }

    group_combinations = newCombinations;
    touched_combinations = newTouchedCombinations;
    rotation_combinations = newRotationCombinations;
    deferred_combinations = newDeferredCombinations;

    // Build the unique combinations
    for (int obj = 0; obj < objects.count; obj++) {
        short objectGroups[MAX_GROUPS_PER_OBJECT];
        int groupCount = collectGroups(obj, objectGroups);

        int combinationIndex = -1;

        for (int i = 0; i < groupCombinationCount; i++) {
            GroupCombination *combination = &group_combinations[i];

            if (combination->groupCount == groupCount && isSameGroupCombination(groupCount, combination->groups, objectGroups)) {
                combinationIndex = i;
                break;
            }
        }
        
        if (combinationIndex < 0) {
            // New combination
            combinationIndex = groupCombinationCount++;

            GroupCombination *combination = &group_combinations[combinationIndex];
            
            combination->groupCount = groupCount;

            // Copy groups
            for (int i = 0; i < groupCount; i++) {
                combination->groups[i] = objectGroups[i];
            }
        }

        objects.groupCombinationIndex[obj] = combinationIndex;

        group_combinations[combinationIndex].objectCount++;

        if (objectIs240hz(obj)) {
            group_combinations[combinationIndex].collisionObjectCount++;
        }
    }

    // Count group combinations
    for (int i = 0; i < groupCombinationCount; i++) {
        GroupCombination *combination = &group_combinations[i];

        combination->objects = (int *)arenaAlloc(&levelArena, combination->objectCount * sizeof(int));

        if (!combination->objects) {
            clearGroupCombinations();
            return false;
        }

        for (int group = 0; group < combination->groupCount; group++) {
            groups[combination->groups[group]].combinationCount++;
        }

    }

    // Allocate combinations for each group
    for (int group = 1; group < MAX_GROUPS; group++) {
        GroupNode *node = &groups[group];

        if (node->combinationCount == 0) continue;

        node->combinations = (int *)arenaAlloc(&levelArena, node->combinationCount * sizeof(int));

        if (!node->combinations) {
            clearGroupCombinations();
            return false;
        }

        node->combinationCapacity = node->combinationCount;
        node->combinationCount = 0;
    }
    
    // Fill object arrays
    for (int obj = 0; obj < objects.count; obj++) {
        int combinationIndex = objects.groupCombinationIndex[obj];

        GroupCombination *combination = &group_combinations[combinationIndex];

        if (objectIs240hz(obj)) {
            combination->objects[combination->collisionObjectWriteCount++] = obj;

            for (int i = 0; i < combination->groupCount; i++)
                groups[combination->groups[i]].collisionObjectCount++;
        } else {
            int visualIndex = combination->collisionObjectCount + combination->visualObjectWriteCount++;

            combination->objects[visualIndex] = obj;
        }
    }

    // Fill combination arrays
    for (int i = 0; i < groupCombinationCount; i++) {
        GroupCombination *combination = &group_combinations[i];

        for (int group = 0; group < combination->groupCount; group++) {
            GroupNode *g = &groups[combination->groups[group]];
            g->combinations[g->combinationCount++] = i;
        }
    }

    // FINALLY DONE
    return true;
}

void beginGroupCombinationMoveStep(void) {
    for (int i = 0; i < touchedCombinationCount; i++) {
        GroupCombination *combination = &group_combinations[touched_combinations[i]];
        combination->offsetX = 0.f;
        combination->offsetY = 0.f;
        combination->touched = false;
    }
    touchedCombinationCount = 0;
}

void beginGroupCombinationRotationStep(void) {
    for (int i = 0; i < rotationCombinationCount; i++) {
        GroupCombination *combination = &group_combinations[rotation_combinations[i]];
        combination->rotationMatrix = matrix2x2Identity();
        combination->rotationOffset = (Vec2D){ 0.f, 0.f };
        combination->rotationTouched = false;
    }
    rotationCombinationCount = 0;
}

void addMoveToGroupCombinations(int group, float deltaX, float deltaY) {
    GroupNode *node = getGroup(group);
    if (!node) return;

    if (node->collisionObjectCount == 0) {
        if (deltaX != 0.f || deltaY != 0.f) {
            for (int i = 0; i < node->combinationCount; i++) {
                GroupCombination *combination = &group_combinations[node->combinations[i]];
                deferGroupCombinationMove(combination, deltaX, deltaY);
            }
        }
        return;
    }

    for (int i = 0; i < node->combinationCount; i++) {
        int combinationIndex = node->combinations[i];
        GroupCombination *combination = &group_combinations[combinationIndex];
        if (!combination->touched) {
            combination->touched = true;
            touched_combinations[touchedCombinationCount++] = combinationIndex;
        }
        combination->offsetX += deltaX;
        combination->offsetY += deltaY;
    }
}

bool addRotationToGroupCombinations(int group, const Matrix2x2 *matrix, float pivotX, float pivotY) {
    GroupNode *node = getGroup(group);
    if (!node) return false;

    Vec2D pivot = { pivotX, pivotY };
    Vec2D offset = subVec(pivot, matrix2x2Transform(*matrix, pivot));
    bool hasTransform = matrix->m[0][0] != 1.f || matrix->m[0][1] != 0.f || matrix->m[1][0] != 0.f || matrix->m[1][1] != 1.f || offset.x != 0.f || offset.y != 0.f;

    for (int i = 0; i < node->combinationCount; i++) {
        int combinationIndex = node->combinations[i];
        GroupCombination *combination = &group_combinations[combinationIndex];
        if (hasTransform && combination->collisionObjectCount > 0) {
            if (!combination->rotationTouched) {
                combination->rotationMatrix = matrix2x2Identity();
                combination->rotationOffset = (Vec2D){ 0.f, 0.f };
                combination->rotationTouched = true;
                rotation_combinations[rotationCombinationCount++] = combinationIndex;
            }
            composeAffineTransform(&combination->rotationMatrix, &combination->rotationOffset, *matrix, offset);
        }

        if (hasTransform && combination->objectCount > combination->collisionObjectCount) {
            deferGroupCombinationTransform(combination, *matrix, offset);
        }
    }
    return true;
}

void applyGroupCombinationMove(void) {
    for (int i = 0; i < touchedCombinationCount; i++) {
        int combinationIndex = touched_combinations[i];

        GroupCombination *combination = &group_combinations[combinationIndex];

        float deltaX = combination->offsetX;
        float deltaY = combination->offsetY;
        bool hasPositionDelta = deltaX != 0.f || deltaY != 0.f;

        for (int j = 0; j < combination->collisionObjectCount; j++) {
            int object = combination->objects[j];
            objects.lastY[object] = objects.y[object];

            if (hasPositionDelta) {
                int oldSx = objects.sectionX[object];
                int oldSy = objects.sectionY[object];

                if (deltaX != 0.f && (objects.flags[object] & static_cast<int>(ObjectFlags::FlagCanBeXMoved))) objects.x[object] += deltaX;
                if (deltaY != 0.f) objects.y[object] += deltaY;

                if ((int)(objects.x[object] / SECTION_SIZE) != oldSx || (int)(objects.y[object] / SECTION_SIZE) != oldSy)
                    updateObjectSection(object);
            }
        }

        if (hasPositionDelta && combination->objectCount > combination->collisionObjectCount) {
            deferGroupCombinationMove(combination, deltaX, deltaY);
        }
    }
}

void applyGroupCombinationRotation(void) {
    for (int i = 0; i < rotationCombinationCount; i++) {
        int combinationIndex = rotation_combinations[i];
        GroupCombination *combination = &group_combinations[combinationIndex];

        for (int j = 0; j < combination->collisionObjectCount; j++) {
            int object = combination->objects[j];
            objects.lastY[object] = objects.y[object];

            float x = objects.x[object];
            float y = objects.y[object];

            Vec2D transformed = affineTransformPoint(combination->rotationMatrix, combination->rotationOffset, (Vec2D){ x, y });
            objects.x[object] = transformed.x;
            objects.y[object] = transformed.y;

            if ((int)(objects.x[object] / SECTION_SIZE) != objects.sectionX[object] || (int)(objects.y[object] / SECTION_SIZE) != objects.sectionY[object])
                updateObjectSection(object);
        }

        combination->rotationMatrix = matrix2x2Identity();
        combination->rotationOffset = (Vec2D){ 0.f, 0.f };
        combination->rotationTouched = false;
    }
    rotationCombinationCount = 0;
}

void getGroupObjectPosition(int object, float *x, float *y) {
    if (object < 0 || object >= objects.count) return;
    *x = objects.x[object];
    *y = objects.y[object];

    int combinationIndex = objects.groupCombinationIndex[object];
    if (combinationIndex < 0 || combinationIndex >= groupCombinationCount) return;

    GroupCombination *combination = &group_combinations[combinationIndex];
    if (objectIs240hz(object)) {
        if (!combination->rotationTouched) return;

        float currentX = *x;
        float currentY = *y;
        Vec2D transformed = affineTransformPoint(combination->rotationMatrix, combination->rotationOffset, (Vec2D){ currentX, currentY });
        *x = transformed.x;
        *y = transformed.y;
        return;
    }

    if (!combination->deferredTouched) return;

    float currentX = *x;
    float currentY = *y;

    bool canMoveX = (objects.flags[object] & static_cast<int>(ObjectFlags::FlagCanBeXMoved)) != 0;

    Matrix2x2 matrix = canMoveX ? combination->deferredMatrix : combination->deferredNoXMatrix;
    Vec2D offset = canMoveX ? combination->deferredOffset : combination->deferredNoXOffset;
    Vec2D transformed = applyDeferredTransform(&matrix, offset, (Vec2D){ currentX, currentY });
    *x = transformed.x;
    *y = transformed.y;
}

void flushGroupCombinationVisualMoves(void) {
    for (int i = 0; i < deferredCombinationCount; i++) {
        int combinationIndex = deferred_combinations[i];
        GroupCombination *combination = &group_combinations[combinationIndex];
        for (int j = combination->collisionObjectCount; j < combination->objectCount; j++) {
            int object = combination->objects[j];

            int oldSx = objects.sectionX[object];
            int oldSy = objects.sectionY[object];

            float x = objects.x[object];
            float y = objects.y[object];

            bool canMoveX = (objects.flags[object] & static_cast<int>(ObjectFlags::FlagCanBeXMoved)) != 0;

            Matrix2x2 matrix = canMoveX ? combination->deferredMatrix : combination->deferredNoXMatrix;
            Vec2D offset = canMoveX ? combination->deferredOffset : combination->deferredNoXOffset;
            Vec2D transformed = applyDeferredTransform(&matrix, offset, (Vec2D){ x, y });
            objects.x[object] = transformed.x;
            objects.y[object] = transformed.y;

            if ((int)(objects.x[object] / SECTION_SIZE) != oldSx || (int)(objects.y[object] / SECTION_SIZE) != oldSy)
                updateObjectSection(object);
        }
        combination->deferredOffset = (Vec2D){ 0.f, 0.f };
        combination->deferredMatrix = matrix2x2Identity();
        combination->deferredTouched = false;
        combination->deferredNoXMatrix = matrix2x2Identity();
        combination->deferredNoXOffset = (Vec2D){ 0.f, 0.f };
    }
    deferredCombinationCount = 0;
}

void discardGroupCombinationVisualMoves(void) {
    for (int i = 0; i < deferredCombinationCount; i++) {
        GroupCombination *combination = &group_combinations[deferred_combinations[i]];
        combination->deferredOffset = (Vec2D){ 0.f, 0.f };
        combination->deferredMatrix = matrix2x2Identity();
        combination->deferredTouched = false;
        combination->deferredNoXMatrix = matrix2x2Identity();
        combination->deferredNoXOffset = (Vec2D){ 0.f, 0.f };
    }
    deferredCombinationCount = 0;
}

// Groups

void resetGroupData(int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    
    GroupNode *gr = &groups[g]; 

    gr->toggled = false;
    gr->alpha = 1.0f;
}

void addToGroup(int obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    GroupNode *gr = &groups[g]; 
    
    // Initialize
    if (gr->objects == NULL) {
        resetGroupData(g);
    }

    if (gr->count >= gr->capacity) {
        size_t newCapacity = gr->capacity == 0 ? 32 : gr->capacity + 32;

        int *newList = (int *)realloc(gr->objects, newCapacity * sizeof(int));
        if (!newList) {
            return;
        }

        gr->objects = newList;
        gr->capacity = newCapacity;
    }
    gr->objects[gr->count++] = obj;
}

GroupNode *getGroup(int g) {
    if (g < 1 || g >= MAX_GROUPS) return NULL;
    return &groups[g];
}

void clearGroups(void) {
    for (int g = 0; g < MAX_GROUPS; g++) {
        groups[g].toggled = false;
        groups[g].alpha = 1.0f;

        groups[g].count = 0;
        groups[g].capacity = 0;
        
        free(groups[g].objects);
        groups[g].objects = NULL;
    }
}

int compareObjects(const void *a, const void *b) {
    int aX = objects.x[*(int *)a];
    int bX = objects.x[*(int *)b];
    int aY = objects.y[*(int *)a];
    int bY = objects.y[*(int *)b];

    if (aX != bX) return aX - bX;   // smaller x first
    return bY - aY;                   // if x same, bigger y first
}


void sortGroup(int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    
    GroupNode *gr = &groups[g];
    // Sort array
    qsort(gr->objects, gr->count, sizeof(int), compareObjects);
}

}
