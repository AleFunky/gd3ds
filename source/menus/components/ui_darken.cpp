
#include <citro2d.h>
#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"

namespace gd3ds
{

void uiDarkenSetOpacity(UIDarken* e, float opacity) {
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(0, 0, 0, opacity), 1.0f);
}

void uiDarkenResetOpacity(UIDarken* e){
    if (!e) return;

    C2D_PlainImageTint(&e->image.tint, C2D_Color32f(0, 0, 0, e->base.opacity), 1.0f);
}

static void uiDarkenUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UIDarken *darken = (UIDarken *) e;

    //there's not really a reason to animate non-fullscreen darkens tbh
    if(darken->fullScreen){
        //hehe trans
        const UITransition *trans = &e->screen->transition;

        float duration = trans->duration * trans->darkenFrac;
        float time = trans->time;
        if(time > duration){
            time = duration;
        }

        if(duration == 0.f){
            duration = 1.f;
        }

        if(!e->screen->transition.done){
            float fade = 0.f;
            switch(e->screen->transition.state){
                case UI_TRANSITION_OPENING:
                    if(trans->inDuration > 0.f){
                        fade = time / duration;
                    }
                    break;
                case UI_TRANSITION_CLOSING:
                    if(trans->outDuration > 0.f){
                        fade = 1.f - (time / duration);
                    }
                    break;
                case UI_TRANSITION_NONE:
                    fade = 0.f;
                    break;
            }

            if(fade < 0.f){
                fade = 0.f;
            }

            e->opacity = fade * darken->opacity;
        }
    }
}

static void uiDarkenDraw(UIElement* e, UITransform *transform) {
    UIDarken *darken = (UIDarken *) e;

    if(!e->screen->transition.done){
        uiDarkenResetOpacity(darken);
    }

    if (darken->fullScreen) {
        C2D_SpriteSetPos(&darken->image.sprite, SCREEN_WIDTH/2.f, SCREEN_HEIGHT/2.f);
        C2D_SpriteSetScale(&darken->image.sprite, SCREEN_WIDTH/16.f, SCREEN_HEIGHT/16.f);
    } else {
        C2D_SpriteSetPos(&darken->image.sprite, transform->x, transform->y);
        C2D_SpriteSetScale(&darken->image.sprite, e->w/16.f, e->h/16.f);
    }
    C2D_DrawSpriteTinted(&darken->image.sprite, &darken->image.tint);
}

static void uiDarkenDestroy(UIElement *e) {
    if (e) {
        delete (UIDarken *)e;
        e = NULL;
    }
}

UIDarken *uiCreateDarken(UIScreen *screen) {
    UIDarken *e = new UIDarken{};

    if (!e) return NULL;

    e->base.type = UI_DARKEN;
    e->base.enabled = true;
    e->base.opacity = 0.0f;

    e->opacity = 0.4f;
    
    uiElementApplyDefaultProperties(&e->base, screen);
    
    C2D_SpriteFromSheet(&e->image.sprite, uiSheet, 416);
    C2D_SpriteSetCenter(&e->image.sprite, 0.5f, 0.5f);

    e->base.update = uiDarkenUpdate;
    e->base.draw = uiDarkenDraw;
    e->base.destroy = uiDarkenDestroy;

    return e;
}

UIElement *uiCreateDarkenFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIDarken *darken = uiCreateDarken(screen);

    if (!darken) return NULL;

    uiElementApplyProperties(&darken->base, screen, props);

    if (darken->base.w == 0 || darken->base.h == 0) {
        darken->fullScreen = true;
    }

    darken->opacity = uiPropFloat(props, "opacity", 0.4f);

    C2D_PlainImageTint(&darken->image.tint, C2D_Color32f(0.f, 0.f, 0.f, darken->base.opacity), 1.0f);

    return &darken->base;
}

}