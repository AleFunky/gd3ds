#pragma once
#include "menus/core/ui_stack.hpp"
#include "utils/server_utils.hpp"

void delete_level();
void check_warnings_and_play();

extern int online_menu_level_id;
extern SearchEntry *current_search_entry;
extern CreatorEntry *current_creator_entry;
extern SongEntry *current_song_entry;

extern bool refresh;

extern const UIScreenDefPair online_level_menu_def;