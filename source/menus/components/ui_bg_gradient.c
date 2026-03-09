#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"
#include "ui_bg_gradient.h"

static void ui_bg_gradient_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

static void ui_bg_gradient_draw(UIElement* e) {
    C2D_SpriteSetPos(&e->image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->image.sprite, e->image.scaleX, e->image.scaleY);
    if (e->image.useTint) {
        C2D_DrawSpriteTinted(&e->image.sprite, &e->image.tint);
    } else {
        C2D_DrawSprite(&e->image.sprite);
    }
}

UIElement ui_create_bg_gradient(char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_IMAGE;
    e.x = 0;
    e.y = 0;
    e.enabled = true;
    e.image.useTint = false;

    C2D_SpriteFromSheet(&e.image.sprite, bg_gradient_sheet, 0);

    // Copy tag
    copy_tag_array(&e, tag);

    e.w = e.image.sprite.image.subtex->width * BG_GRADIENT_XSCALE;
    e.h = e.image.sprite.image.subtex->height * BG_GRADIENT_YSCALE;

    e.image.scaleX = BG_GRADIENT_XSCALE;
    e.image.scaleY = BG_GRADIENT_YSCALE;

    e.update = ui_bg_gradient_update;
    e.draw = ui_bg_gradient_draw;

    return e;
}