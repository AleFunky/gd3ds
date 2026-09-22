#pragma once
#include "menus/core/ui_element.h"

#include <stdbool.h>

#define RAY_SPAWN_TIME 0.2f
#define RAY_SPAWN_DELAY 0.2f

#define FIREWORK_SPAWN_TIME 2.f
#define FIREWORK_SPAWN_DELAY 0.23f

#define MENU_TIME (FIREWORK_SPAWN_TIME + 1.5f)

#define CIRCUNFERENCE_COUNT 5
#define CIRCUNFERENCE_SPAWN_DELAY 0.05f

#define END_EFFECT_COUNT 120
#define FIREWORK_COUNT 6

#define LVL_COMPLETE_IMAGE_ID 91
#define LVL_COMPLETE_PRACTICE_IMAGE_ID 147

#define LVL_COMPLETE_STATE_0_DURATION 0.66f
#define LVL_COMPLETE_STATE_0_TARGET_SCALE 1.1f

#define LVL_COMPLETE_STATE_1_DURATION 0.88f

#define LVL_COMPLETE_STATE_2_DURATION 0.22f
#define LVL_COMPLETE_STATE_2_TARGET_SCALE 0.01f

int handle_wall_cutscene(float delta, UIInput *touch);

void clear_level_complete_popup();
void handle_level_complete_popup(float delta);
void draw_level_complete_popup();