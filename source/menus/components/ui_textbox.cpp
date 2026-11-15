
#include <citro2d.h>
#include "ui_image.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "easing.hpp"
#include "utils/gfx.hpp"
#include "ui_checkbox.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "ui_textbox.hpp"

#include "utils/keyboard.hpp"

namespace gd3ds
{

static void uiTextboxUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UITextbox *textbox = (UITextbox *) e;

    bool inside = uiElementBasicBoundCheck(e, touch, transform);
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;

        if (hidKeysDown() & KEY_TOUCH) {
            readText(textbox->text, textbox->title, textbox->characterLimit);
            performActions(e);
        }
    }
}

static void uiTextboxDraw(UIElement* e, UITransform *transform) {
    UITextbox *textbox = (UITextbox *) e;

    draw9Slice(textbox->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, textbox->border, C2D_Color32(0, 0, 0, 127));
    
    // Get text length in pixels
    float length = getTextLength(&bigFontFontCharset, 1.f, false, textbox->text);

    // Resize it to fit the button bounds
    float txtScale;
    if (e->w - TEXTBOX_MARGIN < length) {
        txtScale = ((e->w - TEXTBOX_MARGIN) / length);
    } else {
        txtScale = 1.0f;
    }

    drawText(&bigFontFontCharset, &bigFontSheet, transform->x - ((e->w / 2) - (TEXTBOX_MARGIN / 2)) * transform->scaleX, transform->y, txtScale * transform->scaleX, txtScale * transform->scaleY, 0.f, false, "%s", textbox->text);
}

static void uiTextboxDestroy(UIElement *e) {
    (void)e;
}

UITextbox *uiCreateTextbox(UIScreen *screen) {
    auto e = std::make_unique<UITextbox>();

    UITextbox *result = e.get();

    result->base.type = UIElementType::UiTextbox;
    result->base.enabled = true;
    result->base.update = uiTextboxUpdate;
    result->base.draw = uiTextboxDraw;
    result->base.destroy = uiTextboxDestroy;
    
    uiElementApplyDefaultProperties(&result->base, screen);

    result->atlas = C2D_SpriteSheetGetImage(windowSheet, TEXTBOX_STYLE);
    result->border = result->atlas.subtex->width / 3;

    return e.release();
}

UIElement *uiCreateTextboxFromProps(UIScreen *screen, const UIPropertyList *props) {
    UITextbox *textbox = uiCreateTextbox(screen);

    if (!textbox) return NULL;

    uiElementApplyProperties(&textbox->base, screen, props);

    if (textbox->base.h == 0) {
        textbox->base.h = 30;
    }
    
    textbox->characterLimit = uiPropInt(props, "limit", 16);

    strncpy(textbox->title, uiPropString(props, "title", "Enter text:"), sizeof(textbox->title) - 1);

    return &textbox->base;
}

}