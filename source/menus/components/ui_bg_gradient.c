#include "menus/components/ui_image.h"
#include "menus/core/common_setters.h"

#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "ui_bg_gradient.h"
#include "math_helpers.h"
#include "menus/creator_menu/search_menu.h"

static void ui_bg_gradient_draw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;

    C2D_SpriteFromSheet(&image->image.sprite, bg_gradient_sheet, gdps ? 1 : 0);

    C2D_SpriteSetPos(&image->image.sprite, gdps ? 0.f : transform->x, gdps ? 0.f : transform->y);
    C2D_SpriteSetScale(&image->image.sprite, gdps ? 1.f : transform->scaleX, gdps ? 1.f : transform->scaleY);
    C2D_DrawSpriteTinted(&image->image.sprite, &image->image.tint);
}

static void ui_bg_gradient_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UIImage *ui_create_bg_gradient(UIScreen *screen) {
    UIImage *e = malloc(sizeof(UIImage));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIImage));
    e->base.type = UI_IMAGE;
    e->base.x = 0;
    e->base.y = 0;
    e->base.enabled = true;
    
    ui_element_apply_default_properties(&e->base, screen);

    C2D_SpriteFromSheet(&e->image.sprite, bg_gradient_sheet, 0);

    ui_element_set_scale_xy((UIElement *) e, BG_GRADIENT_XSCALE, BG_GRADIENT_YSCALE);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;

    ui_image_clear_tint(e);

    e->base.draw = ui_bg_gradient_draw;
    e->base.destroy = ui_bg_gradient_destroy;

    return e;
}

UIElement *ui_create_bg_gradient_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIImage *bg_gradient = ui_create_bg_gradient(screen);

    if (!bg_gradient) return NULL;
    
    ui_element_apply_properties(&bg_gradient->base, screen, props);

    // Those have to be hardcoded
    ui_element_set_position((UIElement *) bg_gradient, 0, 0);
    ui_element_set_scale_xy((UIElement *) bg_gradient, BG_GRADIENT_XSCALE, BG_GRADIENT_YSCALE);

    // Scale is hardcoded
    ui_image_set_tint(bg_gradient, ui_prop_color(props, "color", ABGR8(255, 255, 255, 255)));

    return &bg_gradient->base;
}