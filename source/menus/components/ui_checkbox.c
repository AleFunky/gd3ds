#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_checkbox.h"

static void set_checkbox_texture(UIElement* e, bool enabled) {
    int tex = enabled ? 28 : 27;
    C2D_SpriteFromSheet(&e->checkbox.image.sprite, ui_sheet, tex);

    e->w = e->checkbox.image.sprite.params.pos.w;
    e->h = e->checkbox.image.sprite.params.pos.h;
}

// Set checkbox checked state
void set_checkbox_enabled(UIElement *e, bool enabled) {
    if (e->type != UI_CHECKBOX) return;

    set_checkbox_texture(e, enabled);
    e->checkbox.checked = enabled;
}


static void ui_checkbox_update(UIElement* e, touchPosition* touch) {
    if (!e->enabled || !e->visible) return;

    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->px >= e->x - (e->w / 2) && touch->px < e->x + (e->w / 2) &&
                  touch->py >= e->y - (e->h / 2) && touch->py < e->y + (e->h / 2);

    // Check if pressed the checkbox
    if (inside && pressedTouch) {
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
        if (e->action)
            e->action(e->action_data);
        e->checkbox.checked ^= 1;
        set_checkbox_texture(e, e->checkbox.checked);
    }
    
    // Unpress the checkbox
    if (!inside) {
        e->checkbox.hovered = false;
    }
}

static void ui_checkbox_draw(UIElement* e) {
    if (!e->visible) return;

    float scale = e->checkbox.hoverScale;

    C2D_SpriteSetCenter(&e->checkbox.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->checkbox.image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->checkbox.image.sprite, scale, scale);
    C2D_DrawSprite(&e->checkbox.image.sprite);
}

UIElement ui_create_checkbox(
    int x, int y, bool enabled,
    UIActionFn action,
    void *action_data,
    char *tag
) {
    UIElement e = {
        .type = UI_CHECKBOX,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .visible = true,
        .enabled = true,
        .action = action,
        .action_data = action_data,
        .update = ui_checkbox_update,
        .draw = ui_checkbox_draw
    };

    // Copy tag
    strncpy(e.tag, tag, 15);

    set_checkbox_texture(&e, enabled);

    return e;
}