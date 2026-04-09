#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/chatFont.h"
#include "ui_screen.h"

static void ui_label_aller_update(UIElement* e, UIInput* touch) {
    // Do absolutely nothing
    (void)e;
    (void)touch;
}

static void ui_label_aller_draw(UIElement* e) {
    draw_text(chatFont_fontCharset, chatFont_sheet, e->x, e->y, e->label.scale, e->label.alignment, "%s", e->label.text);
}

UIElement ui_create_label_aller(int x, int y, float scale, char *text, float alignment, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_LABEL_ALLER;
    e.x = x;
    e.y = y;
    e.w = 0;
    e.h = 10;
    e.enabled = true;
    
    e.label.alignment = alignment;
    e.label.scale = scale;
    
    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    strncpy(e.label.text, text, 255);

    e.update = ui_label_aller_update;
    e.draw = ui_label_aller_draw;

    return e;
}