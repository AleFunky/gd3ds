
#include <citro2d.h>
#include "game/game_manager.hpp"
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
    updateUseEffects(GameManager::get()->deltaTime, pool);
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
        if (useEffect->useEffects.pool) {
            free(useEffect->useEffects.pool);
            useEffect->useEffects.pool = NULL;
        }
    }
}

UIUseEffect *uiCreateUseEffect(UIScreen *screen) {
    auto e = std::make_unique<UIUseEffect>();

    UIUseEffect *result = e.get();

    UseEffectPool *pool = &(result->useEffects);
    initUseEffectPool(pool, MAX_UI_USE_EFFECTS);
    pool->stationary = true;
    for (size_t i = 0; i < pool->capacity; i++) {
        pool->pool[i].active = false;
        result->xPos[i] = 0;
        result->yPos[i] = 0;
    }
    
    uiElementApplyDefaultProperties(&result->base, screen);

    result->base.type = UIElementType::UiUseEffect;
    result->base.enabled = true;

    result->base.update = uiUseEffectUpdate;
    result->base.draw = uiUseEffectDraw;
    result->base.destroy = uiUseEffectDestroy;

    return e.release();
}

UIElement *uiCreateUseEffectFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIUseEffect *useEffect = uiCreateUseEffect(screen);

    if (!useEffect) return NULL;

    uiElementApplyProperties(&useEffect->base, screen, props);

    return &useEffect->base;
}

}