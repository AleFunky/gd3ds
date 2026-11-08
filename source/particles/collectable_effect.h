#pragma once

#define COLLECTABLE_EFFECT_POOL_SIZE 16

#define COLLECTABLE_EFFECT_ANGLE      90.0f
#define COLLECTABLE_EFFECT_ANGLE_VAR   7.0f
#define COLLECTABLE_EFFECT_SPEED     400.0f
#define COLLECTABLE_EFFECT_SPEED_VAR  10.0f
#define COLLECTABLE_EFFECT_GRAVITY_Y -1500.0f
#define COLLECTABLE_EFFECT_LIFE        3.0f
#define COLLECTABLE_EFFECT_DRAW_OFFSET 6.0f

void collectable_effect_spawn(float x, float y, int base_tex, int detail_tex,
                              int main_channel, int detail_channel,
                              float detail_x, float detail_y,
                              float scale_x, float scale_y,
                              unsigned char flip_x, unsigned char flip_y);
void update_collectable_effect(float delta);
void draw_collectable_effect();
