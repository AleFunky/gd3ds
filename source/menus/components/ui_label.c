#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "ui_screen.h"


typedef struct {
    const Charset *charset;
    C2D_SpriteSheet *sheet;    
} LabelFont;

static LabelFont fonts[] = {
    {
        .charset = &bigFont_fontCharset,
        .sheet = &bigFont_sheet
    },
    {
        .charset = &chatFont_fontCharset,
        .sheet = &chatFont_sheet
    },
    {
        .charset = &goldFont_fontCharset,
        .sheet = &goldFont_sheet
    }
};

#define NUM_FONTS (sizeof(fonts) / sizeof(LabelFont))

static void ui_label_update(UIElement* e, UIInput* touch) {
    // Do absolutely nothing
    (void)e;
    (void)touch;
}

static void ui_label_draw(UIElement* e) {
    int font_id = e->label.font;

    // Set to pusab if invalid
    if (font_id >= NUM_FONTS) font_id = 0;

    LabelFont *font = &fonts[font_id];
    draw_text(font->charset, font->sheet, e->x, e->y, e->label.scale, e->label.alignment, "%s", e->label.text);
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