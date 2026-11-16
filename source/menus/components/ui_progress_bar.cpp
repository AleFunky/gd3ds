
#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "math_helpers.hpp"
#include "utils/gfx.hpp"

namespace gd3ds
{

static void drawFrame(UIProgressBar *e, UITransform *transform) {
    C2D_SpriteSetCenter(&e->frame.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->frame.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&e->frame.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&e->frame.sprite, &e->frame.tint);
}

static void drawBar(UIProgressBar *e, UITransform *transform) {
    float barWidth = e->bar.sprite.image.subtex->width;
    
    int pixels = (e->value / e->maxValue) * barWidth;

    if (pixels > 0) {
        if (pixels > barWidth) {
            pixels = barWidth;
        }

        C2D_Sprite spr;
        Tex3DS_SubTexture sub;
        C2D_Image img;

        float sx = transform->scaleX;
        float sy = transform->scaleY;

        float widthScaled = e->base.w * sx;
        float heightScaled = e->base.h * sy;

        if (e->style == 0) {
            // Add 1 pixel of margin
            sx *= ((float)(widthScaled - 2) / (float)widthScaled);
            sy *= ((float)(heightScaled - 2) / (float)heightScaled);
        }
        
        float x = transform->x - (widthScaled / 2);
        if (e->style == 0) {
            x += 2.f * transform->scaleX;
        }

        sub = selectBox(&e->bar.sprite.image, 0, 0, pixels, e->bar.sprite.image.subtex->height);
        img = e->bar.sprite.image; img.subtex = &sub;
        C2D_SpriteFromImage(&spr, img);
        C2D_SpriteSetCenter(&spr, 0.f, 0.5f);
        C2D_SpriteSetPos(&spr, x, transform->y);
        C2D_SpriteSetScale(&spr, sx, sy);
        C2D_DrawSpriteTinted(&spr, &e->bar.tint);
    }
}

static void uiProgressBarDraw(UIElement* e, UITransform *transform) {
    UIProgressBar *progressBar = (UIProgressBar *) e;
    if (progressBar->flipOrder) {
        drawBar(progressBar, transform);
        drawFrame(progressBar, transform);
    } else {
        drawFrame(progressBar, transform);
        drawBar(progressBar, transform);
    }
}

static void uiProgressBarDestroy(UIElement *e) {
    if (e) {
        delete (UIProgressBar *)e;
        e = NULL;
    }
}

void uiProgressBarSetTint(UIProgressBar* e, u32 color) {
    if (!e) return;

    C2D_PlainImageTint(&e->bar.tint, color, 1.0f);
}

void uiProgressBarClearTint(UIProgressBar* e) {
    if (!e) return;

    C2D_PlainImageTint(&e->bar.tint, 0xffffffff, 1.0f);
}

void uiProgressBarSetImages(UIProgressBar *e, int style) {
    switch (style) {
        case 0:
            C2D_SpriteFromSheet(&e->bar.sprite, barSheet, 0);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(0, 0, 0, 127), 1.0f);

            e->frame.sprite = e->bar.sprite;

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flipOrder = false;
            break;
        case 1:
            C2D_SpriteFromSheet(&e->bar.sprite, barSheet, 3);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

            C2D_SpriteFromSheet(&e->frame.sprite, barSheet, 2);
            C3D_TexSetFilter(e->frame.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(255, 255, 255, 255), 1.0f);

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flipOrder = true;
            break;
        case 2:
            C2D_SpriteFromSheet(&e->bar.sprite, barSheet, 3);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

            C2D_SpriteFromSheet(&e->frame.sprite, barSheet, 1);
            C3D_TexSetFilter(e->frame.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(255, 255, 255, 255), 1.0f);

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flipOrder = true;
            break;
    }
}

UIProgressBar *uiCreateProgressBar(UIScreen *screen) {
    UIProgressBar *e = new UIProgressBar{};

    if (!e) return NULL;

    e->base.type = UIElementType::UiProgressBar;
    e->base.enabled = true;
    e->useTint = false;

    uiElementApplyDefaultProperties(&e->base, screen);

    e->value = 0;
    e->maxValue = 100;

    e->base.draw = uiProgressBarDraw;
    e->base.destroy = uiProgressBarDestroy;

    return e;
}

UIElement *uiCreateProgressBarFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIProgressBar *progressBar = uiCreateProgressBar(screen);

    if (!progressBar) return NULL;

    uiElementApplyProperties(&progressBar->base, screen, props);
    
    progressBar->style = uiPropInt(props, "style", 0);

    progressBar->maxValue = uiPropFloat(props, "max_value", 100);

    // Please no divisions by zero, thanks
    if (progressBar->maxValue == 0) {
        progressBar->maxValue = 100;
    }

    uiProgressBarSetImages(progressBar, progressBar->style);

    uiProgressBarSetTint(progressBar, uiPropColor(props, "barColor", ABGR8(255, 255, 255, 255)));
    
    return &progressBar->base;
}

}