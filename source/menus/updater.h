#pragma once
#include "menus/core/ui_stack.h"

extern GenericTask updater_task;
extern Thread updater_thread;
extern bool checkedForUpdate;

extern const UIScreenDefPair updater_def;