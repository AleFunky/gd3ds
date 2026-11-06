#pragma once
#include <math.h>
#include "graphics.h"

#define BEAM_COUNT 8
#define BASE_W 1
#define W_MID 30
#define W_JITTER 20
#define MAX_BEAM_W ((int) sqrtf(SCREEN_WIDTH_AREA * SCREEN_WIDTH_AREA + SCREEN_HEIGHT_AREA * SCREEN_HEIGHT_AREA) + 32.5f)
#define DUR_BASE (180 / 1000.f)
#define DUR_JITTER (40 / 1000.f)
#define STAGGER_STEP (195 / 1000.f)
#define STAGGER_BASE (40 / 1000.f)
#define STAGGER_RAND (40 / 1000.f)
#define ALPHA_BASE (155 / 255.f)
#define ALPHA_JITTER (100 / 255.f)
#define FADE_DELAY_AFTER (400 / 1000.f)
#define ANGLE_START -135
#define ANGLE_STEP (90.f / BEAM_COUNT)

#define RAY_FADE_DURATION (400 / 1000.f)

void rays_start();
void rays_start_fade();
void update_rays(float delta);
void draw_rays();