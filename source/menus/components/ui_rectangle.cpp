#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "math_helpers.hpp"

namespace gd3ds
{

void uiRectangleSetColor(UIRectangle* e, u32 color) {
    if (!e) return;

    e->color = color;
}

void uiRectangleClearColor(UIRectangle* e) {
    if (!e) return;
    
    e->color = 0xffffffff;
}

static void uiRectangleDraw(UIElement* e, UITransform *transform) {
    UIRectangle *rectangle = (UIRectangle *) e;

    float width = e->w * transform->scaleX;
    float height = e->h * transform->scaleY;

    C2D_DrawRectangle(transform->x - width * 0.5f, transform->y - height * 0.5f, 0, width, height, rectangle->color, rectangle->color, rectangle->color, rectangle->color);
}

static void uiRectangleDestroy(UIElement *e) {
    if (e) {
        delete (UIRectangle *)e;
        e = NULL;
    }
}

UIRectangle *uiCreateRectangle(UIScreen *screen) {
    UIRectangle *e = new UIRectangle{};

    if (!e) return NULL;

    e->base.type = UIElementType::UiRectangle;
    e->base.enabled = true;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    uiRectangleClearColor(e);

    e->base.draw = uiRectangleDraw;
    e->base.destroy = uiRectangleDestroy;

    return e;
}

UIElement *uiCreateRectangleFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIRectangle *rectangle = uiCreateRectangle(screen);

    if (!rectangle) return NULL;
    
    uiElementApplyProperties(&rectangle->base, screen, props);

    uiRectangleSetColor(rectangle, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    return &rectangle->base;
}

}