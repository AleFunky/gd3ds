#include "key_effect.h"

#include <math.h>
#include "math_helpers.h"
#include "main.h"
#include "state.h"
#include "graphics.h"
#include "utils/gfx.h"
#include "easing.h"

typedef struct {
    float x, y;
    float vel_x, vel_y;
    float gravity;
    float elapsed;
    float opacity;
    float scale_x, scale_y;

    int base_tex;
    int detail_tex;
    int main_channel;
    int detail_channel;

    bool active;
} KeyCollectEffect;

static KeyCollectEffect key_effects[KEY_EFFECT_POOL_SIZE] = { 0 };

#define KEY_DEG_TO_RAD (3.14159265358979f / 180.0f)

void key_effect_spawn(float x, float y, int base_tex, int detail_tex,
                      int main_channel, int detail_channel,
                      float scale_x, float scale_y,
                      unsigned char flip_x, unsigned char flip_y) {
    for (int i = 0; i < KEY_EFFECT_POOL_SIZE; i++) {
        KeyCollectEffect *e = &key_effects[i];
        if (e->active) continue;

        e->x = x;
        e->y = y;

        float angle = (KEY_EFFECT_ANGLE + KEY_EFFECT_ANGLE_VAR * random_float(-1.0f, 1.0f)) * KEY_DEG_TO_RAD;
        float speed = KEY_EFFECT_SPEED + KEY_EFFECT_SPEED_VAR * random_float(-1.0f, 1.0f);
        e->vel_x = cosf(angle) * speed;
        e->vel_y = sinf(angle) * speed;
        e->gravity = KEY_EFFECT_GRAVITY_Y;

        e->elapsed = 0.0f;
        e->opacity = 1.0f;
        e->scale_x = scale_x * (flip_x ? -1.0f : 1.0f);
        e->scale_y = scale_y * (flip_y ? -1.0f : 1.0f);
        e->base_tex = base_tex;
        e->detail_tex = detail_tex;
        e->main_channel = main_channel;
        e->detail_channel = detail_channel;
        e->active = true;
        break;
    }
}

void update_key_effect(float delta) {
    for (int i = 0; i < KEY_EFFECT_POOL_SIZE; i++) {
        KeyCollectEffect *e = &key_effects[i];
        if (!e->active) continue;

        e->vel_y += e->gravity * delta;
        e->x += e->vel_x * delta;
        e->y += e->vel_y * delta;

        e->elapsed += delta;
        e->opacity = easeValue(EASE_OUT, 1.0f, 0.0f, e->elapsed, KEY_EFFECT_LIFE, 2.0f);

        if (e->elapsed >= KEY_EFFECT_LIFE) e->active = false;
    }
}

static void draw_key_layer(int tex, float px, float py, int channel, float opacity, float scale_x, float scale_y) {
    if (tex < 0) return;

    ColorChannel *c = &channels[get_col_channel_index(channel)];
    u8 alpha = (u8)(opacity * c->alpha * 255.0f);
    u32 tint = C2D_Color32(c->color.r, c->color.g, c->color.b, alpha);

    C2D_Sprite spr = { 0 };
    C2D_ImageTint itint = { 0 };
    C2D_PlainImageTint(&itint, tint, 1.0f);

    int rel;
    C2D_SpriteSheet *sheet = get_sprite_sheet_ex(tex, &rel);
    C2D_SpriteFromSheet(&spr, *sheet, rel);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, px, py);
    C2D_SpriteSetScale(&spr, scale_x * state.mirror_mult, scale_y);
    C2D_DrawSpriteTinted(&spr, &itint);
}

void draw_key_effect() {
    for (int i = 0; i < KEY_EFFECT_POOL_SIZE; i++) {
        KeyCollectEffect *e = &key_effects[i];
        if (!e->active) continue;

        float calc_x = e->x - state.camera_x;
        float calc_y = SCREEN_HEIGHT - (e->y - state.camera_y);

        float px = get_mirror_x(calc_x, state.mirror_factor) + KEY_EFFECT_DRAW_OFFSET;
        float py = calc_y + KEY_EFFECT_DRAW_OFFSET;

        draw_key_layer(e->base_tex, px, py, e->main_channel, e->opacity, e->scale_x, e->scale_y);
        draw_key_layer(e->detail_tex, px, py - KEY_DETAIL_UP_PX_WORLD * e->scale_y, e->detail_channel, e->opacity, e->scale_x, e->scale_y);
    }
}
