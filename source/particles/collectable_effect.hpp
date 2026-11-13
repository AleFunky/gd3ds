#pragma once

#define COLLECTABLE_EFFECT_POOL_SIZE 16

#define COLLECTABLE_EFFECT_ANGLE      90.0f
#define COLLECTABLE_EFFECT_ANGLE_VAR   7.0f
#define COLLECTABLE_EFFECT_SPEED     400.0f
#define COLLECTABLE_EFFECT_SPEED_VAR  10.0f
#define COLLECTABLE_EFFECT_GRAVITY_Y -1500.0f
#define COLLECTABLE_EFFECT_LIFE        3.0f
#define COLLECTABLE_EFFECT_DRAW_OFFSET 6.0f

void collectableEffectSpawn(float x, float y, int baseTex, int detailTex,
                              int mainChannel, int detailChannel,
                              float detailX, float detailY,
                              float scaleX, float scaleY,
                              unsigned char flipX, unsigned char flipY);
void updateCollectableEffect(float delta);
void drawCollectableEffect();
