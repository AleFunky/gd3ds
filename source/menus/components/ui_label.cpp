
#include <citro2d.h>
#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"

namespace gd3ds
{

static const UIFloatEnumEntry alignmentTable[] = {
    { "LEFT",   0.f },
    { "CENTER", 0.5f },
    { "RIGHT",  1.f }
};

static void uiLabelDraw(UIElement* e, UITransform *transform) {
    UILabel *label = (UILabel *) e;
    int fontID = label->font;

    // Set to pusab if invalid
    if (fontID >= static_cast<int>(Fonts::NumFonts)) fontID = 0;

    const LabelFont *font = &fonts[fontID];

    float scale = transform->scaleX;

    int width = e->w;

    if (width > 0){
        float length = getLongestLineLength(font->charset, e->scaleX, label->text);

        if (width < length && length > 0) {
            scale *= (width / length);
        }
    }

    drawText(font->charset, font->sheet, transform->x, transform->y, scale, scale, label->alignment, label->parseTags, "%s", label->text);
}

static void uiLabelDestroy(UIElement* e) {
    if (e) {
        delete (UILabel *)e;
        e = NULL;
    }
}

void uiLabelSetText(UILabel *e, const char *text) {
    if (!e || !text) return;

    strncpy(e->text, text, sizeof(e->text) - 1);
}

UILabel *uiCreateLabel(UIScreen *screen) {
    UILabel *e = new UILabel{};

    if (!e) return NULL;

    e->base.type = UIElementType::UiLabel;
    e->base.enabled = true;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    e->parseTags = true;

    e->base.draw = uiLabelDraw;
    e->base.destroy = uiLabelDestroy;

    return e;
}

UIElement *uiCreateLabelFromProps(UIScreen *screen, const UIPropertyList *props) {
    UILabel *label = uiCreateLabel(screen);

    if (!label) return NULL;

    uiElementApplyProperties(&label->base, screen, props);

    uiLabelSetText(label, uiPropString(props, "text", ""));
    
    label->font = uiPropInt(props, "font", 0);
    label->alignment = uiPropFloatEnum(props, "align", alignmentTable, ARRAY_LEN(alignmentTable), 0);
    label->parseTags = uiPropBool(props, "parseTags", true);

    return &label->base;
}

}