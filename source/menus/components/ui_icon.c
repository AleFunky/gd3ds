#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_icon.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"
#include "graphics.h"

#include "menus/icon_kit.h"

#include "main.h"

#define FIRST_TRAIL_ID 27

static void ui_icon_update(UIElement* e, UIInput* touch) {
    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    // Check if pressed the icon
    if (inside && pressedTouch && !touch->did_something) {
        e->icon.hovered = true;
        e->icon.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside && e->icon.pressed) {
        e->icon.hovered = true;
    }
    
    EaseTypes bounce_type;
    // Animation
    if (e->icon.hovered) {
        e->icon.hoverTimer += DT;
        bounce_type = BOUNCE_OUT;
    } else {
        e->icon.hoverTimer -= DT;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->icon.hoverTimer = clampf(e->icon.hoverTimer, 0.f, ICON_HOVER_ANIM_TIME);
    e->icon.hoverScale = easeValue(bounce_type, 1.0f, ICON_HOVER_SCALE, e->icon.hoverTimer, ICON_HOVER_ANIM_TIME, 0);


    // If released on icon, do its action
    if (e->icon.hovered && releasedTouch) {
        e->icon.pressed = false;
        e->icon.hovered = false;
        e->icon.hoverTimer = 0.f;
        e->icon.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the icon
    if (!inside) {
        e->icon.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_icon_draw(UIElement* e) {
    float scale = e->icon.hoverScale;

    float y = e->y;
    if (e->icon.gamemode == GAMEMODE_SHIP) y -= 4;

    if (e->icon.gamemode == TRAIL) {
        C2D_Sprite spr = { 0 };
        C2D_SpriteFromSheet(&spr, ui_2_sheet, FIRST_TRAIL_ID + e->icon.index);
        C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C2D_SpriteSetPos(&spr, e->x, y);
        C2D_SpriteSetScale(&spr, scale * e->icon.scaleX, scale * e->icon.scaleX);
        C2D_DrawSprite(&spr);
    } else {
        spawn_icon_at(
            e->icon.gamemode,
            e->icon.index,
            false,
            e->x, y,
            0,
            0,
            0,
            scale * e->icon.scaleX,
            C2D_Color32(175, 175, 175, 255),
            C2D_Color32(255, 255, 255, 255),
            0
        );
    }

    if (e->icon.isSelected) {
        C2D_SpriteSetCenter(&e->icon.image.sprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&e->icon.image.sprite, e->x, e->y);
        C2D_SpriteSetScale(&e->icon.image.sprite, e->icon.scaleX, e->icon.scaleX);
        C2D_DrawSprite(&e->icon.image.sprite);
    }
}

void ui_icon_set_gamemode_index(UIElement *e, int gamemode, int index) {
    if (e->type != UI_ICON) return;

    e->icon.isSelected = *current_icons[gamemode] == index,
    e->icon.gamemode = gamemode;
    e->icon.index = index;
}

UIElement ui_create_icon(
    int x, int y, float scale, int index, int gamemode, 
    UIActionFn action,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {
        .type = UI_ICON,
        .x = x, .y = y,
        .w = 30*scale, .h = 30*scale,
        .enabled = true,
        .action = action,
        .update = ui_icon_update,
        .draw = ui_icon_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);

    e.icon.scaleX = scale;

    C2D_SpriteFromSheet(&e.icon.image.sprite, ui_sheet, 175);
    C3D_TexSetFilter(e.icon.image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    ui_icon_set_gamemode_index(&e, gamemode, index);
    
    e.icon.hoverScale = 1.f;

    return e;
}