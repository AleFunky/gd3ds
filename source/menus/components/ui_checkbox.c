#include "menus/core/ui_element.h"
#include <citro2d.h>
#include "ui_button.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_checkbox.h"
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"

#include "main.h"

static void set_checkbox_texture(UICheckBox* e, bool enabled) {
    if (!e) return;
    UIButton *button = (UIButton *) e;
    int tex = enabled ? 28 : 27;
    C2D_SpriteFromSheet(&button->image.sprite, ui_sheet, tex);
    C3D_TexSetFilter(button->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    button->base.w = button->image.sprite.image.subtex->width;
    button->base.h = button->image.sprite.image.subtex->height;
}

// Set checkbox checked state
void ui_set_checkbox_checked(UICheckBox *e, bool enabled) {
    if (!e) return;

    set_checkbox_texture(e, enabled);
    e->checked = enabled;
}

static void ui_checkbox_update(UIElement* e, UIInput* touch, UITransform *transform) {
    ui_button_update(e, touch, transform);
}

static void ui_checkbox_draw(UIElement* e, UITransform *transform) {
    UIButton *checkbox = (UIButton *) e;

    C2D_SpriteSetCenter(&checkbox->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&checkbox->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&checkbox->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&checkbox->image.sprite);
}

static void ui_checkbox_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

static void ui_checkbox_pre_action(UIElement *e) {
    UICheckBox *checkbox = (UICheckBox *) e;

    checkbox->checked ^= 1;
    set_checkbox_texture(checkbox, checkbox->checked);
}

static void ui_checkbox_on_disable(UIElement *e) {
    UIButton *checkbox = (UIButton *) e;
    checkbox->hovered = false;
    checkbox->hoverScale = 1.f;
    checkbox->hoverTimer = 0.f;
}

UICheckBox *ui_create_checkbox(const UIContext *ctx) {
    UICheckBox *e = malloc(sizeof(UICheckBox));

    if (!e) return NULL;

    UIButton *button = (UIButton* ) e;
    
    memset(e, 0, sizeof(UICheckBox));
    button->base.type = UI_CHECKBOX;
    button->base.enabled = true;
    button->base.update = ui_checkbox_update;
    button->base.draw = ui_checkbox_draw;
    button->base.destroy = ui_checkbox_destroy;

    button->pre_action = ui_checkbox_pre_action;

    button->base.modify_transform = ui_button_modify_transform;

    button->base.on_disable = ui_checkbox_on_disable;

    ui_element_apply_default_properties(&button->base, ctx);
    
    button->hoverScale = 1;
    button->hoverFactor = 1;

    set_checkbox_texture(e, e->checked);

    return e;
}

UIElement *ui_create_checkbox_from_props(const UIContext *ctx, const UIPropertyList *props) {
    UICheckBox *checkbox = ui_create_checkbox(ctx);

    if (!checkbox) return NULL;

    UIButton *button = (UIButton* ) checkbox;

    ui_element_apply_properties(&button->base, ctx, props);
    
    checkbox->checked = ui_prop_bool(props, "checked", false);
    
    button->hoverFactor = ui_prop_float(props, "hoverFactor", 1);

    set_checkbox_texture(checkbox, checkbox->checked);

    return &button->base;
}