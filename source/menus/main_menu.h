#pragma once
#include "menus/core/ui_stack.h"

extern bool in_info_card;

void action_open_info_card(int id);
void action_open_info_card_text(const char *text);

void open_soundtrack();

extern const UIScreenDefPair main_menu_def;