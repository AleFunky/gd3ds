#pragma once

#include <3ds.h>

#define AUTO_CHECKPOINT_TIME 1.f

namespace gd3ds
{

extern int checkpointCount;
extern int checkpointPointer;
extern bool pseudoCheckpointExists;
extern bool restoredCheckpoint;

void startPracticeMode();
void exitPracticeMode();

int getCheckpointCount();

void newCheckpoint();
void restoreCheckpoint();
void deleteLastCheckpoint();

void handlePracticeMode();
void drawCheckpoints();
void clearPracticeMode();

void handleAutoCheckpoints(float delta);
void runPendingCheckpoint();
void setCheckpointTimer(float timer);

void loadUsedChannels();

}
