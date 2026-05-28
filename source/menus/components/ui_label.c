#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "ui_screen.h"

static void ui_label_update(UIElement* e, UIInput* touch) {
    // Do absolutely nothing
    (void)e;
    (void)touch;
}

static void ui_label_draw(UIElement* e) {
    int font_id = e->label.font;

    // Set to pusab if invalid
    if (font_id >= NUM_FONTS) font_id = 0;

    const LabelFont *font = &fonts[font_id];
    draw_text(font->charset, font->sheet, e->x, e->y, e->label.scale, e->label.alignment, "%s", e->label.text);
}

void ui_label_set_text(UIElement *e, const char *text) {
    strncpy(e->label.text, text, sizeof(e->label.text) - 1);
}

UIElement ui_create_label(int x, int y, float scale, char *text, int font, float alignment, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_LABEL;
    e.x = x;
    e.y = y;
    e.w = 0;
    e.h = 10;
    e.enabled = true;
    
    e.label.font = font;
    e.label.alignment = alignment;
    e.label.scale = scale;
    
    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    strncpy(e.label.text, text, 255);

    e.update = ui_label_update;
    e.draw = ui_label_draw;

    return e;
}