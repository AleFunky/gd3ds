#include "menus/components/ui_image.hpp"
#include "menus/core/common_setters.hpp"

#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "ui_bg_gradient.hpp"
#include "math_helpers.hpp"
#include "menus/creator_menu/search_menu.hpp"

namespace gd3ds
{

static void uiBgGradientDraw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;

    C2D_SpriteFromSheet(&image->image.sprite, bgGradientSheet, gdps ? 1 : 0);

    C2D_SpriteSetPos(&image->image.sprite, gdps ? 0.f : transform->x, gdps ? 0.f : transform->y);
    C2D_SpriteSetScale(&image->image.sprite, gdps ? 1.f : transform->scaleX, gdps ? 1.f : transform->scaleY);
    C2D_DrawSpriteTinted(&image->image.sprite, &image->image.tint);
}

static void uiBgGradientDestroy(UIElement *e) {
    if (e) {
        delete (UIImage *)e;
        e = NULL;
    }
}

UIImage *uiCreateBgGradient(UIScreen *screen) {
    UIImage *e = new UIImage{};

    if (!e) return NULL;

    e->base.type = UI_IMAGE;
    e->base.x = 0;
    e->base.y = 0;
    e->base.enabled = true;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    C2D_SpriteFromSheet(&e->image.sprite, bgGradientSheet, 0);

    uiElementSetScaleXy((UIElement *) e, BG_GRADIENT_XSCALE, BG_GRADIENT_YSCALE);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;

    uiImageClearTint(e);

    e->base.draw = uiBgGradientDraw;
    e->base.destroy = uiBgGradientDestroy;

    return e;
}

UIElement *uiCreateBgGradientFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIImage *bgGradient = uiCreateBgGradient(screen);

    if (!bgGradient) return NULL;
    
    uiElementApplyProperties(&bgGradient->base, screen, props);

    // Those have to be hardcoded
    uiElementSetPosition((UIElement *) bgGradient, 0, 0);
    uiElementSetScaleXy((UIElement *) bgGradient, BG_GRADIENT_XSCALE, BG_GRADIENT_YSCALE);

    // Scale is hardcoded
    uiImageSetTint(bgGradient, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    return &bgGradient->base;
}

}