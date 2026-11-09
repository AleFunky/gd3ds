#pragma once
#include "utils/network.hpp"
#include "menus/core/ui_stack.hpp"

void enable_demons(UIScreen *s);
void disable_demons(UIScreen *s);

void update_difficulty_tints(UIScreen *s);

extern bool search_needs_refresh;
extern bool gdps;

bool user_coins_counter_visible(void);

extern SearchFilters filters;

extern const UIScreenDefPair search_menu_def;
