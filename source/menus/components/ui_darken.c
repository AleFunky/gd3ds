#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"

void ui_darken_reset_opacity(UIElement* e){
    C2D_PlainImageTint(&e->darken.tint, C2D_Color32f(0, 0, 0, e->opacity), 1.0f);
}

static void ui_darken_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;

    if(e->darken.darkenTime <= 0.f){
        e->opacity = e->darken.targetOpacity;
        e->darken.darkenOver = true;
    }

    if(!e->darken.darkenOver){
        e->opacity = (e->darken.darkenTimeElapsed / e->darken.darkenTime) * e->darken.targetOpacity;
        e->darken.darkenTimeElapsed += 1.f / 60.f;
    }

    if(e->darken.darkenTimeElapsed > e->darken.darkenTime && !e->darken.darkenOver){
        e->darken.darkenOver = true;
    }
}

static void ui_darken_draw(UIElement* e) {
    if(!e->darken.darkenOver){
        ui_darken_reset_opacity(e);
    }

    C2D_SpriteSetPos(&e->darken.sprite, 200, 120);
    C2D_SpriteSetScale(&e->darken.sprite, 400, 240);
    C2D_DrawSpriteTinted(&e->darken.sprite, &e->darken.tint);
}

UIElement ui_create_darken(float x, float y, float width, float height, float opacity, float darkenTime, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_DARKEN;
    e.x = x;
    e.y = y;
    e.w = width;
    e.h = height;
    e.enabled = true;
    e.opacity = 0.0f;

    e.darken.darkenTime = darkenTime;
    e.darken.darkenTimeElapsed = 0.f;
    e.darken.darkenOver = false;
    e.darken.targetOpacity = opacity;

    // Copy tag
    copy_tag_array(&e, tag);

    C2D_PlainImageTint(&e.darken.tint, C2D_Color32f(0, 0, 0, opacity), 1.0f);
    C2D_SpriteFromSheet(&e.darken.sprite, ui_sheet, 416);
    C2D_SpriteSetCenter(&e.darken.sprite, 0.5f, 0.5f);

    e.update = ui_darken_update;
    e.draw = ui_darken_draw;

    return e;
}