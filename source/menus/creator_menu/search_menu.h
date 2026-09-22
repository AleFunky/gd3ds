#pragma once
#include "utils/network.h"
#include "menus/core/ui_stack.h"

void enable_demons(UIScreen *s);
void disable_demons(UIScreen *s);

void update_difficulty_tints(UIScreen *s);

extern bool search_needs_refresh;
extern bool gdps;

extern SearchFilters filters;

extern const UIScreenDefPair search_menu_def;
