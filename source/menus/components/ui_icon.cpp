#include "math_helpers.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/core/common_setters.hpp"

#include <citro2d.h>
#include "ui_icon.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "graphics.hpp"

#include "menus/icon_kit.hpp"
#include "player/robot_anim_data.hpp"
#include "player/spider_anim_data.hpp"

#define FIRST_TRAIL_ID 27

static void uiIconUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    uiButtonUpdate(e, touch, transform);
}

static void uiIconDraw(UIElement* e, UITransform *transform) {
    UIIcon *icon = (UIIcon *) e;
    UIButton *button = (UIButton *) e;

    if (icon->gamemode == GAMEMODE_SHIP) transform->y -= 4;

    if (icon->gamemode == TRAIL) {
        C2D_Sprite spr = { 0 };
        C2D_SpriteFromSheet(&spr, ui2Sheet, FIRST_TRAIL_ID + icon->index);
        C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C2D_SpriteSetPos(&spr, transform->x, transform->y);
        C2D_SpriteSetScale(&spr, transform->scaleX,  transform->scaleY);
        C2D_DrawSprite(&spr);
    } else {
        spawnIconAt(
            icon->gamemode,
            icon->index,
            icon->glow,
            transform->x, transform->y,
            0,
            0,
            0,
            transform->scaleX,
            icon->p1Color,
            icon->p2Color,
            icon->glowColor,
            (IconParameters) { 
                .animID = (icon->gamemode == (int)GAMEMODE_ROBOT ? (int)ROBOT_ANIM_IDLE : (int)SPIDER_ANIM_IDLE),
                .animFrame = 0
            }
        );
    }

    if (icon->isSelected) {
        C2D_SpriteSetCenter(&button->image.sprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&button->image.sprite, transform->x, transform->y);
        C2D_SpriteSetScale(&button->image.sprite, transform->scaleX, transform->scaleY);
        C2D_DrawSprite(&button->image.sprite);
    }
}

static void uiIconDestroy(UIElement *e) {
    if (e) {
        delete (UIIcon *)e;
        e = NULL;
    }
}

void uiIconSetSelected(UIIcon *e, bool selected) {
    if (!e) return;

    e->isSelected = selected;
}

void uiIconSetGamemodeIndex(UIIcon *e, int gamemode, int index) {
    if (!e) return;

    e->gamemode = gamemode;
    e->index = index;
}

void uiIconSetP1(UIIcon *e, u32 color) {
    if (!e) return;

    e->p1Color = color;
}

void uiIconSetP2(UIIcon *e, u32 color) {
    if (!e) return;

    e->p2Color = color;
}

void uiIconSetGlow(UIIcon *e, u32 color) {
    if (!e) return;

    e->glowColor = color;
}

UIIcon *uiCreateIcon(UIScreen *screen) {
    UIIcon *e = new UIIcon{};

    if (!e) return NULL;

    UIButton *button = (UIButton *) e;

    button->base.type = UI_ICON;
    button->base.enabled = true;

    button->base.update = uiIconUpdate;
    button->base.draw = uiIconDraw;
    button->base.destroy = uiIconDestroy;

    button->base.modifyTransform = uiButtonModifyTransform;

    uiElementApplyDefaultProperties(&button->base, screen);
    
    button->hoverProgress = 1;
    button->hoverFactor = 1;

    C2D_SpriteFromSheet(&button->image.sprite, uiSheet, 175);
    C3D_TexSetFilter(button->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    return e;
}

UIElement *uiCreateIconFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIIcon *icon = uiCreateIcon(screen);

    if (!icon) return NULL;

    UIButton *button = (UIButton *) icon;

    uiElementApplyProperties(&button->base, screen, props);

    uiElementSetSize(&button->base, 30, 30);
    
    button->hoverFactor = uiPropFloat(props, "hoverFactor", 1);    

    uiIconSetGamemodeIndex(icon, 
        uiPropInt(props, "gamemode", 0), 
        uiPropInt(props, "id", 0));

    icon->glow = uiPropBool(props, "glow", false);

    uiIconSetP1(icon, uiPropColor(props, "p1_color", ABGR8(175, 175, 175, 255)));
    uiIconSetP2(icon, uiPropColor(props, "p2_color", ABGR8(255, 255, 255, 255)));
    uiIconSetGlow(icon, uiPropColor(props, "glow_color", ABGR8(255, 255, 255, 255)));

    return &button->base;
}