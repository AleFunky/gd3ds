#include <citro2d.h>
#include "menus/core/ui_props.hpp"
#include "ui_image.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "easing.hpp"
#include "utils/gfx.hpp"
#include "ui_checkbox.hpp"
#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"

namespace gd3ds
{

void uiWindowSetTint(UIWindow* e, u32 color) {
    if (!e) return;

    e->color = color;
}

void uiWindowSetAtlas(UIWindow* e, int index) {
    if (!e) return;

    e->atlas = C2D_SpriteSheetGetImage(windowSheet, index);
    if (e->atlas.tex) {
        e->border = e->atlas.subtex->width / 3;
    }
}

static void uiWindowDraw(UIElement* e, UITransform *transform) {
    UIWindow *window = (UIWindow *) e;

    draw9Slice(window->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, window->border, window->color);
}

static void uiWindowDestroy(UIElement *e) {
    (void)e;
}

UIWindow *uiCreateWindow(UIScreen *screen) {
    auto e = std::make_unique<UIWindow>();

    UIWindow *result = e.get();

    result->base.type = UIElementType::UiWindow;
    result->base.enabled = true;

    result->base.draw = uiWindowDraw;
    result->base.destroy = uiWindowDestroy;
    
    uiElementApplyDefaultProperties(&result->base, screen);

    uiWindowSetAtlas(result, 0);
    uiWindowSetTint(result, C2D_Color32(255, 255, 255, 255));

    return e.release();
}

UIElement *uiCreateWindowFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIWindow *window = uiCreateWindow(screen);

    if (!window) return NULL;

    uiElementApplyProperties(&window->base, screen, props);

    uiWindowSetAtlas(window, uiPropInt(props, "style", 0));

    uiWindowSetTint(window, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    return &window->base;
}

}