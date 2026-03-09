#include "gfx.h"
#include "math_helpers.h"
#include "graphics.h"
#include "main.h"

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
void draw_9_slice(const C2D_Image atlas, const float x, const float y, const int width, const int height, const float border, const u32 color) {
    // Set color
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color, 1.f);


    float cx = x;
    float cy = y;

    float w = width;
    float h = height;

    float b = border;

    if (w < 2*b) w = 2*b;
    if (h < 2*b) h = 2*b;

    float halfW = w * 0.5f;
    float halfH = h * 0.5f;

    float midW = w - 2*b;
    float midH = h - 2*b;

    C2D_Sprite spr;

    Tex3DS_SubTexture sub;
    C2D_Image img;

    // Corners

    C2D_SpriteSetScale(&spr, 1.0f, 1.0f);

    sub = TILE(0,0);
    img = atlas; img.subtex = &sub;
    C2D_SpriteFromImage(&spr, img);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,0);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Edges

    sub = TILE(1,0);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, 1.0f);
    C2D_SpriteSetPos(&spr, cx, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(1,2);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, 1.0f);
    C2D_SpriteSetPos(&spr, cx, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, 1.0f, midH / b);
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, 1.0f, midH / b);
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Center

    sub = TILE(1,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, midH / b);
    C2D_SpriteSetPos(&spr, cx, cy);
    C2D_DrawSpriteTinted(&spr, &tint);
}

#undef TILE

#define DISPLAY_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
                GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8))

C3D_RenderTarget* C2D_CreateScreenTargetExt(gfxScreen_t screen, gfx3dSide_t side, bool aa) {
    u8 model;
	CFGU_GetSystemModel(&model);
    bool wide = wideEnabled && model != CFG_MODEL_2DS && !is_citra() && screen == GFX_TOP;

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
        opacity += FADE_SPEED * (1.f/60);
        if (opacity >= 255) {
            opacity = 255;
            fade_status = FADE_STATUS_NONE;
        }
    } else if (fade_status == FADE_STATUS_IN) {
        opacity -= FADE_SPEED * (1.f/60);
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

void scale_view() {
    if (aaEnabled)
        C2D_ViewScale(2,2);
}

void reinitialize_screens() {
    C3D_RenderTargetDelete(top);
    C3D_RenderTargetDelete(bot);
    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, aaEnabled);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, aaEnabled);
}

void set_wide(bool wide) {
	u8 model;
	CFGU_GetSystemModel(&model);
	if (model != CFG_MODEL_2DS && !is_citra()) {
		wideEnabled = wide;
		gfxSetWide(wide);	
	}
}

void set_aa(bool aa) {
    aaEnabled = aa;
}