#pragma once
#include "utils/network.h"

void enable_demons();
void disable_demons();

void update_difficulty_tints();
void search_menu_loop();

extern bool search_needs_refresh;
extern bool gdps;

extern SearchFilters filters;
