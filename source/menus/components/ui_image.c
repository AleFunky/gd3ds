#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"

static void ui_image_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

static void ui_image_draw(UIElement* e) {
    C2D_SpriteSetCenter(&e->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->image.sprite, e->image.scaleX, e->image.scaleY);
    if (e->image.useTint) {
        C2D_DrawSpriteTinted(&e->image.sprite, &e->image.tint);
    } else {
        C2D_DrawSprite(&e->image.sprite);
    }
}

void ui_image_set_tint(UIElement* e, u32 color) {
    if (e->type != UI_IMAGE) return;

    C2D_PlainImageTint(&e->image.tint, color, 1.0f);
    e->image.useTint = true;
}

void ui_image_clear_tint(UIElement* e) {
    if (e->type != UI_IMAGE) return;
    
    e->image.useTint = false;
}

void ui_image_set_image(UIElement *e, int sprite_index, int sheet) {
    if (e->type != UI_IMAGE) return;

    C2D_SpriteFromSheet(&e->image.sprite, *get_sheet(sheet), sprite_index);

    float even_sx = closest_even_mult(e->image.sprite.image.subtex->width, e->image.scaleX);
    float even_sy = closest_even_mult(e->image.sprite.image.subtex->height, e->image.scaleY);

    e->w = e->image.sprite.image.subtex->width * even_sx;
    e->h = e->image.sprite.image.subtex->height * even_sy;

    e->image.scaleX = even_sx;
    e->image.scaleY = even_sy;
}

UIElement ui_create_image(int x, int y, int sprite_index, int sheet, float sx, float sy, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_IMAGE;
    e.x = x;
    e.y = y;
    e.enabled = true;
    e.image.useTint = false;

    e.image.scaleX = sx;
    e.image.scaleY = sy;

    // Copy tag
    copy_tag_array(&e, tag);

    ui_image_set_image(&e, sprite_index, sheet);

    e.update = ui_image_update;
    e.draw = ui_image_draw;

    return e;
}