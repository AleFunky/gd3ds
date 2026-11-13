#include "menus/components/ui_button.hpp"

#include <citro2d.h>
#include "ui_window_button.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"

#include "main.hpp"
#include "utils/gfx.hpp"

//if a button has been pressed with a keybind, no other button should be pressed after
static int pressedKey;

void uiWindowButtonSetTint(UIWindowButton* e, u32 color) {
    if (!e) return;

    e->color = color;
}

static void uiWindowButtonUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    uiButtonUpdate(e, touch, transform);
}

static void uiWindowButtonDraw(UIElement* e, UITransform *transform) {
    UIWindowButton *windowButton = (UIWindowButton *) e;

    draw9Slice(windowButton->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, windowButton->border, windowButton->color);

    uiButtonDrawText(e, transform);
}

static void uiWindowButtonDestroy(UIElement *e) {
    if (e) {
        delete (UIWindowButton *)e;
        e = NULL;
    }
}

static void uiWindowButtonOnDisable(UIElement *e) {
    UIButton *button = (UIButton *) e;
    button->hovered = false;
    button->hoverProgress = 1.f;
    button->hoverTimer = 0.f;
}

void uiWindowButtonSetStyle(UIWindowButton *e, int style) {
    if (!e) return;

    e->atlas = C2D_SpriteSheetGetImage(windowSheet, style);

    e->border = e->atlas.subtex->width / 3;
}

UIWindowButton *uiCreateWindowButton(UIScreen *screen) {
    UIWindowButton *e = new UIWindowButton{};

    if (!e) return NULL;
    UIButton *button = (UIButton *) e;

    button->base.type = UI_WINDOW_BUTTON;
    button->base.enabled = true;
    button->base.update = uiWindowButtonUpdate;
    button->base.draw = uiWindowButtonDraw;
    button->base.destroy = uiWindowButtonDestroy;
    
    button->base.modifyTransform = uiButtonModifyTransform;

    button->base.onDisable = uiWindowButtonOnDisable;

    uiElementApplyDefaultProperties(&button->base, screen);
    
    button->hoverProgress = 1;
    button->hoverFactor = 1;

    uiWindowButtonSetTint(e, C2D_Color32(255, 255, 255, 255));

    pressedKey = false;

    return e;
}

UIElement *uiCreateWindowButtonFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIWindowButton *windowButton = uiCreateWindowButton(screen);

    if (!windowButton) return NULL;
    
    UIButton *button = (UIButton *) windowButton;

    uiElementApplyProperties(&button->base, screen, props);

    button->font = uiPropInt(props, "font", 0);
    button->textScale = uiPropFloat(props, "textScale", 0);

    // Copy text
    strncpy(button->text, uiPropString(props, "text", ""), 63);

    uiWindowButtonSetStyle(windowButton, uiPropInt(props, "style", 0));

    uiWindowButtonSetTint(windowButton, uiPropColor(props, "color", ABGR8(255, 255, 255, 255)));

    button->hoverFactor = uiPropFloat(props, "hoverFactor", 1);

    button->keyBinds = uiPropBitfield(props, "keyBinds", keybindTable, ARRAY_LEN(keybindTable));

    return &button->base;
}