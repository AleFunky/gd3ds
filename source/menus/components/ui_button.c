#include "menus/core/common_setters.h"

#include <citro2d.h>
#include "text.h"
#include "easing.h"
#include "math_helpers.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_button.h"
#include "menus/settings_hub/settings.h"
#include "menus/core/ui_props.h"

#include "main.h"
#include "ui_slider.h"

const UIIntEnumEntry button_anim_enum[] = {
    { "normal", BUTTON_ANIM_NORMAL },
    { "pull", BUTTON_ANIM_PULL },
    { "darken", BUTTON_ANIM_DARKEN }
};

//prevents two or more buttons from being pressed at the exact same time
static int pressedKey;

void ui_button_update(UIElement* e, UIInput* touch, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    //Keybinds logic
    u32 validKeybinds = button->keyBinds;

    if(settingsState.enableDebugBindings && game_state == STATE_GAME && !game_paused && !in_level_complete){
        validKeybinds &= ~(KEY_B | KEY_X | KEY_L | KEY_R);
    }

    if((touch->down & validKeybinds) > 0){
        button->pressed = true;
        button->hovered = true;
        button->hoverTimer = 0.2f;
        button->keyPressTimer = 45;
        pressedKey = true;
    }

    if(button->keyPressTimer > 0){
        if(button->keyPressTimer == 44){
            pressedKey = false;
            perform_actions(e);
        }
        if(--(button->keyPressTimer) == 0){
            button->pressed = false;
            button->hovered = false;
        }
    }

    // Animation
    switch(button->animType){
        case BUTTON_ANIM_NORMAL:
            EaseTypes bounce_type;
            if (button->hovered) {
                button->hoverTimer += DT * (button->keyPressTimer > 0 ? 2 : 1);
                bounce_type = (button->keyPressTimer > 0 ? EASE_OUT : BOUNCE_OUT);
            } else {
                button->hoverTimer -= DT;
                // As the animation plays in reverse, we just use bounce in
                bounce_type = BOUNCE_IN;
            }

            button->hoverTimer = clampf(button->hoverTimer, 0.f, BUTTON_HOVER_ANIM_TIME);
            button->hoverProgress = easeValue(bounce_type, 1.0f, BUTTON_HOVER_SCALE, button->hoverTimer, BUTTON_HOVER_ANIM_TIME, 0);

            // Apply hover factor
            button->hoverProgress = 1 + (button->hoverProgress - 1) * button->hoverFactor;
            break;
        case BUTTON_ANIM_PULL:
            if (button->hovered) {
                button->hoverTimer += DT;
            } else {
                button->hoverTimer -= DT;
            }

            button->hoverTimer = clampf(button->hoverTimer, 0.f, 0.2f);
            button->hoverProgress = easeValue(EASE_IN_OUT, 0.f, 6.f, button->hoverTimer, 0.2f, 2.0f);
            break;
        default:
            break;
    }

    bool pressedTouch = touch->down & KEY_TOUCH;
    bool releasedTouch = touch->up & KEY_TOUCH;

    bool inside = ui_element_basic_bound_check(e, touch, transform) && !pressedKey;

    // Check if pressed the button
    if (inside && pressedTouch) {
        button->hovered = true;
        button->pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && !sliding) {
        button->hovered = true;
    }
    
    // If released on button, do its action
    if (button->hovered && releasedTouch && !pressedKey) {
        button->pressed = false;
        button->hovered = false;
        //only the normal animation (zoom in) snaps when you release it
        if(button->animType == BUTTON_ANIM_NORMAL){
            button->hoverTimer = 0.f;
            button->hoverProgress = 1.f;
        }
        
        // This lets subclasses do something before the real action (checkbox uses it to flip the texture)
        if (button->pre_action) {
            button->pre_action(e, NULL);
        }

        perform_actions(e);
    }
    
    // Unpress the button
    if (!inside) {
        button->hovered = false;
    }

    // Mask background elements
    if (inside) {
        touch->interacted = true;
    }
}

void ui_button_draw_text(UIElement *e, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    int font_id = button->font;

    // Set to pusab if invalid
    if (font_id >= NUM_FONTS) font_id = 0;

    const LabelFont *font = &fonts[font_id];

    float text_scale = button->textScale;

    float width = e->w;

    if (button->textScale == 0){
        // Get text length in pixels
        float length = get_text_length(font->charset, 1 / 0.85f, true, button->text);
    
        if (width < length) {
            text_scale = width / length;
        } else {
            text_scale = 0.85f;
        }
    } else {
        text_scale = button->textScale;
    }

    draw_text(font->charset, font->sheet, transform->x, transform->y, text_scale * transform->scaleX, text_scale * transform->scaleY, 0.5f, true, "%s", button->text);
}

void ui_button_modify_transform(UIElement *e, UITransform *t) {
    UIButton *button = (UIButton *) e;

    switch(button->animType){
        case BUTTON_ANIM_NORMAL:
            t->scaleX *= button->hoverProgress;
            t->scaleY *= button->hoverProgress;
            break;
        case BUTTON_ANIM_PULL:
            t->y += button->hoverProgress;
            break;
        default:
            break;
    }
}

static void ui_button_draw(UIElement* e, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    if(button->invisible) return;

    C2D_SpriteSetCenter(&button->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&button->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&button->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&button->image.sprite, &button->image.tint);

    ui_button_draw_text(e, transform);
}

static void ui_button_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

static void ui_button_on_disable(UIElement *e) {
    UIButton *button = (UIButton *) e;
    button->hovered = false;
    button->hoverProgress = 1.f;
    button->hoverTimer = 0.f;
}

void ui_button_set_text(UIButton *e, const char *text) {
    if (!e || !text) return;

    strncpy(e->text, text, sizeof(e->text) - 1);
}

void ui_button_set_image(UIButton *e, int sprite_index, int sheet) {
    if (!e || e->invisible) return;

    C2D_SpriteFromSheet(&e->image.sprite, *get_sheet(sheet), sprite_index);
    C3D_TexSetFilter(e->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;
}

UIButton *ui_create_button(UIScreen *screen) {
    UIButton *e = malloc(sizeof(UIButton));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIButton));

    e->base.type = UI_BUTTON;
    e->base.enabled = true;
    e->base.update = ui_button_update;
    e->base.draw = ui_button_draw;
    e->base.destroy = ui_button_destroy;
    
    e->base.modify_transform = ui_button_modify_transform;

    e->base.on_disable = ui_button_on_disable;

    e->base.opacity = 1.f;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(1, 1, 1, 1), 1.f);

    ui_element_apply_default_properties(&e->base, screen);

    e->hoverProgress = 1.f;
    e->hoverFactor = 1.f;

    pressedKey = false;

    return e;
}

UIElement *ui_create_button_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIButton *button = ui_create_button(screen);

    if (!button) return NULL;

    ui_element_apply_properties(&button->base, screen, props);

    button->invisible = ui_prop_bool(props, "invisible", false);

    ui_button_set_image(button, 
        ui_prop_int(props, "id", 0),
        ui_prop_int(props, "sheet", 0)
    );
    C2D_PlainImageTint(&button->image.tint, C2D_Color32f(1, 1, 1, button->base.opacity), 1.f);
    
    // Copy text
    strncpy(button->text, ui_prop_string(props, "text", ""), 63);

    button->font = ui_prop_int(props, "font", 0);
    button->textScale = ui_prop_float(props, "textScale", 0);

    button->hoverFactor = ui_prop_float(props, "hoverFactor", 1);
    
    button->keyBinds = ui_prop_bitfield(props, "keyBinds", keybind_table, ARRAY_LEN(keybind_table));

    button->animType = ui_prop_int_enum(props, "anim", button_anim_enum, ARRAY_LEN(button_anim_enum), BUTTON_ANIM_NORMAL);
    
    return &button->base;
}