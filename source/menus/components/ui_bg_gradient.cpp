#include "menus/components/ui_image.hpp"
#include "menus/core/common_setters.hpp"

#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "ui_bg_gradient.hpp"
#include "math_helpers.hpp"
#include "game/game_manager.hpp"

namespace gd3ds
{

static void uiBgGradientDraw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;

    bool gdps = GameManager::get()->platformState.isGDPS;

    C2D_SpriteFromSheet(&image->image.sprite, bgGradientSheet, gdps ? 1 : 0);

    C2D_SpriteSetPos(&image->image.sprite, gdps ? 0.f : transform->x, gdps ? 0.f : transform->y);
    C2D_SpriteSetScale(&image->image.sprite, gdps ? 1.f : transform->scaleX, gdps ? 1.f : transform->scaleY);
    C2D_DrawSpriteTinted(&image->image.sprite, &image->image.tint);
}

static void uiBgGradientDestroy(UIElement *e) {
    (void)e;
}

UIImage *uiCreateBgGradient(UIScreen *screen) {
    auto e = std::make_unique<UIImage>();

    UIImage *result = e.get();

    result->base.type = UIElementType::UiImage;
    result->base.x = 0;
    result->base.y = 0;
    result->base.enabled = true;
    
    uiElementApplyDefaultProperties(&result->base, screen);

    C2D_SpriteFromSheet(&result->image.sprite, bgGradientSheet, 0);

    uiElementSetScaleXy((UIElement *) result, BG_GRADIENT_XSCALE, BG_GRADIENT_YSCALE);

    result->base.w = result->image.sprite.image.subtex->width;
    result->base.h = result->image.sprite.image.subtex->height;

    uiImageClearTint(result);

    result->base.draw = uiBgGradientDraw;
    result->base.destroy = uiBgGradientDestroy;

    return e.release();
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