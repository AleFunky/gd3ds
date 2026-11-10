#pragma once
#include <math.h>
#include "graphics.hpp"

#define NEW_BEST_IMAGE_ID 121

#define NEW_BEST_STATE_0_DURATION 0.4f
#define NEW_BEST_STATE_0_TARGET_SCALE 1.f

#define NEW_BEST_STATE_1_DURATION 0.7f

#define NEW_BEST_STATE_2_DURATION 0.2f
#define NEW_BEST_STATE_2_TARGET_SCALE 0.01f

#define NEW_BEST_SEPARATION 25.f

void init_new_best_popup(int progress);
void handle_new_best_popup(float delta);
void draw_new_best_popup();
void clear_new_best_popup();