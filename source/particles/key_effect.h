#pragma once

#define KEY_EFFECT_POOL_SIZE 16

#define KEY_EFFECT_ANGLE      90.0f
#define KEY_EFFECT_ANGLE_VAR   7.0f
#define KEY_EFFECT_SPEED     400.0f
#define KEY_EFFECT_SPEED_VAR  10.0f
#define KEY_EFFECT_GRAVITY_Y -1500.0f
#define KEY_EFFECT_LIFE        3.0f
#define KEY_EFFECT_DRAW_OFFSET 6.0f

#define KEY_DETAIL_UP_PX_WORLD (4.0f / 3.0f)

void key_effect_spawn(float x, float y, int base_tex, int detail_tex,
                      int main_channel, int detail_channel,
                      float scale_x, float scale_y,
                      unsigned char flip_x, unsigned char flip_y);
void update_key_effect(float delta);
void draw_key_effect();
