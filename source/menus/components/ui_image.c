#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "math_helpers.h"

void ui_image_update(UIElement* e, UIInput* touch, UITransform *transform) {
    bool inside = ui_element_basic_bound_check(e, touch, transform);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

void ui_image_draw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;

    C2D_SpriteSetCenter(&image->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&image->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&image->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&image->image.sprite, &image->image.tint);
}

static void ui_image_destroy(UIElement* e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

void ui_image_set_tint(UIImage* e, u32 color) {
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, color, 1.0f);
}

void ui_image_clear_tint(UIImage* e) {
    if (!e) return;
    
    C2D_PlainImageTint(&e->image.tint, 0xffffffff, 1.0f);
}

void ui_image_set_image(UIImage *e, int sprite_index, int sheet) {
    if (!e) return;

    C2D_SpriteFromSheet(&e->image.sprite, *get_sheet(sheet), sprite_index);
    C3D_TexSetFilter(e->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;
}

UIImage *ui_create_image(const UIContext *ctx) {
    UIImage *e = malloc(sizeof(UIImage));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIImage));
    e->base.type = UI_IMAGE;
    e->base.enabled = true;

    ui_element_apply_default_properties(&e->base, ctx);

    ui_image_clear_tint(e);

    e->base.update = ui_image_update;
    e->base.draw = ui_image_draw;
    e->base.destroy = ui_image_destroy;

    return e;
}

UIElement *ui_create_image_from_props(const UIContext *ctx, const UIPropertyList *props) {
    UIImage *image = ui_create_image(ctx);
    
    if (!image) return NULL;

    ui_element_apply_properties(&image->base, ctx, props);

    ui_image_set_image(image, 
        ui_prop_int(props, "id", 0),
        ui_prop_int(props, "sheet", 0)
    );
    
    
    ui_image_set_tint(image, ui_prop_color(props, "color", ABGR8(255, 255, 255, 255)));

    return &image->base;
}