#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"

static void ui_action_area_update(UIElement* e, UIInput* touch) {
    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->action_area.hovered = true;
        e->action_area.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && e->action_area.pressed) {
        e->action_area.hovered = true;
    }

    // If released on button, do its action
    if (e->action_area.hovered && releasedTouch) {
        e->action_area.pressed = false;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the button
    if (!inside) {
        e->action_area.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_action_area_draw(UIElement* e) {
    (void)e;
}

UIElement ui_create_action_area(
    int x, int y, float w, float h, 
    UIActionFn action,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_ACTION_AREA,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .enabled = true,
        .action = action,
        .update = ui_action_area_update,
        .draw = ui_action_area_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);
    
    e.w = w;
    e.h = h;

    return e;
}