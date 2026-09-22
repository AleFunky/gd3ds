
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "utils/gfx.h"
#include "ui_checkbox.h"
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "ui_textbox.h"

#include "utils/keyboard.h"

static void ui_textbox_update(UIElement* e, UIInput* touch, UITransform *transform) {
    UITextbox *textbox = (UITextbox *) e;

    bool inside = ui_element_basic_bound_check(e, touch, transform);
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;

        if (hidKeysDown() & KEY_TOUCH) {
            read_text(textbox->text, textbox->title, textbox->character_limit);
            perform_actions(e);
        }
    }
}

static void ui_textbox_draw(UIElement* e, UITransform *transform) {
    UITextbox *textbox = (UITextbox *) e;

    draw_9_slice(textbox->atlas, transform->x, transform->y, transform->scaleX, transform->scaleY, e->w, e->h, textbox->border, C2D_Color32(0, 0, 0, 127));
    
    // Get text length in pixels
    float length = get_text_length(&bigFont_fontCharset, 1.f, false, textbox->text);

    // Resize it to fit the button bounds
    float txt_scale;
    if (e->w - TEXTBOX_MARGIN < length) {
        txt_scale = ((e->w - TEXTBOX_MARGIN) / length);
    } else {
        txt_scale = 1.0f;
    }

    draw_text(&bigFont_fontCharset, &bigFont_sheet, transform->x - ((e->w / 2) - (TEXTBOX_MARGIN / 2)) * transform->scaleX, transform->y, txt_scale * transform->scaleX, txt_scale * transform->scaleY, 0.f, false, "%s", textbox->text);
}

static void ui_textbox_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UITextbox *ui_create_textbox(UIScreen *screen) {
    UITextbox *e = malloc(sizeof(UITextbox));

    if (!e) return NULL;

    memset(e, 0, sizeof(UITextbox));
    e->base.type = UI_TEXTBOX;
    e->base.enabled = true;
    e->base.update = ui_textbox_update;
    e->base.draw = ui_textbox_draw;
    e->base.destroy = ui_textbox_destroy;
    
    ui_element_apply_default_properties(&e->base, screen);

    e->atlas = C2D_SpriteSheetGetImage(window_sheet, TEXTBOX_STYLE);
    e->border = e->atlas.subtex->width / 3;

    return e;
}

UIElement *ui_create_textbox_from_props(UIScreen *screen, const UIPropertyList *props) {
    UITextbox *textbox = ui_create_textbox(screen);

    if (!textbox) return NULL;

    ui_element_apply_properties(&textbox->base, screen, props);

    if (textbox->base.h == 0) {
        textbox->base.h = 30;
    }
    
    textbox->character_limit = ui_prop_int(props, "limit", 16);

    strncpy(textbox->title, ui_prop_string(props, "title", "Enter text:"), sizeof(textbox->title) - 1);

    return &textbox->base;
}