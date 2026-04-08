#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_screen.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"

#include "main.h"

static void ui_external_level_card_update(UIElement* e, UIInput* touch) {
    float right_side = e->x + e->w / 2;
    float button_pos = right_side - 10;

    float width = e->external_level_card.button_w;
    float height = e->external_level_card.button_h;

    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= button_pos - (width / 2) && touch->touchPosition.px < button_pos + (width / 2) &&
                  touch->touchPosition.py >= e->y - (height / 2) && touch->touchPosition.py < e->y + (height / 2);

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->external_level_card.button.hovered = true;
        e->external_level_card.button.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && e->external_level_card.button.pressed) {
        e->external_level_card.button.hovered = true;
    }
    
    EaseTypes bounce_type;
    // Animation
    if (e->external_level_card.button.hovered) {
        e->external_level_card.button.hoverTimer += DT;
        bounce_type = BOUNCE_OUT;
    } else {
        e->external_level_card.button.hoverTimer -= DT;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->external_level_card.button.hoverTimer = clampf(e->external_level_card.button.hoverTimer, 0.f, BUTTON_HOVER_ANIM_TIME);
    e->external_level_card.button.hoverScale = easeValue(bounce_type, 1.0f, BUTTON_HOVER_SCALE, e->external_level_card.button.hoverTimer, BUTTON_HOVER_ANIM_TIME, 0);


    // If released on button, do its action
    if (e->external_level_card.button.hovered && releasedTouch) {
        e->external_level_card.button.pressed = false;
        e->external_level_card.button.hovered = false;
        e->external_level_card.button.hoverTimer = 0.f;
        e->external_level_card.button.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the button
    if (!inside) {
        e->external_level_card.button.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_external_level_card_draw(UIElement* e) {
    float left_side = e->x - e->w / 2;
    float right_side = e->x + e->w / 2;
    float icon_pos = left_side + 15;
    float text_pos = icon_pos + 14;
    float button_pos = right_side - 15;

    // Draw icon
    C2D_SpriteSetCenter(&e->external_level_card.icon.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->external_level_card.icon.sprite, icon_pos, e->y);
    C2D_SpriteSetScale(&e->external_level_card.icon.sprite, e->external_level_card.icon.scaleX, e->external_level_card.icon.scaleY);
    C2D_DrawSprite(&e->external_level_card.icon.sprite);

    draw_text(bigFont_fontCharset, bigFont_sheet, text_pos, e->y, e->external_level_card.label.scale, e->external_level_card.label.alignment, "%s", e->external_level_card.label.text);

    float scale = e->external_level_card.button.hoverScale;

    C2D_SpriteSetCenter(&e->external_level_card.button.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->external_level_card.button.image.sprite, button_pos, e->y);
    C2D_SpriteSetScale(&e->external_level_card.button.image.sprite, scale * e->external_level_card.button.scaleX, scale * e->external_level_card.button.scaleY);
    C2D_DrawSprite(&e->external_level_card.button.image.sprite);
}

void ui_external_level_card_set_image(UIElement *e, int sprite_index, int sheet) {
    if (e->type != UI_EXTERNAL_LEVEL_CARD) return;

    C2D_SpriteFromSheet(&e->external_level_card.icon.sprite, *get_sheet(sheet), sprite_index);
    C3D_TexSetFilter(e->external_level_card.icon.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
}

UIElement ui_create_external_level_card(int x, int y, int image, int sheet, char *text, char *level_path, UIActionFn action, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_EXTERNAL_LEVEL_CARD;
    e.x = x;
    e.y = y;
    e.w = 200;
    e.h = 28;
    e.enabled = true;
    e.action = action;
    
    // Copy tag
    copy_tag_array(&e, tag);
    
    ui_external_level_card_set_image(&e, image, sheet);
    
    // Copy text
    strncpy(e.external_level_card.label.text, text, 255);
    strncpy(e.external_level_card.path, level_path, 255);

    e.external_level_card.icon.scaleX = 0.58f;
    e.external_level_card.icon.scaleY = 0.58f;

    e.external_level_card.label.scale = 0.54f;
    e.external_level_card.label.alignment = 0;

    // Button image
    C2D_SpriteFromSheet(&e.external_level_card.button.image.sprite, *get_sheet(0), 6);
    C3D_TexSetFilter(e.external_level_card.button.image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e.external_level_card.button.scaleX = -0.5f;
    e.external_level_card.button.scaleY = 0.5f;

    e.external_level_card.button_w = fabsf(e.external_level_card.button.image.sprite.image.subtex->width * e.external_level_card.button.scaleX);
    e.external_level_card.button_h = fabsf(e.external_level_card.button.image.sprite.image.subtex->height * e.external_level_card.button.scaleY);
    
    e.external_level_card.button.hoverScale = 1.f;

    e.update = ui_external_level_card_update;
    e.draw = ui_external_level_card_draw;

    return e;
}