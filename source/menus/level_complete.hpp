#pragma once
#include "menus/core/ui_element.hpp"

#define COMPLETE_COIN_FILLED_ID 26
#define COMPLETE_COIN_UNFILLED_ID 23

namespace gd3ds
{

void levelCompleteInit();
int levelCompleteLoop(UIInput *touch);
void levelCompleteDestroy();
void drawLevelCompleteTop();
void drawLevelComplete();

}