#pragma once
#include "utils/network.hpp"
#include "menus/core/ui_stack.hpp"

void enableDemons(UIScreen *s);
void disableDemons(UIScreen *s);

void updateDifficultyTints(UIScreen *s);

extern bool searchNeedsRefresh;
extern bool gdps;

bool userCoinsCounterVisible(void);

extern SearchFilters filters;

extern const UIScreenDefPair searchMenuDef;
