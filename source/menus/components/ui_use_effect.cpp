
#include <citro2d.h>
#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "graphics.hpp"
#include "particles/circles.hpp"

namespace gd3ds
{

void uiUseEffectClear(UIUseEffect* e){
    if (!e) return;

    UseEffectPool *pool = &(e->useEffects);
    clearUseEffects(pool);
}

void uiUseEffectUpdatePos(UIUseEffect* e) {
    if (!e) return;

    UseEffectPool *pool = &(e->useEffects);
    if (!pool->pool) return;

    for(size_t i = 0; i < pool->capacity; i++){
        pool->pool[i].x = e->base.x + e->xPos[i];
        pool->pool[i].y = e->base.y + e->yPos[i];
    }
}

UseEffect *uiAddUseEffect(UIUseEffect* e, float x, float y, const UseEffectDefinition *def) {
    if (!e) return NULL;

    UseEffectPool *pool = &(e->useEffects);
    UseEffect *effect = addUseEffect(x, y, USE_EFFECT_OBJ_NOTHING, def, pool);
    e->xPos[effect->index] = x - e->base.x;
    e->yPos[effect->index] = y - e->base.y;
    return effect;
}

void uiSetUseEffectCol(UseEffect *effect, float r, float g, float b) {
    if (!effect) return;
    
    effect->def.colorR = r;
    effect->def.colorG = g;
    effect->def.colorB = b;
}

static void uiUseEffectUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UIUseEffect *useEffect = (UIUseEffect *) e;

    UseEffectPool *pool = &(useEffect->useEffects);
    updateUseEffects(delta, pool);
}

static void uiUseEffectDraw(UIElement* e, UITransform *transform) {
    UIUseEffect *useEffect = (UIUseEffect *) e;

    UseEffectPool *pool = &(useEffect->useEffects);
    changeBlending(true);
    drawUseEffects(pool);
    changeBlending(false);
}

static void uiUseEffectDestroy(UIElement *e) {
    if (e) {
        UIUseEffect *useEffect = (UIUseEffect *) e;
        // Free pool
        if (useEffect->useEffects.pool) {
            free(useEffect->useEffects.pool);
        }

        delete useEffect;
        e = NULL;
    }
}

UIUseEffect *uiCreateUseEffect(UIScreen *screen) {
    UIUseEffect *e = new UIUseEffect{};

    if (!e) return NULL;

    UseEffectPool *pool = &(e->useEffects);
    initUseEffectPool(pool, MAX_UI_USE_EFFECTS);
    pool->stationary = true;
    for (size_t i = 0; i < pool->capacity; i++) {
        pool->pool[i].active = false;
        e->xPos[i] = 0;
        e->yPos[i] = 0;
    }
    
    uiElementApplyDefaultProperties(&e->base, screen);

    e->base.type = UI_USE_EFFECT;
    e->base.enabled = true;

    e->base.update = uiUseEffectUpdate;
    e->base.draw = uiUseEffectDraw;
    e->base.destroy = uiUseEffectDestroy;

    return e;
}

UIElement *uiCreateUseEffectFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIUseEffect *useEffect = uiCreateUseEffect(screen);

    if (!useEffect) return NULL;

    uiElementApplyProperties(&useEffect->base, screen, props);

    return &useEffect->base;
}

}