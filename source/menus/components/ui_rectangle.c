#include "c2d/base.h"

#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "math_helpers.h"

void ui_rectangle_set_color(UIRectangle* e, u32 color) {
    if (!e) return;

    e->color = color;
}

void ui_rectangle_clear_color(UIRectangle* e) {
    if (!e) return;
    
    e->color = 0xffffffff;
}

static void ui_rectangle_draw(UIElement* e, UITransform *transform) {
    UIRectangle *rectangle = (UIRectangle *) e;

    float width = e->w * transform->scaleX;
    float height = e->h * transform->scaleY;

    C2D_DrawRectangle(transform->x - width * 0.5f, transform->y - height * 0.5f, 0, width, height, rectangle->color, rectangle->color, rectangle->color, rectangle->color);
}

static void ui_rectangle_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UIRectangle *ui_create_rectangle(UIScreen *screen) {
    UIRectangle *e = malloc(sizeof(UIRectangle));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIRectangle));
    e->base.type = UI_RECTANGLE;
    e->base.enabled = true;
    
    ui_element_apply_default_properties(&e->base, screen);

    ui_rectangle_clear_color(e);

    e->base.draw = ui_rectangle_draw;
    e->base.destroy = ui_rectangle_destroy;

    return e;
}

UIElement *ui_create_rectangle_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIRectangle *rectangle = ui_create_rectangle(screen);

    if (!rectangle) return NULL;
    
    ui_element_apply_properties(&rectangle->base, screen, props);

    ui_rectangle_set_color(rectangle, ui_prop_color(props, "color", ABGR8(255, 255, 255, 255)));

    return &rectangle->base;
}