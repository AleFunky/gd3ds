#pragma once
#include "menus/core/ui_stack.hpp"

extern bool inInfoCard;

namespace gd3ds
{

void actionOpenInfoCard(int id);
void actionOpenInfoCardText(const char *text);

void openSoundtrack();

extern const UIScreenDefPair mainMenuDef;

}