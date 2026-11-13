#pragma once
#include "menus/core/ui_stack.hpp"

extern bool inInfoCard;

void actionOpenInfoCard(int id);
void actionOpenInfoCardText(const char *text);

void openSoundtrack();

extern const UIScreenDefPair mainMenuDef;