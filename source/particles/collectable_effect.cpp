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
    float velX, velY;
    float gravity;
    float elapsed;
    float opacity;
    float scaleX, scaleY;
    float detailX, detailY;

    int baseTex;
    int detailTex;
    int mainChannel;
    int detailChannel;

    bool active;
} CollectableEffect;

static CollectableEffect collectableEffects[COLLECTABLE_EFFECT_POOL_SIZE] = { 0 };

#define COLLECTABLE_DEG_TO_RAD (3.14159265358979f / 180.0f)

void collectableEffectSpawn(float x, float y, int baseTex, int detailTex,
                              int mainChannel, int detailChannel,
                              float detailX, float detailY,
                              float scaleX, float scaleY,
                              unsigned char flipX, unsigned char flipY) {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectableEffects[i];
        if (e->active) continue;

        e->x = x;
        e->y = y;

        float angle = (COLLECTABLE_EFFECT_ANGLE + COLLECTABLE_EFFECT_ANGLE_VAR * randomFloat(-1.0f, 1.0f)) * COLLECTABLE_DEG_TO_RAD;
        float speed = COLLECTABLE_EFFECT_SPEED + COLLECTABLE_EFFECT_SPEED_VAR * randomFloat(-1.0f, 1.0f);
        e->velX = cosf(angle) * speed;
        e->velY = sinf(angle) * speed;
        e->gravity = COLLECTABLE_EFFECT_GRAVITY_Y;

        e->elapsed = 0.0f;
        e->opacity = 1.0f;
        e->scaleX = scaleX * (flipX ? -1.0f : 1.0f);
        e->scaleY = scaleY * (flipY ? -1.0f : 1.0f);
        e->detailX = detailX;
        e->detailY = detailY;
        e->baseTex = baseTex;
        e->detailTex = detailTex;
        e->mainChannel = mainChannel;
        e->detailChannel = detailChannel;
        e->active = true;
        break;
    }
}

void updateCollectableEffect(float delta) {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectableEffects[i];
        if (!e->active) continue;

        e->velY += e->gravity * delta;
        e->x += e->velX * delta;
        e->y += e->velY * delta;

        e->elapsed += delta;
        e->opacity = easeValue(EASE_OUT, 1.0f, 0.0f, e->elapsed, COLLECTABLE_EFFECT_LIFE, 2.0f);

        if (e->elapsed >= COLLECTABLE_EFFECT_LIFE) e->active = false;
    }
}

static void drawCollectableLayer(int tex, float px, float py, int channel, float opacity, float scaleX, float scaleY) {
    if (tex < 0) return;

    ColorChannel *c = &channels[getColChannelIndex(channel)];
    u8 alpha = (u8)(opacity * c->alpha * 255.0f);
    u32 tint = C2D_Color32(c->color.r, c->color.g, c->color.b, alpha);

    C2D_Sprite spr = { 0 };
    C2D_ImageTint itint = { 0 };
    C2D_PlainImageTint(&itint, tint, 1.0f);

    int rel;
    C2D_SpriteSheet sheet = getSpriteSheetEx(tex, &rel);
    C2D_SpriteFromSheet(&spr, sheet, rel);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, px, py);
    C2D_SpriteSetScale(&spr, scaleX * state.mirrorMult, scaleY);
    C2D_DrawSpriteTinted(&spr, &itint);
}

void drawCollectableEffect() {
    for (int i = 0; i < COLLECTABLE_EFFECT_POOL_SIZE; i++) {
        CollectableEffect *e = &collectableEffects[i];
        if (!e->active) continue;

        float calcX = e->x - state.cameraX;
        float calcY = SCREEN_HEIGHT - (e->y - state.cameraY);

        float px = getMirrorX(calcX, state.mirrorFactor) + COLLECTABLE_EFFECT_DRAW_OFFSET;
        float py = calcY + COLLECTABLE_EFFECT_DRAW_OFFSET;
        float detailPx = px + e->detailX * e->scaleX * state.mirrorMult;
        float detailPy = py - e->detailY * e->scaleY;

        drawCollectableLayer(e->baseTex, px, py, e->mainChannel, e->opacity, e->scaleX, e->scaleY);
        drawCollectableLayer(e->detailTex, detailPx, detailPy, e->detailChannel, e->opacity, e->scaleX, e->scaleY);
    }
}
