#pragma once
#include <3ds.h>
#include <citro2d.h>

#include "c2d_internal.h"

#include "graphics.hpp"

#include <math.h>
#include "player/collision.hpp"

#define FADE_DURATION 0.25f
#define FADE_SPEED (255 / FADE_DURATION)

namespace gd3ds
{

enum class StereoEye {
    EyeLeft,
    EyeRight
};

void initTrigTable();
float lutCos(float angle);
float lutSin(float angle);

void c2dFastCalcQuad(C2Di_Quad* quad, const C2D_DrawParams* params);

// Where each layer sits relative to the screen (negative goes into it, positive pops out)
#define DEPTH_BACKGROUND (-1.f)
#define DEPTH_LEVEL      (-0.2f)
// Text is the hardest thing to fuse, so the UI stays close to the screen
#define DEPTH_UI         (0.f)
#define DEPTH_POPUP      (0.1f)
void drawHitboxLineInward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color, const float depth);

void drawPolygonInwardMitered(Vec2D *poly, int n, float thickness, u32 *color, float depth); 

float calcXOnScreen(float val);
float mirrorXOnScreen(float val);
float calcYOnScreen(float val);

C3D_RenderTarget* c2dCreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa);

void setWide(bool wide);
void reinitializeScreens();

// Stereoscopic 3D
bool stereoSupported();
void setStereo(bool stereo);
void applyScreenModes();
void updateStereoTarget();
bool beginTopEye(int eye);
bool beginTopEyeTarget(int eye, C3D_RenderTarget *primary);
bool isExtraEye();
bool isStereoActive();
void beginEyeLayer(float depth);
void endEyeLayer();
float getDepthShift(float depth);

float getStereoSlider();

float getMirrorX(float x, float factor);

void draw9Slice(const C2D_Image atlas, const float x, const float y, const float sx, const float sy, const int width, const int height, const float border, const u32 color);
Tex3DS_SubTexture selectBox(const C2D_Image *image, int x, int y, int endX, int endY);
void setScissor(GPU_SCISSORMODE mode, int x, int y, int width, int height);
bool c2dDrawTriangleUV(float x0, float y0, float u0, float v0, u32 clr0, float x1, float y1, float u1, float v1, u32 clr1, float x2, float y2, float u2, float v2, u32 clr2, float depth, C2D_Image img);

void customCircunference (const float x, const float y, const float radius,
                     const u32 color, const float lineWidth);
void customCircle (const float x, const float y, const float radius,
                     const u32 color);

void calcQuadParams(SpriteObject *vo);
bool c2dDrawImageFast(C2D_Image *img, const QuadParams *quadParams, const C2D_DrawParams *params, const u32 color);

}
