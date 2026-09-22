#include "c2d/base.h"

#include <citro2d.h>
#include "menus/core/ui_props.h"
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "utils/gfx.h"
#include "ui_checkbox.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"

void ui_window_set_tint(UIWindow* e, u32 color) {
    if (!e) return;

    e->color = color;
}

void ui_window_set_atlas(UIWindow* e, int index) {
    if (!e) return;

    e->atlas = C2D_SpriteSheetGetImage(window_sheet, index);
    if (e->atlas.tex) {
        e->border = e->atlas.subtex->width / 3;
    }
}

static void ui_window_draw(UIElement* e, UITransform *transform) {
    UIWindow *window = (UIWindow *) e;

    draw_9_slice(window->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, window->border, window->color);
}

static void ui_window_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UIWindow *ui_create_window(UIScreen *screen) {
    UIWindow *e = malloc(sizeof(UIWindow));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIWindow));
    e->base.type = UI_WINDOW;
    e->base.enabled = true;

    e->base.draw = ui_window_draw;
    e->base.destroy = ui_window_destroy;
    
    ui_element_apply_default_properties(&e->base, screen);

    ui_window_set_atlas(e, 0);
    ui_window_set_tint(e, C2D_Color32(255, 255, 255, 255));

    return e;
}

UIElement *ui_create_window_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIWindow *window = ui_create_window(screen);

    if (!window) return NULL;

    ui_element_apply_properties(&window->base, screen, props);

    ui_window_set_atlas(window, ui_prop_int(props, "style", 0));

    ui_window_set_tint(window, ui_prop_color(props, "color", ABGR8(255, 255, 255, 255)));

    return &window->base;
}