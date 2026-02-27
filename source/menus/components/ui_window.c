#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_checkbox.h"
#include "ui_screen.h"

static void ui_window_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

static void ui_window_draw(UIElement* e) {
    draw_9_slice(e->window.atlas, e->x, e->y, e->w, e->h, e->window.border, C2D_Color32(255, 255, 255, 255));
}

UIElement ui_create_window(
    int x, int y, int w, int h, int style,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_WINDOW,
        .x = x, .y = y,
        .w = w, .h = h,
        .enabled = true,
        .update = ui_window_update,
        .draw = ui_window_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);
    e.window.atlas = C2D_SpriteSheetGetImage(window_sheet, style);

    e.window.border = e.window.atlas.subtex->width / 3;

    return e;
}