#include "ui_spinner.h"
#include "c2d/sprite.h"
#include "c3d/maths.h"
#include "graphics.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_props.h"
#include "menus/core/ui_screen.h"
#include "ui_image.h"
#include "math_helpers.h"

static void ui_spinner_update(UIElement* e, UIInput* touch, UITransform *transform) {
    ui_image_update(e, touch, transform);
}

static void ui_spinner_draw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;
    UISpinner *spinner = (UISpinner *) image;
    
    C2D_SpriteRotate(&image->image.sprite, C3D_AngleFromDegrees(spinner->rotation_speed * DT)); // Cloud pls save us all and do screen rewrite
    
    change_blending(spinner->blending);
    ui_image_draw(e, transform);
    change_blending(false);
}

void ui_spinner_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}


UISpinner *ui_create_spinner(const UIContext *ctx) {
    UISpinner *e = malloc(sizeof(UISpinner));

    if (!e) return NULL;

    UIImage *image = (UIImage *) e;

    memset(e, 0, sizeof(UIImage));
    image->base.type = UI_IMAGE;
    image->base.enabled = true;

    ui_element_apply_default_properties(&image->base, ctx);

    ui_image_clear_tint(image);
    ui_image_set_image(image, 53, 1);

    e->blending = true;

    image->base.update = ui_spinner_update;
    image->base.draw = ui_spinner_draw;
    image->base.destroy = ui_spinner_destroy;

    return e;
}

UIElement *ui_create_spinner_from_props(const UIContext *ctx, const UIPropertyList *props) {
    UISpinner *spinner = ui_create_spinner(ctx);
    
    if (!spinner) return NULL;

    UIImage *image = (UIImage *) spinner;

    ui_element_apply_properties(&image->base, ctx, props);

    ui_image_set_image(image, 
        ui_prop_int(props, "id", 53),
        ui_prop_int(props, "sheet", 1)
    );
    
    ui_image_set_tint(image, ui_prop_color(props, "color", ABGR8(255, 255, 255, 200)));

    spinner->rotation_speed = ui_prop_float(props, "speed", 360);
    spinner->blending = ui_prop_bool(props, "blending", true);

    return &image->base;
}