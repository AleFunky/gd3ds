#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_checkbox.h"
#include "ui_screen.h"
#include "ui_textbox.h"

#include "utils/keyboard.h"

static void ui_textbox_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) {
        touch->did_something = true;

        if (hidKeysDown() & KEY_TOUCH) {
            read_text(e->textbox.text, e->textbox.title, e->textbox.character_limit);
        }
    }
}

static void ui_textbox_draw(UIElement* e) {
    draw_9_slice(e->textbox.atlas, e->x, e->y, e->w, e->h, e->textbox.border, C2D_Color32(0, 0, 0, 127));
    
    // Get text length in pixels
    float length = get_text_length(bigFont_fontCharset, 1.f, "%s", e->textbox.text);

    // Resize it to fit the button bounds
    float txt_scale;
    if (e->w - TEXTBOX_MARGIN < length) {
        txt_scale = ((e->w - TEXTBOX_MARGIN) / length);
    } else {
        txt_scale = 1.0f;
    }

    draw_text(bigFont_fontCharset, bigFont_sheet, e->x - e->w / 2 + (TEXTBOX_MARGIN / 2), e->y, txt_scale, 0.f, "%s", e->textbox.text);
}

UIElement ui_create_textbox(
    int x, int y, int w, char *title,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_TEXTBOX,
        .x = x, .y = y,
        .w = w, .h = 30,
        .enabled = true,
        .update = ui_textbox_update,
        .draw = ui_textbox_draw
    };

    strncpy(e.textbox.title, title, 63);

    // Copy tag
    copy_tag_array(&e, tag);
    e.textbox.atlas = C2D_SpriteSheetGetImage(window_sheet, TEXTBOX_STYLE);
    e.textbox.border = e.textbox.atlas.subtex->width / 3;

    e.textbox.character_limit = 10;
    return e;
}