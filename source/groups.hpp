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
    int combinationCount;
    int combinationCapacity;

    int collisionObjectCount;
} GroupNode;

bool buildGroupCombinations(void);
void beginGroupCombinationMoveStep(void);
void addMoveToGroupCombinations(int group, float deltaX, float deltaY);
void applyGroupCombinationMove(void);
void beginGroupCombinationRotationStep(void);
bool addRotationToGroupCombinations(int group, const struct Matrix2x2 *matrix, float pivotX, float pivotY);
void applyGroupCombinationRotation(void);
void getGroupObjectPosition(int object, float *x, float *y);
void flushGroupCombinationVisualMoves(void);
void discardGroupCombinationVisualMoves(void);

void clearGroups(void);
GroupNode *getGroup(int g);
void addToGroup(int obj, int g);
void resetGroupData(int g);

void sortGroup(int g);