#include "menus/components/ui_button.hpp"

#include <citro2d.h>
#include "ui_color_button.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "graphics.hpp"

#include "menus/palette_kit.hpp"

#include "main.hpp"
#include "ui_slider.hpp"

namespace gd3ds
{

static void uiColorButtonUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    uiButtonUpdate(e, touch, transform);
}

static void uiColorButtonDraw(UIElement* e, UITransform *transform) {
    UIColor *color = (UIColor *) e;
    UIButton *button = (UIButton *) e;

    float scale = button->hoverProgress;

    size_t colorIdx = color->colorIndex;

    if (colorIdx >= numColors) colorIdx = numColors - 1;

    C2D_ImageTint tint = { 0 };
    C2D_PlainImageTint(&tint, colors[colorIdx], 1.0f);
    
    C2D_SpriteSetCenter(&button->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&button->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&button->image.sprite, transform->scaleX * scale, transform->scaleY * scale);
    C2D_DrawSpriteTinted(&button->image.sprite, &tint);

    if (color->isSelected) {
        C2D_SpriteSetCenter(&color->image.sprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&color->image.sprite, transform->x, transform->y);
        C2D_SpriteSetScale(&color->image.sprite, transform->scaleX + 0.05f, transform->scaleY + 0.05f);
        C2D_DrawSprite(&color->image.sprite);
    }
}

void uiColorButtonSetIndex(UIColor *e, int index, int colorIndex) {
    if (!e) return;
    e->index = index;
    e->colorIndex = colorIndex;
}

static void uiColorButtonDestroy(UIElement *e) {
    if (e) {
        delete (UIColor *)e;
        e = NULL;
    }
}

UIColor *uiCreateColorButton(UIScreen *screen) {
    UIColor *e = new UIColor{};

    if (!e) return NULL;
    
    UIButton *button = (UIButton *) e;

    button->base.type = UIElementType::UiColorButton;
    button->base.enabled = true;

    button->base.update = uiColorButtonUpdate;
    button->base.draw = uiColorButtonDraw;
    button->base.destroy = uiColorButtonDestroy;
    
    uiElementApplyDefaultProperties(&button->base, screen);
    
    button->hoverProgress = 1;
    button->hoverFactor = 1;

    C2D_SpriteFromSheet(&e->image.sprite, uiSheet, 175);
    C3D_TexSetFilter(e->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
    
    C2D_SpriteFromSheet(&button->image.sprite, uiSheet, 35);
    C3D_TexSetFilter(button->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    return e;
}

UIElement *uiCreateColorButtonFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIColor *colorButton = uiCreateColorButton(screen);

    if (!colorButton) return NULL;
    
    UIButton *button = (UIButton *) colorButton;

    uiElementApplyProperties(&button->base, screen, props);
    
    uiElementSetSize(&button->base, 30, 30);

    button->hoverFactor = uiPropFloat(props, "hoverFactor", 1);

    uiColorButtonSetIndex(colorButton,  
        uiPropInt(props, "id", 0), 
        uiPropInt(props, "color_index", 0));

    return &button->base;
}

}