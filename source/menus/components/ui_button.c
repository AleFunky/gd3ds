#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"
#include "menus/settings.h"
#include "menus/gameplay.h"
#include "state.h"

#include "main.h"
#include "ui_slider.h"

//if a button has been pressed with a keybind, no other button should be pressed after
static int pressedKey;

static void ui_button_update(UIElement* e, UIInput* touch) {
    //Keybinds logic
    u32 validKeybinds = e->button.keyBinds;

    if(enableDebugBindings && game_state == STATE_GAME && !game_paused && !in_level_complete){
        validKeybinds &= ~(KEY_B | KEY_X | KEY_L | KEY_R);
    }

    if((hidKeysDown() & validKeybinds) > 0){
        e->button.pressed = true;
        e->button.hovered = true;
        e->button.hoverTimer = 0.2f;
        e->button.keyPressTimer = 45;
        pressedKey = true;
    }

    if(e->button.keyPressTimer > 0){
        if(e->button.keyPressTimer == 44){
            pressedKey = false;
            if (e->action){
                e->action(e);
            }
        }
        if(--(e->button.keyPressTimer) == 0){
            e->button.pressed = false;
            e->button.hovered = false;
        }
    }

    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2) && 
                  !pressedKey;

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->button.hovered = true;
        e->button.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && !sliding) {
        e->button.hovered = true;
    }
    
    // If released on button, do its action
    if (e->button.hovered && releasedTouch && !pressedKey) {
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
    EaseTypes bounce_type;
    // Animation
    if (e->button.hovered) {
        e->button.hoverTimer += DT * (e->button.keyPressTimer > 0 ? 2 : 1);
        bounce_type = (e->button.keyPressTimer > 0 ? EASE_OUT : BOUNCE_OUT);
    } else {
        e->button.hoverTimer -= DT;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->button.hoverTimer = clampf(e->button.hoverTimer, 0.f, BUTTON_HOVER_ANIM_TIME);
    e->button.hoverScale = easeValue(bounce_type, 1.0f, BUTTON_HOVER_SCALE, e->button.hoverTimer, BUTTON_HOVER_ANIM_TIME, 0);

    int font_id = e->button.font;

    // Set to pusab if invalid
    if (font_id >= NUM_FONTS) font_id = 0;

    const LabelFont *font = &fonts[font_id];

    float scale = e->button.hoverScale;
    float text_scale = e->button.textScale;
    C2D_ImageTint tint;

    C2D_PlainImageTint(&tint, C2D_Color32f(1, 1, 1, e->opacity), 1.f);

    C2D_SpriteSetCenter(&e->button.image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->button.image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->button.image.sprite, scale * e->button.scaleX, scale * e->button.scaleY);
    C2D_DrawSpriteTinted(&e->button.image.sprite, &tint);

    if (e->button.textScale == 0){
        // Get text length in pixels
        float length = get_text_length(font->charset, 1 / 0.85f, e->button.text);
    
        if (e->w < length) {
            text_scale = scale * (e->w / length);
        } else {
            text_scale = scale * 0.85f;
        }
    } else {
        text_scale = (e->button.textScale * scale);
    }

    draw_text(font->charset, font->sheet, e->x, e->y, text_scale, text_scale, 0.5f, "%s", e->button.text);
}

void ui_button_set_image(UIElement *e, int sprite_index, int sheet) {
    if (e->type != UI_BUTTON) return;

    C2D_SpriteFromSheet(&e->button.image.sprite, *get_sheet(sheet), sprite_index);
    C3D_TexSetFilter(e->button.image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->w = fabsf(e->button.image.sprite.image.subtex->width * e->button.scaleX);
    e->h = fabsf(e->button.image.sprite.image.subtex->height * e->button.scaleY);
}

UIElement ui_create_button(
    int x, int y, float sx, float sy, int sprite_index, int sheet, float opacity,
    UIActionFn action,
    char *text,
    int font,
    char (*tag)[TAG_LENGTH],
    float textScale,
    u32 keyBinds
) {
    UIElement e = {
        .type = UI_BUTTON,
        .x = x, .y = y,
        .w = 0, .h = 0,
        .enabled = true,
        .action = action,
        .update = ui_button_update,
        .draw = ui_button_draw,
        .opacity = opacity
    };

    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    strncpy(e.button.text, text, 63);

    e.button.scaleX = sx;
    e.button.scaleY = sy;

    ui_button_set_image(&e, sprite_index, sheet);
    
    e.button.hoverScale = 1.f;

    e.button.font = font;
    e.button.textScale = textScale;

    e.button.keyBinds = keyBinds;

    pressedKey = false;

    return e;
}