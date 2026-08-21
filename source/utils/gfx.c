#include "gfx.h"
#include "math_helpers.h"
#include "graphics.h"
#include "main.h"
#include "state.h"
#include "menus/settings.h"
#include "utils/utils.h"

void set_scissor(GPU_SCISSORMODE mode, int x, int y, int width, int height) {
    int screen_width = GSP_SCREEN_HEIGHT_BOTTOM;
    int screen_height = GSP_SCREEN_WIDTH;

    int inv_x = screen_width - (x + width);
    int inv_y = screen_height - (y + height);

    C2D_Flush();
    C3D_SetScissor(mode,
        CLAMP(inv_y         , 0, screen_height), // left (Y min)
        CLAMP(inv_x         , 0, screen_width),  // top (X min)
        CLAMP(inv_y + height, 0, screen_height), // right (Y max)
        CLAMP(inv_x + width , 0, screen_width)   // bottom (X max)
    );
    C2D_Prepare();
}

// Texture area selection
Tex3DS_SubTexture select_box(const C2D_Image *image, int x, int y, int endX, int endY) {
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
#define TILE(ix, iy) select_box(&atlas, ix*b, iy*b, ix*b + b, iy*b + b)

// Draw a 9 slice rectangle (useful for things like windows)
void draw_9_slice(const C2D_Image atlas, const float x, const float y, const float sx, const float sy, const int width, const int height, const float border, const u32 color) {
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

C3D_RenderTarget* C2D_CreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa) {
    u8 model = get_model();
    bool wide = settingsState.wideEnabled && model != CFG_MODEL_2DS && !is_citra() && screen == GFX_TOP;

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

int fade_status = FADE_STATUS_NONE;
float opacity = 0;

bool handle_fading() {
    if (!fade_status) return false;

    if (fade_status == FADE_STATUS_OUT) {
        opacity += FADE_SPEED * DT;
        if (opacity >= 255) {
            opacity = 255;
            fade_status = FADE_STATUS_NONE;
        }
    } else if (fade_status == FADE_STATUS_IN) {
        opacity -= FADE_SPEED * DT;
        if (opacity <= 0) {
            opacity = 0;
            fade_status = FADE_STATUS_NONE;
        }
    }
    return true;
}

void draw_fade() {
    if (opacity) C2D_Fade(C2D_Color32(0, 0, 0, (int) opacity));
}

void set_fade_status(int status) {
    fade_status = status;
}

int get_fade_status() {
    return fade_status;
}

// How far apart both eyes get at most, in pixels
#define MAX_EYE_SHIFT 10.f

// Under this both eyes come out identical, so one of them is enough
#define STEREO_DEADZONE (0.08f / MAX_EYE_SHIFT)

// Wait a moment before freeing, so slider wiggles don't thrash VRAM
#define STEREO_IDLE_FRAMES 60

static int stereo_idle_frames = 0;

// The slider, snapped to 0 where 3D wouldn't show up anyway
static float get_stereo_slider() {
    if (!settingsState.stereoEnabled) return 0.f;

    float slider = osGet3DSliderState();
    return slider < STEREO_DEADZONE ? 0.f : slider;
}

void reinitialize_screens() {
    if (top) C3D_RenderTargetDelete(top);
    if (bot) C3D_RenderTargetDelete(bot);
    if (top_right) C3D_RenderTargetDelete(top_right);

    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, false);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, false);

    // The second eye only gets VRAM once the slider asks for it
    top_right = (get_stereo_slider() > 0.f ? C2D_CreateScreenTargetExt(GFX_TOP, GFX_RIGHT, false) : NULL);
    stereo_idle_frames = 0;
}

// Lends the right eye a target while the slider is up. Call outside a frame
void update_stereo_target() {
    if (get_stereo_slider() > 0.f) {
        stereo_idle_frames = 0;
        if (!top_right) top_right = C2D_CreateScreenTargetExt(GFX_TOP, GFX_RIGHT, false);
        return;
    }

    if (!top_right) return;

    if (++stereo_idle_frames < STEREO_IDLE_FRAMES) return;

    C3D_RenderTargetDelete(top_right);
    top_right = NULL;
}

void set_wide(bool wide) {
    u8 model = get_model();
    if (model != CFG_MODEL_2DS && !is_citra()) {
        settingsState.wideEnabled = wide;
        gfxSetWide(wide);
    }
}

// Neither 2DS has a second eye
bool stereo_supported() {
    u8 model = get_model();
    return model != CFG_MODEL_2DS && model != CFG_MODEL_N2DSXL;
}

void set_stereo(bool stereo) {
    settingsState.stereoEnabled = stereo && stereo_supported();
    gfxSet3D(settingsState.stereoEnabled);
}

// Wide and 3D can't share the top screen, so the old one has to go first
void apply_screen_modes() {
    // A New 2DS XL keeps wide, so only give it up for real 3D
    if (settingsState.stereoEnabled && stereo_supported()) {
        set_wide(false);
        set_stereo(true);
    } else {
        set_stereo(false);
        set_wide(settingsState.wideEnabled);
    }
}

// How deep begin_eye_layer() calls can be nested
#define MAX_EYE_LAYERS 8

static float stereo_strength = 0.f;
static int current_eye = EYE_LEFT;
static bool drawing_top_eye = false;

static C3D_Mtx layer_views[MAX_EYE_LAYERS];
static int layer_count = 0;

// True only while the top screen is being drawn in 3D. Everything else bails out
// early, so 2D and the bottom screen render exactly like they did before
bool is_stereo_active() {
    return drawing_top_eye && stereo_strength > 0.f;
}

// How much this eye moves something floating at the given depth
float get_depth_shift(float depth) {
    return (current_eye == EYE_LEFT ? 1.f : -1.f) * depth * MAX_EYE_SHIFT * stereo_strength;
}

// Sets the top screen up for one eye, returns false once every eye has been drawn
bool begin_top_eye(int eye) {
    // The slider only gets read once per frame so both eyes agree on the depth
    if (eye == EYE_LEFT) {
        stereo_strength = top_right ? get_stereo_slider() : 0.f;
    }

    // One eye is enough with the slider down
    if (eye > (stereo_strength > 0.f ? EYE_RIGHT : EYE_LEFT)) {
        current_eye = EYE_LEFT;
        drawing_top_eye = false;
        return false;
    }

    current_eye = eye;
    drawing_top_eye = true;

    C3D_RenderTarget *target = (eye == EYE_RIGHT ? top_right : top);
    
    C2D_SceneBegin(target);
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 255));

    return true;
}

// True while redrawing the screen for another eye, so nothing gets updated twice
bool is_extra_eye() {
    return current_eye != EYE_LEFT;
}

// Shifts everything drawn after this to the given depth, pair it with end_eye_layer().
// Does nothing in 2D, touching the view would split the batch for no reason
void begin_eye_layer(float depth) {
    if (!is_stereo_active()) return;

    // Still count layers we have no room for, so the pairing keeps working
    if (layer_count++ >= MAX_EYE_LAYERS) return;

    C3D_Mtx *saved = &layer_views[layer_count - 1];
    C2D_ViewSave(saved);

    // The view can be scaled, so divide it out to keep the shift in screen pixels.
    // Whole pixels only, half pixel shifts get filtered and smear thin lines and text
    float scale = saved->r[0].x;
    if (scale != 0.f) C2D_ViewTranslate(roundf(get_depth_shift(depth)) / scale, 0);
}

void end_eye_layer() {
    if (!is_stereo_active()) return;

    if (layer_count <= 0) return;

    if (--layer_count < MAX_EYE_LAYERS) C2D_ViewRestore(&layer_views[layer_count]);
}

float get_mirror_x(float x, float factor) {
    if (factor == 0) return x;
    else if (factor == 1) return ((SCREEN_WIDTH / SCALE)) - x;
    
    return x + factor * ((SCREEN_WIDTH / SCALE) - 2.0f * x);
}

#include "c2d_internal.h"

bool C2D_DrawTriangleUV(float x0, float y0, float u0, float v0, u32 clr0, float x1, float y1, float u1, float v1, u32 clr1, float x2, float y2, float u2, float v2, u32 clr2, float depth, C2D_Image img) {
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

float calc_x_on_screen(float val) {
    return get_mirror_x(val - state.camera_x, state.mirror_factor); 
}

// Same as calc_x_on_screen but doesn't do the animation
float mirror_x_on_screen(float val) {
    return get_mirror_x(val - state.camera_x, state.mirror_factor >= 0.5f ? 1 : 0); 
}

float calc_y_on_screen(float val) {
    return SCREEN_HEIGHT - ((val - state.camera_y));
}


bool is_ccw(Vec2D *poly, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        sum += (poly[j].x - poly[i].x) * (poly[j].y + poly[i].y);
    }
    return sum < 0.0f; // CCW if sum < 0
}

void compute_mitered_offsets(Vec2D *poly, Vec2D *offsets, int n, float thickness, bool ccw) {
    for (int i = 0; i < n; i++) {
        int prev = (i + n - 1) % n;
        int next = (i + 1) % n;

        // Edge vectors
        Vec2D e_prev = { poly[i].x - poly[prev].x, poly[i].y - poly[prev].y };
        Vec2D e_next = { poly[next].x - poly[i].x, poly[next].y - poly[i].y };

        // Normals
        Vec2D n_prev = normalize(perpendicular(e_prev));
        Vec2D n_next = normalize(perpendicular(e_next));

        // Flip to point inward if mirror
        if (state.mirror_factor < 0.5f) { n_prev.x = -n_prev.x; n_prev.y = -n_prev.y; n_next.x = -n_next.x; n_next.y = -n_next.y; }

        // Miter = normalize(n_prev + n_next)
        Vec2D miter = normalize((Vec2D){ n_prev.x + n_next.x, n_prev.y + n_next.y });

        // Compute miter length to preserve thickness
        float cos_half_angle = dot_vec(miter, n_next); // cos(theta/2)
        float miter_length = thickness / cos_half_angle;

        offsets[i].x = poly[i].x + miter.x * miter_length;
        offsets[i].y = poly[i].y + miter.y * miter_length;
    }
}

void draw_polygon_inward_mitered(Vec2D *poly, int n, float thickness, u32 color) {
    bool ccw = is_ccw(poly, n);

    Vec2D offsets[n];
    compute_mitered_offsets(poly, offsets, n, thickness, ccw);

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
            x1, y1, color,
            x2, y2, color,
            ox2, oy2, color,
            0.0f
        );

        // Triangle 2
        C2D_DrawTriangle(
            ox2, oy2, color,
            ox1, oy1, color,
            x1, y1, color,
            0.0f
        );
    }
}

void draw_hitbox_line_inward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color) {

    float ex = x2 - x1;
    float ey = y2 - y1;
    float length = sqrtf(ex * ex + ey * ey);

    // Inward normal depends on winding
    float nx, ny;
    if (is_ccw(rect, 4)) {
        nx = -ey / length;
        ny =  ex / length;
    } else {
        nx =  ey / length;
        ny = -ex / length;
    }

    // Flip it to point inward if not mirror
    if (state.mirror_factor < 0.5f) {
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
        0.0f
    );

    // Second triangle
    C2D_DrawTriangle(
        x2b, y2b, color,
        x1b, y1b, color,
        x1a, y1a, color,
        0.0f
    );
}

void custom_ellipse(float x, float y, float radiusX,
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

void custom_circle (const float x, const float y, const float radius,
                     const u32 color) {
    custom_ellipse(x, y, radius, radius, color, true, 1);
}

void custom_circunference (const float x, const float y, const float radius,
                     const u32 color, const float lineWidth) {
    custom_ellipse(x, y, radius, radius, color, false, lineWidth);
}