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
    (void)e;
}

UIRectangle *uiCreateRectangle(UIScreen *screen) {
    auto e = std::make_unique<UIRectangle>();

    UIRectangle *result = e.get();

    result->base.type = UIElementType::UiRectangle;
    result->base.enabled = true;
    
    uiElementApplyDefaultProperties(&result->base, screen);

    uiRectangleClearColor(result);

    result->base.draw = uiRectangleDraw;
    result->base.destroy = uiRectangleDestroy;

    return e.release();
}

UIElement *uiCreateRectangleFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIRectangle *rectangle = uiCreateRectangle(screen);

    if (!rectangle) return NULL;
    
    uiElementApplyProperties(&rectangle->base, screen, props);

    uiRectangleSetColor(rectangle, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    return &rectangle->base;
}

}