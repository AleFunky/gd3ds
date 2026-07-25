#pragma once
#include <3ds.h>
#include <citro2d.h>

#include <math.h>
#include "player/collision.h"

#define FADE_DURATION 0.25f
#define FADE_SPEED (255 / FADE_DURATION)

enum FadeStatus {
    FADE_STATUS_NONE,
    FADE_STATUS_OUT,
    FADE_STATUS_IN
};

enum StereoEye {
    EYE_LEFT,
    EYE_RIGHT
};

// Where each layer sits relative to the screen (negative goes into it, positive pops out)
#define DEPTH_BACKGROUND (-1.f)
#define DEPTH_LEVEL      (0.3f)
// Text is the hardest thing to fuse, so the UI stays close to the screen
#define DEPTH_UI         (0.5f)
#define DEPTH_POPUP      (1.f)
void draw_hitbox_line_inward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color);

void draw_polygon_inward_mitered(Vec2D *poly, int n, float thickness, u32 color); 

float calc_x_on_screen(float val);
float mirror_x_on_screen(float val);
float calc_y_on_screen(float val);

// Fading
bool handle_fading();
void draw_fade();
void set_fade_status(int status);
int get_fade_status();
C3D_RenderTarget* C2D_CreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa);

void set_wide(bool wide);
void reinitialize_screens();

// Stereoscopic 3D
void set_stereo(bool stereo);
void apply_screen_modes();
bool begin_top_eye(int eye);
bool is_extra_eye();
void begin_eye_layer(float depth);
void end_eye_layer();
float get_depth_shift(float depth);

float get_mirror_x(float x, float factor);

void draw_9_slice(const C2D_Image atlas, const float x, const float y, const float sx, const float sy, const int width, const int height, const float border, const u32 color);
Tex3DS_SubTexture select_box(const C2D_Image *image, int x, int y, int endX, int endY);
void set_scissor(GPU_SCISSORMODE mode, int x, int y, int width, int height);
bool C2D_DrawTriangleUV(float x0, float y0, float u0, float v0, u32 clr0, float x1, float y1, float u1, float v1, u32 clr1, float x2, float y2, float u2, float v2, u32 clr2, float depth, C2D_Image img);

void custom_circunference (const float x, const float y, const float radius,
                     const u32 color, const float lineWidth);
void custom_circle (const float x, const float y, const float radius,
                     const u32 color);
