#include "gfx.hpp"
#include "math_helpers.hpp"
#include "graphics.hpp"
#include "main.hpp"
#include "state.hpp"
#include "menus/settings_hub/settings.hpp"
#include "utils/utils.hpp"

namespace gd3ds
{

void setScissor(GPU_SCISSORMODE mode, int x, int y, int width, int height) {
    int screenWidth = GSP_SCREEN_HEIGHT_BOTTOM;
    int screen_height = GSP_SCREEN_WIDTH;

    int invX = screenWidth - (x + width);
    int invY = screen_height - (y + height);

    C2D_Flush();
    C3D_SetScissor(mode,
        CLAMP(invY         , 0, screen_height), // left (Y min)
        CLAMP(invX         , 0, screenWidth),  // top (X min)
        CLAMP(invY + height, 0, screen_height), // right (Y max)
        CLAMP(invX + width , 0, screenWidth)   // bottom (X max)
    );
    C2D_Prepare();
}

// Texture area selection
Tex3DS_SubTexture selectBox(const C2D_Image *image, int x, int y, int endX, int endY) {
    Tex3DS_SubTexture tex = *image->subtex;
    if (x != endX) {
        int deltaX  = endX - x;
        float texRL = tex.left - tex.right;
        tex.left    = tex.left - (float)texRL / tex.width * x;
        tex.right   = tex.left - (float)texRL / tex.width * deltaX;
        tex.width   = deltaX;
    }
    if (y != endY) {
        float texTB = tex.top - tex.bottom;
        int deltaY  = endY - y;
        tex.top     = tex.top - (float)texTB / tex.height * y;
        tex.bottom  = tex.top - (float)texTB / tex.height * deltaY;
        tex.height  = deltaY;
    }
    return tex;
}

// Helper macro to select tile from 3x3 grid
#define TILE(ix, iy) selectBox(&atlas, ix*b, iy*b, ix*b + b, iy*b + b)

// Draw a 9 slice rectangle (useful for things like windows)
void draw9Slice(const C2D_Image atlas, const float x, const float y, const float sx, const float sy, const int width, const int height, const float border, const u32 color) {
    // Set color
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color, 1.f);

    float cx = x;
    float cy = y;

    float w = width * fabsf(sx);
    float h = height * fabsf(sy);

    float b = border;

    float bx = border * fabsf(sx);
    float by = border * fabsf(sy);

    if (w < 2 * bx) w = 2 * bx;
    if (h < 2 * by) h = 2 * by;

    float halfW = w * 0.5f;
    float halfH = h * 0.5f;

    float midW = w - 2 * bx;
    float midH = h - 2 * by;

    C2D_Sprite spr;

    Tex3DS_SubTexture sub;
    C2D_Image img;

    // Corners
    sub = TILE(0,0);
    img = atlas; img.subtex = &sub;
    C2D_SpriteFromImage(&spr, img);
    C2D_SpriteSetScale(&spr, sx, sy);
    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, cx - halfW + bx*0.5f, cy - halfH + by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,0);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - bx*0.5f, cy - halfH + by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx - halfW + bx*0.5f, cy + halfH - by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - bx*0.5f, cy + halfH - by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Edges

    sub = TILE(1,0);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, sx * (midW / bx), sy);
    C2D_SpriteSetPos(&spr, cx, cy - halfH + by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(1,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx, cy + halfH - by*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, sx, sy * (midH / by));
    C2D_SpriteSetPos(&spr, cx - halfW + bx*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,1);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - bx*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Center

    sub = TILE(1,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, sx * (midW / bx), sy * (midH / by));
    C2D_SpriteSetPos(&spr, cx, cy);
    C2D_DrawSpriteTinted(&spr, &tint);
}

#undef TILE

#define DISPLAY_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
                GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8))

C3D_RenderTarget* c2dCreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa) {
    u8 model = getModel();
    bool wide = settingsState.wideEnabled && model != CFG_MODEL_2DS && !isCitra() && screen == GFX_TOP;

    int height;
    switch (screen)
    {
        default:
        case GFX_BOTTOM:
            height = GSP_SCREEN_HEIGHT_BOTTOM * (aa ? 2 : 1);
            break;
        case GFX_TOP:
            height = (wide || aa) ? GSP_SCREEN_HEIGHT_TOP_2X : GSP_SCREEN_HEIGHT_TOP;
            break;
    }
    int width = aa ? 480 : 240;

    u32 transferFlags = DISPLAY_FLAGS;
    if (aa && !wide)
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY);
    else if (aa && wide)
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_X);
    else
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

    C3D_RenderTarget* target = C3D_RenderTargetCreate(width, height, GPU_RB_RGBA8, GPU_RB_DEPTH16);
    if (target) C3D_RenderTargetSetOutput(target, screen, side, transferFlags);

    return target;
}

FadeStatus fadeStatus = FadeStatus::FadeStatusNone;
float opacity = 0;

// How far apart both eyes get at most, in pixels
#define MAX_EYE_SHIFT 10.f

// Under this both eyes come out identical, so one of them is enough
#define STEREO_DEADZONE (0.08f / MAX_EYE_SHIFT)

// Wait a moment before freeing, so slider wiggles don't thrash VRAM
#define STEREO_IDLE_FRAMES 60

static int stereoIdleFrames = 0;

// The slider, snapped to 0 where 3D wouldn't show up anyway
float getStereoSlider() {
    if (!settingsState.stereoEnabled) return 0.f;

    float slider = osGet3DSliderState();
    return slider < STEREO_DEADZONE ? 0.f : slider;
}

void reinitializeScreens() {
    if (top) C3D_RenderTargetDelete(top);
    if (bot) C3D_RenderTargetDelete(bot);
    if (topRight) C3D_RenderTargetDelete(topRight);

    top = c2dCreateScreenTargetExt(GFX_TOP, GFX_LEFT, false);
    bot = c2dCreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, false);

    // The second eye only gets VRAM once the slider asks for it
    topRight = (getStereoSlider() > 0.f ? c2dCreateScreenTargetExt(GFX_TOP, GFX_RIGHT, false) : NULL);
    stereoIdleFrames = 0;
}

// Lends the right eye a target while the slider is up. Call outside a frame
void updateStereoTarget() {
    if (getStereoSlider() > 0.f) {
        stereoIdleFrames = 0;
        if (!topRight) topRight = c2dCreateScreenTargetExt(GFX_TOP, GFX_RIGHT, false);
        return;
    }

    if (!topRight) return;

    if (++stereoIdleFrames < STEREO_IDLE_FRAMES) return;

    C3D_RenderTargetDelete(topRight);
    topRight = NULL;
}

void setWide(bool wide) {
    u8 model = getModel();
    if (model != CFG_MODEL_2DS && !isCitra()) {
        settingsState.wideEnabled = wide;
        gfxSetWide(wide);
    }
}

// Neither 2DS has a second eye
bool stereoSupported() {
    u8 model = getModel();
    return model != CFG_MODEL_2DS && model != CFG_MODEL_N2DSXL;
}

void setStereo(bool stereo) {
    settingsState.stereoEnabled = stereo && stereoSupported();
    gfxSet3D(settingsState.stereoEnabled);
}

// Wide and 3D can't share the top screen, so the old one has to go first
void applyScreenModes() {
    // A New 2DS XL keeps wide, so only give it up for real 3D
    if (settingsState.stereoEnabled && stereoSupported()) {
        setWide(false);
        setStereo(true);
    } else {
        setStereo(false);
        setWide(settingsState.wideEnabled);
    }
}

// How deep begin_eye_layer() calls can be nested
#define MAX_EYE_LAYERS 8

static float stereoStrength = 0.f;
static int currentEye = static_cast<int>(StereoEye::EyeLeft);
static bool drawingTopEye = false;

static C3D_Mtx layerViews[MAX_EYE_LAYERS];
static int layerCount = 0;

// True only while the top screen is being drawn in 3D. Everything else bails out
// early, so 2D and the bottom screen render exactly like they did before
bool isStereoActive() {
    return drawingTopEye && stereoStrength > 0.f;
}

// How much this eye moves something floating at the given depth
float getDepthShift(float depth) {
    return (currentEye == static_cast<int>(StereoEye::EyeLeft) ? 1.f : -1.f) * depth * MAX_EYE_SHIFT * stereoStrength;
}

// Sets the top screen up for one eye, returns false once every eye has been drawn
bool beginTopEye(int eye) {
    return beginTopEyeTarget(eye, top);
}

bool beginTopEyeTarget(int eye, C3D_RenderTarget *primary) {
    // The slider only gets read once per frame so both eyes agree on the depth
    if (eye == static_cast<int>(StereoEye::EyeLeft)) {
        stereoStrength = (primary == top && topRight) ? getStereoSlider() : 0.f;
    }

    // One eye is enough with the slider down
    if (eye > (stereoStrength > 0.f ? static_cast<int>(StereoEye::EyeRight) : static_cast<int>(StereoEye::EyeLeft))) {
        currentEye = static_cast<int>(StereoEye::EyeLeft);
        drawingTopEye = false;
        return false;
    }

    currentEye = eye;
    drawingTopEye = true;

    C3D_RenderTarget *target = (eye == static_cast<int>(StereoEye::EyeRight) ? topRight : primary);
    
    C2D_SceneBegin(target);
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 255));

    return true;
}

// True while redrawing the screen for another eye, so nothing gets updated twice
bool isExtraEye() {
    return currentEye != static_cast<int>(StereoEye::EyeLeft);
}

// Shifts everything drawn after this to the given depth, pair it with end_eye_layer().
// Does nothing in 2D, touching the view would split the batch for no reason
void beginEyeLayer(float depth) {
    if (!isStereoActive()) return;

    // Still count layers we have no room for, so the pairing keeps working
    if (layerCount++ >= MAX_EYE_LAYERS) return;

    C3D_Mtx *saved = &layerViews[layerCount - 1];
    C2D_ViewSave(saved);

    // The view can be scaled, so divide it out to keep the shift in screen pixels.
    // Whole pixels only, half pixel shifts get filtered and smear thin lines and text
    float scale = saved->r[0].x;
    if (scale != 0.f) C2D_ViewTranslate(roundf(getDepthShift(depth)) / scale, 0);
}

void endEyeLayer() {
    if (!isStereoActive()) return;

    if (layerCount <= 0) return;

    if (--layerCount < MAX_EYE_LAYERS) C2D_ViewRestore(&layerViews[layerCount]);
}

float getMirrorX(float x, float factor) {
    float width = SCREEN_WIDTH_AREA_GAMEPLAY(settingsState.swapGameplayScreens && !inLevelComplete);

    if (factor == 0) return x;
    else if (factor == 1) return width - x;
    
    return x + factor * (width - 2.0f * x);
}

#include "c2d_internal.h"

bool c2dDrawTriangleUV(float x0, float y0, float u0, float v0, u32 clr0, float x1, float y1, float u1, float v1, u32 clr1, float x2, float y2, float u2, float v2, u32 clr2, float depth, C2D_Image img) {
	C2Di_Context* ctx = C2Di_GetContext();
	if (!(ctx->flags & C2DiF_Active))
		return false;
	if (!C2Di_CheckBufSpace(ctx, 3, 3))
		return false;

	C2Di_SetMode((ctx->flags & C2DiF_TintMode_Mask) >> (C2DiF_TintMode_Shift - C2DiF_Mode_Shift));
	C2Di_SetTex(img.tex);
	C2Di_Update();

	C2Di_AppendTri();
	C2Di_AppendVtx(x0, y0, depth, u0, v0, 0, 1.f, clr0);
	C2Di_AppendVtx(x1, y1, depth, u1, v1, 0, 1.f, clr1);
	C2Di_AppendVtx(x2, y2, depth, u2, v2, 0, 1.f, clr2);
	return true;
}

#define TRIG_TABLE_BITS 12
#define TRIG_TABLE_SIZE (1 << TRIG_TABLE_BITS)
#define TRIG_TABLE_MASK (TRIG_TABLE_SIZE - 1)
#define TRIG_QUARTER (TRIG_TABLE_SIZE / 4)

static float sinTable[TRIG_TABLE_SIZE];

void initTrigTable() {
    for (int i = 0; i < TRIG_TABLE_SIZE; i++)
        sinTable[i] = sinf((M_PI * 2) * i / TRIG_TABLE_SIZE);
}

float lutSin(float angle) {
    const float scaled = angle * (TRIG_TABLE_SIZE / (M_PI * 2));
    const int index = (int)(scaled) & TRIG_TABLE_MASK;

    return sinTable[index];
}

float lutCos(float angle) {
    const float scaled = angle * (TRIG_TABLE_SIZE / (M_PI * 2));
    const int index = (int)(scaled) & TRIG_TABLE_MASK;
    
    return sinTable[(index + TRIG_QUARTER) & TRIG_TABLE_MASK];
}


void c2dFastCalcQuad(C2Di_Quad* quad, const C2D_DrawParams* params) {
    const float width = fabsf(params->pos.w);
    const float height = fabsf(params->pos.h);
    const float centerX = params->center.x;
    const float centerY = params->center.y;
    const float positionX = params->pos.x;
    const float positionY = params->pos.y;

    float topLeftX;
    float topLeftY;
    float widthX;
    float widthY;
    float heightX;
    float heightY;

    if (params->angle == 0.0f) {
        topLeftX = positionX - centerX;
        topLeftY = positionY - centerY;
        widthX = width;
        widthY = 0.0f;
        heightX = 0.0f;
        heightY = height;
    } else {   
        float sine = lutSin(params->angle);
        float cosine = lutCos(params->angle);

        topLeftX = positionX - centerX * cosine + centerY * sine;
        topLeftY = positionY - centerY * cosine - centerX * sine;

        widthX = width * cosine;
        widthY = width * sine;
        heightX = -height * sine;
        heightY = height * cosine;
    }

    quad->topLeft[0] = topLeftX;
    quad->topLeft[1] = topLeftY;

    quad->topRight[0] = topLeftX + widthX;
    quad->topRight[1] = topLeftY + widthY;

    quad->botLeft[0] = topLeftX + heightX;
    quad->botLeft[1] = topLeftY + heightY;

    quad->botRight[0] = topLeftX + widthX + heightX;
    quad->botRight[1] = topLeftY + widthY + heightY;
}

void calcQuadParams(SpriteObject *vo) {
    c2dFastCalcQuad(&vo->params.quadr, &vo->spr.params);

    // Calculate texcoords
	float tcTopLeft[2], tcTopRight[2], tcBotLeft[2], tcBotRight[2];
	Tex3DS_SubTextureTopLeft    (vo->spr.image.subtex, &tcTopLeft[0],  &tcTopLeft[1]);
	Tex3DS_SubTextureTopRight   (vo->spr.image.subtex, &tcTopRight[0], &tcTopRight[1]);
	Tex3DS_SubTextureBottomLeft (vo->spr.image.subtex, &tcBotLeft[0],  &tcBotLeft[1]);
	Tex3DS_SubTextureBottomRight(vo->spr.image.subtex, &tcBotRight[0], &tcBotRight[1]);

	// Perform flip if needed
	if (vo->spr.params.pos.w < 0) {
		C2Di_SwapUV(tcTopLeft, tcTopRight);
		C2Di_SwapUV(tcBotLeft, tcBotRight);
	}
	if (vo->spr.params.pos.h < 0) {
		C2Di_SwapUV(tcTopLeft, tcBotLeft);
		C2Di_SwapUV(tcTopRight, tcBotRight);
    }

    for (int i = 0; i < 2; i++) {
        
        vo->params.tcBotLeft[i] = tcBotLeft[i];
        vo->params.tcBotRight[i] = tcBotRight[i];
        vo->params.tcTopLeft[i] = tcTopLeft[i];
        vo->params.tcTopRight[i] = tcTopRight[i];
    }
}

bool c2dDrawImageFast(C2D_Image *img, const QuadParams *quadParams, const C2D_DrawParams *params, const u32 color) {
	C2Di_Context* ctx = C2Di_GetContext();
	if (!(ctx->flags & C2DiF_Active))
		return false;
	if (!C2Di_CheckBufSpace(ctx, 6, 4))
		return false;

	C2Di_SetMode((ctx->flags & C2DiF_TintMode_Mask) >> (C2DiF_TintMode_Shift - C2DiF_Mode_Shift));
	C2Di_SetTex(img->tex);
	C2Di_Update();

	// Calculate colors

    const C2Di_Quad quadr = quadParams->quadr;

	C2Di_AppendQuad();
	C2Di_AppendVtx(quadr.topLeft[0],  quadr.topLeft[1],  params->depth, quadParams->tcTopLeft[0],  quadParams->tcTopLeft[1],  0, 1, color);
	C2Di_AppendVtx(quadr.topRight[0], quadr.topRight[1], params->depth, quadParams->tcTopRight[0], quadParams->tcTopRight[1], 0, 1, color);
	C2Di_AppendVtx(quadr.botLeft[0],  quadr.botLeft[1],  params->depth, quadParams->tcBotLeft[0],  quadParams->tcBotLeft[1],  0, 1, color);
	C2Di_AppendVtx(quadr.botRight[0], quadr.botRight[1], params->depth, quadParams->tcBotRight[0], quadParams->tcBotRight[1], 0, 1, color);
	return true;
}

float calcXOnScreen(float val) {
    return getMirrorX(val - state.cameraX, state.mirrorFactor); 
}

// Same as calc_x_on_screen but doesn't do the animation
float mirrorXOnScreen(float val) {
    return getMirrorX(val - state.cameraX, state.mirrorFactor >= 0.5f ? 1 : 0); 
}

float calcYOnScreen(float val) {
    return SCREEN_HEIGHT - ((val - state.cameraY));
}


bool isCcw(Vec2D *poly, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        sum += (poly[j].x - poly[i].x) * (poly[j].y + poly[i].y);
    }
    return sum < 0.0f; // CCW if sum < 0
}

void computeMiteredOffsets(Vec2D *poly, Vec2D *offsets, int n, float thickness, bool ccw) {
    for (int i = 0; i < n; i++) {
        int prev = (i + n - 1) % n;
        int next = (i + 1) % n;

        // Edge vectors
        Vec2D ePrev = { poly[i].x - poly[prev].x, poly[i].y - poly[prev].y };
        Vec2D eNext = { poly[next].x - poly[i].x, poly[next].y - poly[i].y };

        // Normals
        Vec2D nPrev = normalize(perpendicular(ePrev));
        Vec2D nNext = normalize(perpendicular(eNext));

        // Flip to point inward if mirror
        if (state.mirrorFactor < 0.5f) { nPrev.x = -nPrev.x; nPrev.y = -nPrev.y; nNext.x = -nNext.x; nNext.y = -nNext.y; }

        // Miter = normalize(n_prev + n_next)
        Vec2D miter = normalize((Vec2D){ nPrev.x + nNext.x, nPrev.y + nNext.y });

        // Compute miter length to preserve thickness
        float cosHalfAngle = dotVec(miter, nNext); // cos(theta/2)
        float miterLength = thickness / cosHalfAngle;

        offsets[i].x = poly[i].x + miter.x * miterLength;
        offsets[i].y = poly[i].y + miter.y * miterLength;
    }
}

void drawPolygonInwardMitered(Vec2D *poly, int n, float thickness, u32 *color, float depth) {
    bool ccw = isCcw(poly, n);

    Vec2D offsets[n];
    computeMiteredOffsets(poly, offsets, n, thickness, ccw);

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;

        float x1 = poly[i].x;
        float y1 = poly[i].y;
        float x2 = poly[j].x;
        float y2 = poly[j].y;

        float ox1 = offsets[i].x;
        float oy1 = offsets[i].y;
        float ox2 = offsets[j].x;
        float oy2 = offsets[j].y;

        // Triangle 1
        C2D_DrawTriangle(
            x1, y1, color[i],
            x2, y2, color[i],
            ox2, oy2, color[i],
            depth
        );

        // Triangle 2
        C2D_DrawTriangle(
            ox2, oy2, color[i],
            ox1, oy1, color[i],
            x1, y1, color[i],
            depth
        );
    }
}

void drawHitboxLineInward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color, const float depth) {

    float ex = x2 - x1;
    float ey = y2 - y1;
    float length = sqrtf(ex * ex + ey * ey);

    // Inward normal depends on winding
    float nx, ny;
    if (isCcw(rect, 4)) {
        nx = -ey / length;
        ny =  ex / length;
    } else {
        nx =  ey / length;
        ny = -ex / length;
    }

    // Flip it to point inward if not mirror
    if (state.mirrorFactor < 0.5f) {
        nx = -nx;
        ny = -ny;
    }

    // Offset inward
    float ox = nx * thickness;
    float oy = ny * thickness;

    // Build quad
    float x1a = x1;
    float y1a = y1;
    float x2a = x2;
    float y2a = y2;

    float x1b = x1 + ox;
    float y1b = y1 + oy;
    float x2b = x2 + ox;
    float y2b = y2 + oy;

    // First triangle
    C2D_DrawTriangle(
        x1a, y1a, color,
        x2a, y2a, color,
        x2b, y2b, color,
        depth
    );

    // Second triangle
    C2D_DrawTriangle(
        x2b, y2b, color,
        x1b, y1b, color,
        x1a, y1a, color,
        depth
    );
}

void customEllipse(float x, float y, float radiusX,
                    float radiusY, u32 color, bool filled,
                    float lineWidth) {
    int segments = (int)(MAX(radiusX, radiusY) * 0.75f);
    segments = CLAMP(segments, 12, 256);

    float prevX = x + radiusX;
    float prevY = y;

    if (filled) {
        // Triangle fan style (center -> edge points)
        for (int i = 0; i < segments; i++) {
            float angle1 = i * 2.0f * M_PI / segments;
            float angle2 = (i + 1) * 2.0f * M_PI / segments;

            float x1 = cosf(angle1) * radiusX + x;
            float y1 = sinf(angle1) * radiusY + y;

            float x2 = cosf(angle2) * radiusX + x;
            float y2 = sinf(angle2) * radiusY + y;

            // Draw triangle (center, p1, p2)
            C2D_DrawTriangle(
                x, y, color,
                x1, y1, color,
                x2, y2, color,
                0
            );
        }
    } else {
        // Outline using lines
        for (int i = 1; i <= segments; i++) {
            float angle = i * 2.0f * M_PI / segments;

            float currX = cosf(angle) * radiusX + x;
            float currY = sinf(angle) * radiusY + y;

            C2D_DrawLine(
                prevX, prevY, color,
                currX, currY, color,
                lineWidth, 0
            );

            prevX = currX;
            prevY = currY;
        }
    }
}

void customCircle (const float x, const float y, const float radius,
                     const u32 color) {
    customEllipse(x, y, radius, radius, color, true, 1);
}

void customCircunference (const float x, const float y, const float radius,
                     const u32 color, const float lineWidth) {
    customEllipse(x, y, radius, radius, color, false, lineWidth);
}

}
