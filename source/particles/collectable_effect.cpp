#include "collectable_effect.hpp"

#include <math.h>
#include "math_helpers.hpp"
#include "main.hpp"
#include "state.hpp"
#include "graphics.hpp"
#include "utils/gfx.hpp"
#include "easing.hpp"

typedef struct {
    float x, y;
    float vel_x, vel_y;
    float gravity;
    float elapsed;
    float opacity;
    float scale_x, scale_y;
    float detail_x, detail_y;

    int base_tex;
    int detail_tex;
    int main_channel;
    int detail_channel;

    bool active;
} CollectableEffect;

static CollectableEffect collectable_effects[COLLECTABLE_EFFECT_POOL_SIZE] = { 0 };

#define COLLECTABLE_DEG_TO_RAD (3.14159265358979f / 180.0f)

void collectable_effect_spawn(float x, float y, int base_tex, int detail_tex,
                              int main_channel, int detail_channel,
                              float detail_x, float detail_y,
                              float scale_x, float scale_y,
                              unsigned char flip_x, unsigned char flip_y) {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectable_effects[i];
        if (e->active) continue;

        e->x = x;
        e->y = y;

        float angle = (COLLECTABLE_EFFECT_ANGLE + COLLECTABLE_EFFECT_ANGLE_VAR * random_float(-1.0f, 1.0f)) * COLLECTABLE_DEG_TO_RAD;
        float speed = COLLECTABLE_EFFECT_SPEED + COLLECTABLE_EFFECT_SPEED_VAR * random_float(-1.0f, 1.0f);
        e->vel_x = cosf(angle) * speed;
        e->vel_y = sinf(angle) * speed;
        e->gravity = COLLECTABLE_EFFECT_GRAVITY_Y;

        e->elapsed = 0.0f;
        e->opacity = 1.0f;
        e->scale_x = scale_x * (flip_x ? -1.0f : 1.0f);
        e->scale_y = scale_y * (flip_y ? -1.0f : 1.0f);
        e->detail_x = detail_x;
        e->detail_y = detail_y;
        e->base_tex = base_tex;
        e->detail_tex = detail_tex;
        e->main_channel = main_channel;
        e->detail_channel = detail_channel;
        e->active = true;
        break;
    }
}

void update_collectable_effect(float delta) {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectable_effects[i];
        if (!e->active) continue;

        e->vel_y += e->gravity * delta;
        e->x += e->vel_x * delta;
        e->y += e->vel_y * delta;

        e->elapsed += delta;
        e->opacity = easeValue(EASE_OUT, 1.0f, 0.0f, e->elapsed, COLLECTABLE_EFFECT_LIFE, 2.0f);

        if (e->elapsed >= COLLECTABLE_EFFECT_LIFE) e->active = false;
    }
}

static void draw_collectable_layer(int tex, float px, float py, int channel, float opacity, float scale_x, float scale_y) {
    if (tex < 0) return;

    ColorChannel *c = &channels[get_col_channel_index(channel)];
    u8 alpha = (u8)(opacity * c->alpha * 255.0f);
    u32 tint = C2D_Color32(c->color.r, c->color.g, c->color.b, alpha);

    C2D_Sprite spr = { 0 };
    C2D_ImageTint itint = { 0 };
    C2D_PlainImageTint(&itint, tint, 1.0f);

    int rel;
    C2D_SpriteSheet sheet = get_sprite_sheet_ex(tex, &rel);
    C2D_SpriteFromSheet(&spr, sheet, rel);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, px, py);
    C2D_SpriteSetScale(&spr, scale_x * state.mirror_mult, scale_y);
    C2D_DrawSpriteTinted(&spr, &itint);
}

void draw_collectable_effect() {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectable_effects[i];
        if (!e->active) continue;

        float calc_x = e->x - state.camera_x;
        float calc_y = SCREEN_HEIGHT - (e->y - state.camera_y);

        float px = get_mirror_x(calc_x, state.mirror_factor) + COLLECTABLE_EFFECT_DRAW_OFFSET;
        float py = calc_y + COLLECTABLE_EFFECT_DRAW_OFFSET;
        float detail_px = px + e->detail_x * e->scale_x * state.mirror_mult;
        float detail_py = py - e->detail_y * e->scale_y;

        draw_collectable_layer(e->base_tex, px, py, e->main_channel, e->opacity, e->scale_x, e->scale_y);
        draw_collectable_layer(e->detail_tex, detail_px, detail_py, e->detail_channel, e->opacity, e->scale_x, e->scale_y);
    }
}
