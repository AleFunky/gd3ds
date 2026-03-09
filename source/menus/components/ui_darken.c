#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"

static void ui_darken_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

static void ui_darken_draw(UIElement* e) {
    C2D_SpriteSetPos(&e->darken.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->darken.sprite, 400/16, 240/16);
    C2D_DrawSpriteTinted(&e->darken.sprite, &e->darken.tint);
}

UIElement ui_create_darken(float opacity, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_DARKEN;
    e.x = 0;
    e.y = 0;
    e.enabled = true;
    e.darken.opacity = opacity;

    // Copy tag
    copy_tag_array(&e, tag);

    C2D_PlainImageTint(&e.darken.tint, C2D_Color32f(0, 0, 0, opacity), 1.0f);
    C2D_SpriteFromSheet(&e.darken.sprite, ui_sheet, 416);

    e.update = ui_darken_update;
    e.draw = ui_darken_draw;

    return e;
}