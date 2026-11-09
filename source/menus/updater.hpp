#pragma once
#include "menus/core/ui_screen.hpp"

extern GenericTask updater_task;
extern Thread updater_thread;
extern bool checkedForUpdate;

extern const UIScreenDefPair updater_def;