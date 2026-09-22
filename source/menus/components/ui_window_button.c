#include "menus/components/ui_button.h"

#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "ui_window_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "menus/core/ui_screen.h"
#include "menus/settings_hub/settings.h"
#include "menus/gameplay.h"
#include "state.h"
#include "menus/core/ui_props.h"

#include "main.h"
#include "ui_slider.h"

//if a button has been pressed with a keybind, no other button should be pressed after
static int pressedKey;

void ui_window_button_set_tint(UIWindowButton* e, u32 color) {
    if (!e) return;

    e->color = color;
}

static void ui_window_button_update(UIElement* e, UIInput* touch, UITransform *transform) {
    ui_button_update(e, touch, transform);
}

static void ui_window_button_draw(UIElement* e, UITransform *transform) {
    UIWindowButton *window_button = (UIWindowButton *) e;

    draw_9_slice(window_button->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, window_button->border, window_button->color);

    ui_button_draw_text(e, transform);
}

static void ui_window_button_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

static void ui_window_button_on_disable(UIElement *e) {
    UIButton *button = (UIButton *) e;
    button->hovered = false;
    button->hoverProgress = 1.f;
    button->hoverTimer = 0.f;
}

void ui_window_button_set_style(UIWindowButton *e, int style) {
    if (!e) return;

    e->atlas = C2D_SpriteSheetGetImage(window_sheet, style);

    e->border = e->atlas.subtex->width / 3;
}

UIWindowButton *ui_create_window_button(UIScreen *screen) {
    UIWindowButton *e = malloc(sizeof(UIWindowButton));

    if (!e) return NULL;
    UIButton *button = (UIButton *) e;

    memset(e, 0, sizeof(UIWindowButton));

    button->base.type = UI_WINDOW_BUTTON;
    button->base.enabled = true;
    button->base.update = ui_window_button_update;
    button->base.draw = ui_window_button_draw;
    button->base.destroy = ui_window_button_destroy;
    
    button->base.modify_transform = ui_button_modify_transform;

    button->base.on_disable = ui_window_button_on_disable;

    ui_element_apply_default_properties(&button->base, screen);
    
    button->hoverProgress = 1;
    button->hoverFactor = 1;

    ui_window_button_set_tint(e, C2D_Color32(255, 255, 255, 255));

    pressedKey = false;

    return e;
}

UIElement *ui_create_window_button_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIWindowButton *window_button = ui_create_window_button(screen);

    if (!window_button) return NULL;
    
    UIButton *button = (UIButton *) window_button;

    ui_element_apply_properties(&button->base, screen, props);

    button->font = ui_prop_int(props, "font", 0);
    button->textScale = ui_prop_float(props, "textScale", 0);

    // Copy text
    strncpy(button->text, ui_prop_string(props, "text", ""), 63);

    ui_window_button_set_style(window_button, ui_prop_int(props, "style", 0));

    ui_window_button_set_tint(window_button, ui_prop_color(props, "color", ABGR8(255, 255, 255, 255)));

    button->hoverFactor = ui_prop_float(props, "hoverFactor", 1);

    button->keyBinds = ui_prop_bitfield(props, "keyBinds", keybind_table, ARRAY_LEN(keybind_table));

    return &button->base;
}