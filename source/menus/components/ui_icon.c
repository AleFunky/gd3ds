#include "math_helpers.h"
#include "menus/components/ui_button.h"
#include "menus/core/common_setters.h"
#include "menus/core/ui_element.h"
#include <citro2d.h>
#include "ui_icon.h"
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "graphics.h"

#include "menus/icon_kit.h"

#define FIRST_TRAIL_ID 27

static void ui_icon_update(UIElement* e, UIInput* touch, UITransform *transform) {
    ui_button_update(e, touch, transform);
}

static void ui_icon_draw(UIElement* e, UITransform *transform) {
    UIIcon *icon = (UIIcon *) e;
    UIButton *button = (UIButton *) e;

    if (icon->gamemode == GAMEMODE_SHIP) transform->y -= 4;

    if (icon->gamemode == TRAIL) {
        C2D_Sprite spr = { 0 };
        C2D_SpriteFromSheet(&spr, ui_2_sheet, FIRST_TRAIL_ID + icon->index);
        C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C2D_SpriteSetPos(&spr, transform->x, transform->y);
        C2D_SpriteSetScale(&spr, transform->scaleX,  transform->scaleY);
        C2D_DrawSprite(&spr);
    } else {
        spawn_icon_at(
            icon->gamemode,
            icon->index,
            icon->glow,
            transform->x, transform->y,
            0,
            0,
            0,
            transform->scaleX,
            icon->p1_color,
            icon->p2_color,
            icon->glow_color
        );
    }

    if (icon->isSelected) {
        C2D_SpriteSetCenter(&button->image.sprite, 0.5f, 0.5f);
        C2D_SpriteSetPos(&button->image.sprite, transform->x, transform->y);
        C2D_SpriteSetScale(&button->image.sprite, transform->scaleX, transform->scaleY);
        C2D_DrawSprite(&button->image.sprite);
    }
}

static void ui_icon_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

void ui_icon_set_selected(UIIcon *e, bool selected) {
    if (!e) return;

    e->isSelected = selected;
}

void ui_icon_set_gamemode_index(UIIcon *e, int gamemode, int index) {
    if (!e) return;

    e->gamemode = gamemode;
    e->index = index;
}

void ui_icon_set_p1(UIIcon *e, u32 color) {
    if (!e) return;

    e->p1_color = color;
}

void ui_icon_set_p2(UIIcon *e, u32 color) {
    if (!e) return;

    e->p2_color = color;
}

void ui_icon_set_glow(UIIcon *e, u32 color) {
    if (!e) return;

    e->glow_color = color;
}

UIIcon *ui_create_icon(const UIContext *ctx) {
    UIIcon *e = malloc(sizeof(UIIcon));

    if (!e) return NULL;

    UIButton *button = (UIButton *) e;

    memset(e, 0, sizeof(UIIcon));
    button->base.type = UI_ICON;
    button->base.enabled = true;

    button->base.update = ui_icon_update;
    button->base.draw = ui_icon_draw;
    button->base.destroy = ui_icon_destroy;

    button->base.modify_transform = ui_button_modify_transform;

    ui_element_apply_default_properties(&button->base, ctx);
    
    button->hoverScale = 1;
    button->hoverFactor = 1;

    C2D_SpriteFromSheet(&button->image.sprite, ui_sheet, 175);
    C3D_TexSetFilter(button->image.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    return e;
}

UIElement *ui_create_icon_from_props(const UIContext *ctx, const UIPropertyList *props) {
    UIIcon *icon = ui_create_icon(ctx);

    if (!icon) return NULL;

    UIButton *button = (UIButton *) icon;

    ui_element_apply_properties(&button->base, ctx, props);

    ui_element_set_size(&button->base, 30, 30);
    
    button->hoverFactor = ui_prop_float(props, "hoverFactor", 1);    

    ui_icon_set_gamemode_index(icon, 
        ui_prop_int(props, "gamemode", 0), 
        ui_prop_int(props, "id", 0));

    icon->glow = ui_prop_bool(props, "glow", false);

    ui_icon_set_p1(icon, ui_prop_color(props, "p1_color", ABGR8(175, 175, 175, 255)));
    ui_icon_set_p2(icon, ui_prop_color(props, "p2_color", ABGR8(255, 255, 255, 255)));
    ui_icon_set_glow(icon, ui_prop_color(props, "glow_color", ABGR8(255, 255, 255, 255)));

    return &button->base;
}