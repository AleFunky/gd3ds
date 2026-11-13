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
    if (e) {
        delete (UIWindow *)e;
        e = NULL;
    }
}

UIWindow *uiCreateWindow(UIScreen *screen) {
    UIWindow *e = new UIWindow{};

    if (!e) return NULL;

    e->base.type = UI_WINDOW;
    e->base.enabled = true;

    e->base.draw = uiWindowDraw;
    e->base.destroy = uiWindowDestroy;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    uiWindowSetAtlas(e, 0);
    uiWindowSetTint(e, C2D_Color32(255, 255, 255, 255));

    return e;
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