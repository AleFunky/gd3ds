#include "coin_effect.h"

#include "math_helpers.h"
#include "main.h"
#include "state.h"
#include "utils/gfx.h"
#include "easing.h"

typedef struct {
    float x;
    float vel_x;
    float y;
    float vel_y;
    float gravity;

    float fade_start;

    float elapsed;
    float opacity;

    bool already_collected;

    bool active;
} CoinCollectEffect;

static CoinCollectEffect collected_effect[NUM_COINS] = { 0 };

void start_collect_effect(float x, float y) {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collected_effect[i];

        if (!data->active) {
            data->already_collected = false; // TODO: coin saving
            data->elapsed = 0;
            data->x = x;
            data->y = y;
            data->opacity = 1.f;
            data->fade_start = -1;

            data->vel_x = random_float(-1, 1) * COIN_VEL_X;
            data->vel_y = COIN_VEL_Y;
            data->gravity = COIN_GRAVITY;

            data->active = true;
            break;
        }
    }
}

void update_collect_effect(float delta) {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collected_effect[i];

        if (data->active) {
            data->x += data->vel_x * delta;
            data->y += data->vel_y * delta;
            data->vel_y += data->gravity * delta;

            if (data->vel_y < 0) {
                if (data->fade_start < 0) {
                    data->fade_start = data->elapsed;
                }
                data->opacity = easeValue(EASE_IN, 1, 0, data->elapsed - data->fade_start, COIN_COLLECT_DURATION - data->fade_start, 1.f);
            }

            data->elapsed += delta;

            if (data->elapsed >= COIN_COLLECT_DURATION) {
                data->active = false;
            }
        }
    }
}

void draw_collect_effect() {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collected_effect[i];

        if (data->active) {
            C2D_Sprite spr = { 0 };
            C2D_ImageTint tint = { 0 };

            C2D_PlainImageTint(&tint, C2D_Color32f(1, 1, 1, data->opacity), 1.f);

            int index = get_coin_texture(game_objects[SECRET_COIN].texture, 12) - SPRITESHEET2_START;
            
            float calc_x = (data->x - state.camera_x);
            float calc_y = SCREEN_HEIGHT - ((data->y - state.camera_y));  
            
            C2D_SpriteFromSheet(&spr, spriteSheet2, index);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            C2D_SpriteSetPos(&spr, get_mirror_x(calc_x, state.mirror_factor), calc_y);
            C2D_SpriteSetScale(&spr, state.mirror_mult, 1.f);
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
}