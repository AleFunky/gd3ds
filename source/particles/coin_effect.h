#pragma once

#define NUM_COINS 8
#define COIN_COLLECT_DURATION 0.8f

#define COIN_VEL_X 70.f
#define COIN_VEL_Y 580.f
#define COIN_GRAVITY -1800.f

void start_collect_effect(float x, float y);
void update_collect_effect(float delta);
void draw_collect_effect();