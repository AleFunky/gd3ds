#include "menus/core/ui_element.h"
#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"

void ui_darken_set_opacity(UIDarken* e, float opacity) {
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(0, 0, 0, opacity), 1.0f);
}

void ui_darken_reset_opacity(UIDarken* e){
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(0, 0, 0, e->base.opacity), 1.0f);
}

static void ui_darken_update(UIElement* e, UIInput* touch, UITransform *transform) {
    UIDarken *darken = (UIDarken *) e;

    bool inside = ui_element_basic_bound_check(e, touch, transform);
    
    // Mask background elements
    if (inside) touch->did_something = true;

    if(!darken->darkenOver){
        e->opacity = (darken->darkenTimeElapsed / darken->darkenTime) * darken->targetOpacity;
        darken->darkenTimeElapsed += 1.f / 60.f;
    }

    if(darken->darkenTimeElapsed > darken->darkenTime && !darken->darkenOver){
        darken->darkenOver = true;
    }
}

static void ui_darken_draw(UIElement* e, UITransform *transform) {
    UIDarken *darken = (UIDarken *) e;

    if(!darken->darkenOver){
        ui_darken_reset_opacity(darken);
    }

    if (darken->fullScreen) {
        C2D_SpriteSetPos(&darken->image.sprite, SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
        C2D_SpriteSetScale(&darken->image.sprite, SCREEN_WIDTH/16.f, SCREEN_HEIGHT/16.f);
    } else {
        C2D_SpriteSetPos(&darken->image.sprite, transform->x, transform->y);
        C2D_SpriteSetScale(&darken->image.sprite, e->w/16.f, e->h/16.f);
    }
    C2D_DrawSpriteTinted(&darken->image.sprite, &darken->image.tint);
}

static void ui_darken_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UIDarken *ui_create_darken(const UIContext *ctx) {
    UIDarken *e = malloc(sizeof(UIDarken));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIDarken));
    e->base.type = UI_DARKEN;
    e->base.enabled = true;
    e->base.opacity = 0.0f;

    e->darkenTime = 0.1f;
    e->darkenTimeElapsed = 0.f;
    e->darkenOver = false;
    e->targetOpacity = 0.4f;
    
    ui_element_apply_default_properties(&e->base, ctx);
    
    C2D_SpriteFromSheet(&e->image.sprite, ui_sheet, 416);
    C2D_SpriteSetCenter(&e->image.sprite, 0.5f, 0.5f);

    e->base.update = ui_darken_update;
    e->base.draw = ui_darken_draw;
    e->base.destroy = ui_darken_destroy;

    return e;
}

UIElement *ui_create_darken_from_props(const UIContext *ctx, const UIPropertyList *props) {
    UIDarken *darken = ui_create_darken(ctx);

    if (!darken) return NULL;

    ui_element_apply_properties(&darken->base, ctx, props);

    if (darken->base.w == 0 || darken->base.h == 0) {
        darken->fullScreen = true;
    }

    float darkenTime = ui_prop_float(props, "darkenTime", 0.1f);
    float opacity = ui_prop_float(props, "opacity", 0.4f);
    
    if (darkenTime <= 0.f) {    
        darken->base.opacity = opacity;
        darken->darkenOver = true;
    } else {
        darken->darkenTime = darkenTime;
        darken->darkenTimeElapsed = 0.f;
        darken->darkenOver = false;
        darken->base.opacity = 0.f;
        darken->targetOpacity = opacity;
    }

    C2D_PlainImageTint(&darken->image.tint, C2D_Color32f(0.f, 0.f, 0.f, darken->base.opacity), 1.0f);

    return &darken->base;
}