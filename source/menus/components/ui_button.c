#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"

static void ui_button_update(UIElement* e, UIInput* touch) {
    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->button.hovered = true;
        e->button.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && e->button.pressed) {
        e->button.hovered = true;
    }
    
    EaseTypes bounce_type;
    // Animation
    if (e->button.hovered) {
        e->button.hoverTimer += 1 / 60.f;
        bounce_type = BOUNCE_OUT;
    } else {
        e->button.hoverTimer -= 1 / 60.f;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->button.hoverTimer = clampf(e->button.hoverTimer, 0.f, BUTTON_HOVER_ANIM_TIME);
    e->button.hoverScale = easeValue(bounce_type, 1.0f, BUTTON_HOVER_SCALE, e->button.hoverTimer, BUTTON_HOVER_ANIM_TIME, 0);


    // If released on button, do its action
    if (e->button.hovered && releasedTouch) {
        e->button.pressed = false;
        e->button.hovered = false;
        e->button.hoverTimer = 0.f;
        e->button.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the button
    if (!inside) {
        e->button.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_button_draw(UIElement* e) {
    float scale = e->button.hoverScale;

    float even_sx = closest_even_mult(e->button.image.sprite.image.subtex->width, scale * e->button.scaleX);
    float even_sy = closest_even_mult(e->button.image.sprite.image.subtex->height, scale * e->button.scaleY);

    C2D_SpriteSetCenter(&e->button.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->button.image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->button.image.sprite, even_sx, even_sy);
    C2D_DrawSprite(&e->button.image.sprite);

    // Get text length in pixels
    float length = get_text_length(bigFont_fontCharset, 1 / 0.85f, "%s", e->button.text);

    // Resize it to fit the button bounds
    float txt_scale;
    if (e->w < length) {
        txt_scale = scale * (e->w / length);
    } else {
        txt_scale = scale * 0.85f;
    }

    draw_text(bigFont_fontCharset, bigFont_sheet, e->x, e->y, txt_scale, 0.5f, "%s", e->button.text);
}

UIElement ui_create_button(
    int x, int y, float sx, float sy, int sprite_index, int sheet, 
    UIActionFn action,
    char *text,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_BUTTON,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .enabled = true,
        .action = action,
        .update = ui_button_update,
        .draw = ui_button_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    strncpy(e.button.text, text, 63);

    C2D_SpriteFromSheet(&e.button.image.sprite, *get_sheet(sheet), sprite_index);
    
    e.button.hoverScale = 1.f;
    e.w = fabsf(e.button.image.sprite.image.subtex->width  * sx);
    e.h = fabsf(e.button.image.sprite.image.subtex->height * sy);

    e.button.scaleX = sx;
    e.button.scaleY = sy;

    return e;
}