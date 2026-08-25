#pragma once
#include "utils/network.h"

void update_difficulty_tints();
void search_menu_loop();

extern char search_query[129];
extern bool search_needs_refresh;

extern SearchFilters filters;
