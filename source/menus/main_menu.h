#pragma once
#include "menus/core/ui_element.h"

extern bool in_info_card;

void action_open_info_card(int id);
void action_open_info_card_text(const char *text);
void main_menu_loop();

void open_soundtrack();