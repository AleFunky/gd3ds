#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_checkbox.h"
#include "ui_screen.h"

static void set_checkbox_texture(UIElement* e, bool enabled) {
    int tex = enabled ? 28 : 27;
    C2D_SpriteFromSheet(&e->checkbox.image.sprite, ui_sheet, tex);

    e->w = fabsf(e->checkbox.image.sprite.image.subtex->width * e->checkbox.scaleX);
    e->h = fabsf(e->checkbox.image.sprite.image.subtex->height * e->checkbox.scaleY);
}

// Set checkbox checked state
void set_checkbox_enabled(UIElement *e, bool enabled) {
    if (e->type != UI_CHECKBOX) return;

    set_checkbox_texture(e, enabled);
    e->checkbox.checked = enabled;
}


static void ui_checkbox_update(UIElement* e, UIInput* touch) {
    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    // Check if pressed the checkbox
    if (inside && pressedTouch && !touch->did_something) {
        e->checkbox.hovered = true;
        e->checkbox.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && e->checkbox.pressed) {
        e->checkbox.hovered = true;
    }
    
    EaseTypes bounce_type;
    // Animation
    if (e->checkbox.hovered) {
        e->checkbox.hoverTimer += 1 / 60.f;
        bounce_type = BOUNCE_OUT;
    } else {
        e->checkbox.hoverTimer -= 1 / 60.f;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->checkbox.hoverTimer = clampf(e->checkbox.hoverTimer, 0.f, CHECKBOX_HOVER_ANIM_TIME);
    
    e->checkbox.hoverScale = easeValue(bounce_type, 1.0f, CHECKBOX_HOVER_SCALE, e->checkbox.hoverTimer, CHECKBOX_HOVER_ANIM_TIME, 0);

    // If released on checkbox, do its action
    if (e->checkbox.hovered && releasedTouch) {
        e->checkbox.checked ^= 1;
        e->checkbox.pressed = false;
        e->checkbox.hovered = false;
        e->checkbox.hoverTimer = 0.f;
        e->checkbox.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the checkbox
    if (!inside) {
        e->checkbox.hovered = false;
    }
                      
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }

    if (e->checkbox.checked != e->checkbox.image_id) {
        set_checkbox_texture(e, e->checkbox.checked);
        e->checkbox.image_id = e->checkbox.checked;
    }
}

static void ui_checkbox_draw(UIElement* e) {
    float scale = e->checkbox.hoverScale;

    float even_sx = closest_even_mult(e->checkbox.image.sprite.image.subtex->width, scale * e->checkbox.scaleX);
    float even_sy = closest_even_mult(e->checkbox.image.sprite.image.subtex->height, scale * e->checkbox.scaleY);

    C2D_SpriteSetCenter(&e->checkbox.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->checkbox.image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->checkbox.image.sprite, even_sx, even_sy);
    C2D_DrawSprite(&e->checkbox.image.sprite);
}

UIElement ui_create_checkbox(
    int x, int y, float sx, float sy, bool enabled,
    UIActionFn action,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_CHECKBOX,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .enabled = true,
        .action = action,
        .update = ui_checkbox_update,
        .draw = ui_checkbox_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);

    e.checkbox.scaleX = sx;
    e.checkbox.scaleY = sy;

    set_checkbox_texture(&e, enabled);

    return e;
}