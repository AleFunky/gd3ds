#include "coin_effect.hpp"

#include "math_helpers.hpp"
#include "main.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"
#include "easing.hpp"

namespace gd3ds
{

struct CoinCollectEffect {
    float x;
    float velX;
    float y;
    float velY;
    float gravity;

    float fadeStart;

    float elapsed;
    float opacity;

    bool alreadyCollected;

    bool active;
};

static CoinCollectEffect collectedEffect[NUM_COINS] = { 0 };

void startCollectEffect(float x, float y, bool outline) {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collectedEffect[i];

        if (!data->active) {
            data->alreadyCollected = outline;
            data->elapsed = 0;
            data->x = x;
            data->y = y;
            data->opacity = 1.f;
            data->fadeStart = -1;

            data->velX = randomFloat(-1, 1) * COIN_VEL_X;
            data->velY = COIN_VEL_Y;
            data->gravity = COIN_GRAVITY;

            data->active = true;
            break;
        }
    }
}

void updateCollectEffect(float delta) {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collectedEffect[i];

        if (data->active) {
            data->x += data->velX * delta;
            data->y += data->velY * delta;
            data->velY += data->gravity * delta;

            if (data->velY < 0) {
                if (data->fadeStart < 0) {
                    data->fadeStart = data->elapsed;
                }
                data->opacity = easeValue(EaseTypes::EaseIn, 1, 0, data->elapsed - data->fadeStart, COIN_COLLECT_DURATION - data->fadeStart, 1.f);
            }

            data->elapsed += delta;

            if (data->elapsed >= COIN_COLLECT_DURATION) {
                data->active = false;
            }
        }
    }
}

void drawCollectEffect() {
    for (int i = 0; i < NUM_COINS; i++) {
        CoinCollectEffect *data = &collectedEffect[i];

        if (data->active) {
            C2D_Sprite spr = { 0 };
            C2D_ImageTint tint = { 0 };

            u32 tintColor = levelHasUnverifiedCoins()
                ? C2D_Color32(USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B, (u8)(data->opacity * 255.f))
                : C2D_Color32f(1, 1, 1, data->opacity);
            C2D_PlainImageTint(&tint, tintColor, 1.f);

            int index = getCoinTexture(gameObjects[SECRET_COIN].texture + (state.customLevel ? (data->alreadyCollected ? 8 : 4) : (data->alreadyCollected ? 12 : 0)), 12) - SPRITESHEET2_START;
            
            float calcX = (data->x - state.cameraX);
            float calcY = SCREEN_HEIGHT - ((data->y - state.cameraY));  
            
            C2D_SpriteFromSheet(&spr, spriteSheet2, index);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            C2D_SpriteSetPos(&spr, getMirrorX(calcX, state.mirrorFactor), calcY);
            C2D_SpriteSetScale(&spr, state.mirrorMult, 1.f);
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
}

}