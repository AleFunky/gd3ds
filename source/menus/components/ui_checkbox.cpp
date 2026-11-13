
#include <citro2d.h>
#include "ui_button.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "easing.hpp"
#include "math_helpers.hpp"
#include "ui_checkbox.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"

#include "main.hpp"

namespace gd3ds
{

static void setCheckboxTexture(UICheckBox* e, bool enabled) {
    if (!e) return;
    UIButton *button = (UIButton *) e;
    int tex = enabled ? 28 : 27;
    C2D_SpriteFromSheet(&button->image.sprite, uiSheet, tex);
    C3D_TexSetFilter(button->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    button->base.w = button->image.sprite.image.subtex->width;
    button->base.h = button->image.sprite.image.subtex->height;
}

// Set checkbox checked state
void uiSetCheckboxChecked(UICheckBox *e, bool enabled) {
    if (!e) return;

    setCheckboxTexture(e, enabled);
    e->checked = enabled;
}

static void uiCheckboxUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    uiButtonUpdate(e, touch, transform);
}

static void uiCheckboxDraw(UIElement* e, UITransform *transform) {
    UIButton *checkbox = (UIButton *) e;

    C2D_SpriteSetCenter(&checkbox->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&checkbox->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&checkbox->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&checkbox->image.sprite);
}

static void uiCheckboxDestroy(UIElement *e) {
    if (e) {
        delete (UICheckBox *)e;
        e = NULL;
    }
}

static void uiCheckboxPreAction(UIElement *e, const UIPropertyList *args) {
    UICheckBox *checkbox = (UICheckBox *) e;

    checkbox->checked ^= 1;
    setCheckboxTexture(checkbox, checkbox->checked);
}

static void uiCheckboxOnDisable(UIElement *e) {
    UIButton *checkbox = (UIButton *) e;
    checkbox->hovered = false;
    checkbox->hoverProgress = 1.f;
    checkbox->hoverTimer = 0.f;
}

UICheckBox *uiCreateCheckbox(UIScreen *screen) {
    UICheckBox *e = new UICheckBox{};

    if (!e) return NULL;

    UIButton *button = (UIButton* ) e;
    
    button->base.type = UI_CHECKBOX;
    button->base.enabled = true;
    button->base.update = uiCheckboxUpdate;
    button->base.draw = uiCheckboxDraw;
    button->base.destroy = uiCheckboxDestroy;

    button->preAction = uiCheckboxPreAction;

    button->base.modifyTransform = uiButtonModifyTransform;

    button->base.onDisable = uiCheckboxOnDisable;

    uiElementApplyDefaultProperties(&button->base, screen);
    
    button->hoverProgress = 1;
    button->hoverFactor = 1;

    setCheckboxTexture(e, e->checked);

    return e;
}

UIElement *uiCreateCheckboxFromProps(UIScreen *screen, const UIPropertyList *props) {
    UICheckBox *checkbox = uiCreateCheckbox(screen);

    if (!checkbox) return NULL;

    UIButton *button = (UIButton* ) checkbox;

    uiElementApplyProperties(&button->base, screen, props);
    
    checkbox->checked = uiPropBool(props, "checked", false);
    
    button->hoverFactor = uiPropFloat(props, "hoverFactor", 1);

    setCheckboxTexture(checkbox, checkbox->checked);

    return &button->base;
}

}