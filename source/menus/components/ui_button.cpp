#include "menus/core/common_setters.hpp"

#include <citro2d.h>
#include "text.hpp"
#include "easing.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/core/ui_props.hpp"

#include "main.hpp"
#include "ui_slider.hpp"

const UIIntEnumEntry buttonAnimEnum[] = {
    { "normal", BUTTON_ANIM_NORMAL },
    { "pull", BUTTON_ANIM_PULL },
    { "darken", BUTTON_ANIM_DARKEN }
};

//prevents two or more buttons from being pressed at the exact same time
static int pressedKey;

void uiButtonUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    //Keybinds logic
    u32 validKeybinds = button->keyBinds;

    if(settingsState.enableDebugBindings && gameState == STATE_GAME && !gamePaused && !inLevelComplete){
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
            performActions(e);
        }
        if(--(button->keyPressTimer) == 0){
            button->pressed = false;
            button->hovered = false;
        }
    }

    // Animation
    switch(button->animType){
        case BUTTON_ANIM_NORMAL:
            EaseTypes bounceType;
            if (button->hovered) {
                button->hoverTimer += DT * (button->keyPressTimer > 0 ? 2 : 1);
                bounceType = (button->keyPressTimer > 0 ? EASE_OUT : BOUNCE_OUT);
            } else {
                button->hoverTimer -= DT;
                // As the animation plays in reverse, we just use bounce in
                bounceType = BOUNCE_IN;
            }

            button->hoverTimer = clampf(button->hoverTimer, 0.f, BUTTON_HOVER_ANIM_TIME);
            button->hoverProgress = easeValue(bounceType, 1.0f, BUTTON_HOVER_SCALE, button->hoverTimer, BUTTON_HOVER_ANIM_TIME, 0);

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

    bool inside = uiElementBasicBoundCheck(e, touch, transform) && !pressedKey;

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
        if (button->preAction) {
            button->preAction(e, NULL);
        }

        performActions(e);
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

void uiButtonDrawText(UIElement *e, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    int fontID = button->font;

    // Set to pusab if invalid
    if (fontID >= NUM_FONTS) fontID = 0;

    const LabelFont *font = &fonts[fontID];

    float textScale = button->textScale;

    float width = e->w;

    if (button->textScale == 0){
        // Get text length in pixels
        float length = getTextLength(font->charset, 1 / 0.85f, true, button->text);
    
        if (width < length) {
            textScale = width / length;
        } else {
            textScale = 0.85f;
        }
    } else {
        textScale = button->textScale;
    }

    drawText(font->charset, font->sheet, transform->x, transform->y, textScale * transform->scaleX, textScale * transform->scaleY, 0.5f, true, "%s", button->text);
}

void uiButtonModifyTransform(UIElement *e, UITransform *t) {
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

static void uiButtonDraw(UIElement* e, UITransform *transform) {
    UIButton *button = (UIButton *) e;

    if(button->invisible) return;

    C2D_SpriteSetCenter(&button->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&button->image.sprite, transform->x, transform->y);
    C2D_SpriteSetScale(&button->image.sprite, transform->scaleX, transform->scaleY);
    C2D_DrawSpriteTinted(&button->image.sprite, &button->image.tint);

    uiButtonDrawText(e, transform);
}

static void uiButtonDestroy(UIElement *e) {
    if (e) {
        delete (UIButton *)e;
        e = NULL;
    }
}

static void uiButtonOnDisable(UIElement *e) {
    UIButton *button = (UIButton *) e;
    button->hovered = false;
    button->hoverProgress = 1.f;
    button->hoverTimer = 0.f;
}

void uiButtonSetText(UIButton *e, const char *text) {
    if (!e || !text) return;

    strncpy(e->text, text, sizeof(e->text) - 1);
}

void uiButtonSetImage(UIButton *e, int spriteIndex, int sheet) {
    if (!e || e->invisible) return;

    C2D_SpriteFromSheet(&e->image.sprite, getSheet(sheet), spriteIndex);
    C3D_TexSetFilter(e->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->image.sprite.image.subtex->width;
    e->base.h = e->image.sprite.image.subtex->height;
}

UIButton *uiCreateButton(UIScreen *screen) {
    UIButton *e = new UIButton{};

    if (!e) return NULL;

    e->base.type = UI_BUTTON;
    e->base.enabled = true;
    e->base.update = uiButtonUpdate;
    e->base.draw = uiButtonDraw;
    e->base.destroy = uiButtonDestroy;
    
    e->base.modifyTransform = uiButtonModifyTransform;

    e->base.onDisable = uiButtonOnDisable;

    e->base.opacity = 1.f;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(1, 1, 1, 1), 1.f);

    uiElementApplyDefaultProperties(&e->base, screen);

    e->hoverProgress = 1.f;
    e->hoverFactor = 1.f;

    pressedKey = false;

    return e;
}

UIElement *uiCreateButtonFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIButton *button = uiCreateButton(screen);

    if (!button) return NULL;

    uiElementApplyProperties(&button->base, screen, props);

    button->invisible = uiPropBool(props, "invisible", false);

    uiButtonSetImage(button, 
        uiPropInt(props, "id", 0),
        uiPropInt(props, "sheet", 0)
    );
    C2D_PlainImageTint(&button->image.tint, C2D_Color32f(1, 1, 1, button->base.opacity), 1.f);
    
    // Copy text
    strncpy(button->text, uiPropString(props, "text", ""), 63);

    button->font = uiPropInt(props, "font", 0);
    button->textScale = uiPropFloat(props, "textScale", 0);

    button->hoverFactor = uiPropFloat(props, "hoverFactor", 1);
    
    button->keyBinds = uiPropBitfield(props, "keyBinds", keybindTable, ARRAY_LEN(keybindTable));

    button->animType = (ButtonAnimType)uiPropIntEnum(props, "anim", buttonAnimEnum, ARRAY_LEN(buttonAnimEnum), BUTTON_ANIM_NORMAL);
    
    return &button->base;
}