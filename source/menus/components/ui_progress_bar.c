
#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "math_helpers.h"

static void draw_frame(UIProgressBar *e, UITransform *transform) {
    C2D_SpriteSetCenter(&e->frame.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->frame.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&e->frame.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&e->frame.sprite, &e->frame.tint);
}

static void draw_bar(UIProgressBar *e, UITransform *transform) {
    float bar_width = e->bar.sprite.image.subtex->width;
    
    int pixels = (e->value / e->max_value) * bar_width;

    if (pixels > 0) {
        if (pixels > bar_width) {
            pixels = bar_width;
        }

        C2D_Sprite spr;
        Tex3DS_SubTexture sub;
        C2D_Image img;

        float sx = transform->scaleX;
        float sy = transform->scaleY;

        float width_scaled = e->base.w * sx;
        float height_scaled = e->base.h * sy;

        if (e->style == 0) {
            // Add 1 pixel of margin
            sx *= ((float)(width_scaled - 2) / (float)width_scaled);
            sy *= ((float)(height_scaled - 2) / (float)height_scaled);
        }
        
        float x = transform->x - (width_scaled / 2);
        if (e->style == 0) {
            x += 2.f * transform->scaleX;
        }

        sub = select_box(&e->bar.sprite.image, 0, 0, pixels, e->bar.sprite.image.subtex->height);
        img = e->bar.sprite.image; img.subtex = &sub;
        C2D_SpriteFromImage(&spr, img);
        C2D_SpriteSetCenter(&spr, 0.f, 0.5f);
        C2D_SpriteSetPos(&spr, x, transform->y);
        C2D_SpriteSetScale(&spr, sx, sy);
        C2D_DrawSpriteTinted(&spr, &e->bar.tint);
    }
}

static void ui_progress_bar_draw(UIElement* e, UITransform *transform) {
    UIProgressBar *progress_bar = (UIProgressBar *) e;
    if (progress_bar->flip_order) {
        draw_bar(progress_bar, transform);
        draw_frame(progress_bar, transform);
    } else {
        draw_frame(progress_bar, transform);
        draw_bar(progress_bar, transform);
    }
}

static void ui_progress_bar_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

void ui_progress_bar_set_tint(UIProgressBar* e, u32 color) {
    if (!e) return;

    C2D_PlainImageTint(&e->bar.tint, color, 1.0f);
}

void ui_progress_bar_clear_tint(UIProgressBar* e) {
    if (!e) return;

    C2D_PlainImageTint(&e->bar.tint, 0xffffffff, 1.0f);
}

void ui_progress_bar_set_images(UIProgressBar *e, int style) {
    switch (style) {
        case 0:
            C2D_SpriteFromSheet(&e->bar.sprite, bar_sheet, 0);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(0, 0, 0, 127), 1.0f);

            e->frame.sprite = e->bar.sprite;

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flip_order = false;
            break;
        case 1:
            C2D_SpriteFromSheet(&e->bar.sprite, bar_sheet, 3);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

            C2D_SpriteFromSheet(&e->frame.sprite, bar_sheet, 2);
            C3D_TexSetFilter(e->frame.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(255, 255, 255, 255), 1.0f);

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flip_order = true;
            break;
        case 2:
            C2D_SpriteFromSheet(&e->bar.sprite, bar_sheet, 3);
            C3D_TexSetFilter(e->bar.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

            C2D_SpriteFromSheet(&e->frame.sprite, bar_sheet, 1);
            C3D_TexSetFilter(e->frame.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
            
            C2D_PlainImageTint(&e->frame.tint, C2D_Color32(255, 255, 255, 255), 1.0f);

            e->base.w = e->bar.sprite.image.subtex->width;
            e->base.h = e->bar.sprite.image.subtex->height;

            e->flip_order = true;
            break;
    }
}

UIProgressBar *ui_create_progress_bar(UIScreen *screen) {
    UIProgressBar *e = malloc(sizeof(UIProgressBar));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIProgressBar));
    e->base.type = UI_PROGRESS_BAR;
    e->base.enabled = true;
    e->useTint = false;

    ui_element_apply_default_properties(&e->base, screen);

    e->value = 0;
    e->max_value = 100;

    e->base.draw = ui_progress_bar_draw;
    e->base.destroy = ui_progress_bar_destroy;

    return e;
}

UIElement *ui_create_progress_bar_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIProgressBar *progress_bar = ui_create_progress_bar(screen);

    if (!progress_bar) return NULL;

    ui_element_apply_properties(&progress_bar->base, screen, props);
    
    progress_bar->style = ui_prop_int(props, "style", 0);

    progress_bar->max_value = ui_prop_float(props, "max_value", 100);

    // Please no divisions by zero, thanks
    if (progress_bar->max_value == 0) {
        progress_bar->max_value = 100;
    }

    ui_progress_bar_set_images(progress_bar, progress_bar->style);

    ui_progress_bar_set_tint(progress_bar, ui_prop_color(props, "barColor", ABGR8(255, 255, 255, 255)));
    
    return &progress_bar->base;
}