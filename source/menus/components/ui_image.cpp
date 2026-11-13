#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "math_helpers.hpp"

namespace gd3ds
{

void uiImageDraw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;

    C2D_SpriteSetCenter(&image->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&image->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&image->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&image->image.sprite, &image->image.tint);
}

static void uiImageDestroy(UIElement* e) {
    if (e) {
        delete (UIImage *)e;
        e = NULL;
    }
}

void uiImageSetTint(UIImage* e, u32 color) {
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, color, 1.0f);
}

void uiImageClearTint(UIImage* e) {
    if (!e) return;
    
    C2D_PlainImageTint(&e->image.tint, 0xffffffff, 1.0f);
}

void uiImageSetImage(UIImage *e, int spriteIndex, int sheet) {
    if (!e) return;

    C2D_SpriteFromSheet(&e->image.sprite, getSheet(sheet), spriteIndex);
    C3D_TexSetFilter(e->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;
}

UIImage *uiCreateImage(UIScreen *screen) {
    UIImage *e = new UIImage{};

    if (!e) return NULL;

    e->base.type = UI_IMAGE;
    e->base.enabled = true;

    uiElementApplyDefaultProperties(&e->base, screen);

    uiImageClearTint(e);

    e->base.draw = uiImageDraw;
    e->base.destroy = uiImageDestroy;

    return e;
}

UIElement *uiCreateImageFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIImage *image = uiCreateImage(screen);
    
    if (!image) return NULL;

    uiElementApplyProperties(&image->base, screen, props);

    uiImageSetImage(image, 
        uiPropInt(props, "id", 0),
        uiPropInt(props, "sheet", 0)
    );
    
    
    uiImageSetTint(image, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    return &image->base;
}

}