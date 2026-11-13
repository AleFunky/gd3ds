#include "ui_element.hpp"
#include "ui_screen.hpp"

static_assert(
    offsetof(UIElement, update) == 0xFC,
    "Unexpected UIElement layout!"
);

void uiElementSetUserdata(UIElement *element, void *userdata) {
    element->userdata = userdata;
}

bool uiElementBasicBoundCheck(UIElement *e, UIInput *touch, UITransform *transform) {
    if (touch->touchPos.px == 0 && touch->touchPos.py == 0) return false;

    float width  = (e->w * 0.5f) * fabsf(transform->scaleX);
    float height = (e->h * 0.5f) * fabsf(transform->scaleY);

    return touch->touchPos.px >= transform->x - width  && touch->touchPos.px < transform->x + width &&
           touch->touchPos.py >= transform->y - height && touch->touchPos.py < transform->y + height;
}

// Combines a transform an element's data
UITransform uiTransformCombine(UITransform *parent, UIElement *e) {
    return (UITransform){
        .x = parent->x + e->x * parent->scaleX,
        .y = parent->y + e->y * parent->scaleY,

        .scaleX = parent->scaleX * e->scaleX,
        .scaleY = parent->scaleY * e->scaleY,
    };
}

// Updates an element and its children
void uiUpdateTree(UIElement *e, UIInput *input, UITransform *parent) {
    if (!e->enabled) return;

    UITransform world = uiTransformCombine(parent, e);
    
    // Let the element modify its own transform
    if (e->modifyTransform)
        e->modifyTransform(e, &world);
    
    if(e->update){
        e->update(e, input, &world);
    }

    if (!e->drawsChildren) {
        for (UIElement *child = e->firstChild; child; child = child->nextSibling) {
            uiUpdateTree(child, input, &world);
        }
    }
}

// Draws an element and its children
void uiDrawTree(UIElement *e, UITransform *parent) {
    if (!e->enabled) return;

    UITransform world = uiTransformCombine(parent, e);

    // Let the element modify its own transform
    if (e->modifyTransform)
        e->modifyTransform(e, &world);

    if(e->draw) e->draw(e, &world);

    if (!e->drawsChildren) {
        for (UIElement *child = e->firstChild; child; child = child->nextSibling) {
            uiDrawTree(child, &world);
        }
    }
}

// Destroys an element and its children
void uiDestroyTree(UIElement *e) {
    UIElement *child = e->firstChild;

    while (child) {
        UIElement *next = child->nextSibling;

        uiDestroyTree(child);

        child = next;
    }

    if (e->userdata && e->userdataDestroy) e->userdataDestroy(e->userdata);

    if(e->actions){
        for(size_t i = 0; i < e->actionCount; i++){
            uiDestroyProplist(&e->actions[i].args);
        }
        free(e->actions);
    }

    uiDestroyProplist(&e->customProperties);
    
    e->destroy(e);
}

void uiEnableElement(UIElement *e) { 
    e->enabled = true;

    if (e->onEnable) {
        e->onEnable(e);
    }
}

void uiDisableElement(UIElement *e) { 
    e->enabled = false;
    
    if (e->onDisable) {
        e->onDisable(e);
    }
}

void uiElementAddChild(UIElement *parent, UIElement *child) {
    if (!parent || !child) return;

    child->parent = parent;

    child->nextSibling = NULL;
    child->prevSibling = parent->lastChild;

    if (parent->lastChild)
        parent->lastChild->nextSibling = child;
    else
        parent->firstChild = child;

    parent->lastChild = child;
}

void uiElementRemove(UIElement *element) {
    if (!element->parent) return;

    UIElement *parent = element->parent;

    if (element->prevSibling)
        element->prevSibling->nextSibling = element->nextSibling;
    else
        parent->firstChild = element->nextSibling;

    if (element->nextSibling)
        element->nextSibling->prevSibling = element->prevSibling;
    else
        parent->lastChild = element->prevSibling;

    element->parent = NULL;
    element->nextSibling = NULL;
    element->prevSibling = NULL;
}

UIElement *uiGetChildByType(UIElement *parent, UIElementType type) {
    for (UIElement *child = parent->firstChild; child; child = child->nextSibling) {
        if (child->type == type) {
            return child;
        }
    }

    return NULL;
}

UIActionFn uiFindAction(const UIActionDef* actions, size_t count, const char* name) {
    for (size_t i = 0; i < count; i++)
        if (strcmp(actions[i].name, name) == 0)
            return actions[i].fn;
    return NULL;
}

void uiElementApplyProperties(UIElement *e, UIScreen *screen, const UIPropertyList *props) {
    if (!e || !screen || !props) return;

    uiElementSetPosition(e, 
        uiPropFloat(props, "x", e->x), 
        uiPropFloat(props, "y", e->y));

    uiElementSetSize(e, 
        uiPropInt(props, "w", e->w), 
        uiPropInt(props, "h", e->h));
    
    float scale = uiPropFloat(props, "scale", 1.0f);
    uiElementSetScaleXy(e,
        uiPropFloat(props, "sx", scale),
        uiPropFloat(props, "sy", scale)
    );

    // Copy tag
    copyTagArray(e, uiPropString(props, "tag", ""));
    
    e->opacity = uiPropFloat(props, "opacity", 1);

    const UIActionDef* actions = (screen->def && screen->def->actionList.actions) ? screen->def->actionList.actions : NULL;
    const size_t actionCount = (screen->def && screen->def->actionList.actions) ? screen->def->actionList.actionCount : 0;

    e->action = uiFindAction(
        actions, actionCount,
        uiPropString(props, "action", "")
    );

    e->actions = uiPropActions(
        props, 
        actions, actionCount,
        "actions",
        &e->actionCount
    );

    e->customProperties = uiPropList(props, "custom");
}

void uiElementApplyDefaultProperties(UIElement *e, UIScreen *screen) {
    if (!e || !screen) return;

    uiElementSetScale(e, 1);

    e->opacity = 1;

    e->screen = screen;

    e->userdataDestroy = free;
}

void performActions(UIElement *e){
    if (e->action){
        e->action(e, NULL);
    }

    if(e->actions){
        for(size_t i = 0; i < e->actionCount; i++){
            if(e->actions[i].action){
                e->actions[i].action(e, &e->actions[i].args);
            }
        }
    }
}