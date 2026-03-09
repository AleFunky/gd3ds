#pragma once
#include <3ds.h>
#include <citro2d.h>

#include <math.h>

// To get around the gpu weirdness
inline float closest_even_mult(float value, float mult)
{
    float r = value * mult;
    int n = (int)roundf(r);

    if (n & 1)
        n += (r > n) ? 1 : -1;

    return (float)n / value;
}

#define FADE_DURATION 0.25f
#define FADE_SPEED (255 / FADE_DURATION)

enum FadeStatus {
    FADE_STATUS_NONE,
    FADE_STATUS_OUT,
    FADE_STATUS_IN
};

// Fading
bool handle_fading();
void draw_fade();
void set_fade_status(int status);
C3D_RenderTarget* C2D_CreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa);
void scale_view();

void set_wide(bool wide);
void set_aa(bool aa);
void reinitialize_screens();

void draw_9_slice(const C2D_Image atlas, const float x, const float y, const int width, const int height, const float border, const u32 color);
Tex3DS_SubTexture select_box(const C2D_Image *image, int x, int y, int endX, int endY);
void set_scissor(GPU_SCISSORMODE mode, int x, int y, int width, int height);
