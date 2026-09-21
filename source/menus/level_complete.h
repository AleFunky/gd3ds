#pragma once
#include "menus/core/ui_element.h"

#define COMPLETE_COIN_FILLED_ID 26
#define COMPLETE_COIN_UNFILLED_ID 23

void level_complete_init();
int level_complete_loop(UIInput *touch);
void level_complete_destroy();
void draw_level_complete_top();
void draw_level_complete();