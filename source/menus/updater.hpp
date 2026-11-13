#pragma once
#include "menus/core/ui_screen.hpp"

namespace gd3ds
{
extern GenericTask updaterTask;
extern Thread updaterThread;
extern bool checkedForUpdate;

extern const UIScreenDefPair updaterDef;
}