#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"

static void ui_button_update(UIElement* e, touchPosition* touch) {
    if (!e->enabled || !e->visible) return;

    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->px >= e->x - (e->w / 2) && touch->px < e->x + (e->w / 2) &&
                  touch->py >= e->y - (e->h / 2) && touch->py < e->y + (e->h / 2);

    // Check if pressed the button
    if (inside && pressedTouch) {
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
        if (e->action)
            e->action(e->action_data);
    }
    
    // Unpress the button
    if (!inside) {
        e->button.hovered = false;
    }
}

static void ui_button_draw(UIElement* e) {
    if (!e->visible) return;

    float scale = e->button.hoverScale;

    C2D_SpriteSetCenter(&e->button.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->button.image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->button.image.sprite, scale, scale);
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
    int x, int y, int sprite_index, 
    UIActionFn action,
    void *action_data,
    char *text,
    char *tag
) {
    UIElement e = {
        .type = UI_BUTTON,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .visible = true,
        .enabled = true,
        .action = action,
        .action_data = action_data,
        .update = ui_button_update,
        .draw = ui_button_draw
    };

    // Copy tag
    strncpy(e.tag, tag, 15);

    // Copy text
    strncpy(e.button.text, text, 63);

    C2D_SpriteFromSheet(&e.button.image.sprite, ui_sheet, sprite_index);

    e.w = e.button.image.sprite.params.pos.w;
    e.h = e.button.image.sprite.params.pos.h;

    return e;
}