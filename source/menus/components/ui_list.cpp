#include "main.hpp"

#include <citro2d.h>
#include "ui_image.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "ui_button.hpp"
#include "easing.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "ui_list.hpp"
#include "utils/gfx.hpp"
#include <stdlib.h>

namespace gd3ds
{

void uiListReset(UIList *list) {
    if (!list) return;
    
    list->scrollY = 0;

    UIElement *child = list->base.firstChild;

    while (child) {
        UIElement *next = child->nextSibling;
        uiDestroyTree(child);
        child = next;
    }

    list->base.firstChild = NULL;
    list->base.lastChild = NULL;
    
    list->contentHeight = 0;
    list->lastTouchY = 0;
}


void uiListAdd(UIList* list, UIElement* item) {
    if (!list) return;

    uiElementAddChild((UIElement *) list, item);

    list->contentHeight += item->h;
}

void uiListSetBgColor(UIList *list, u32 color) {
    if (!list) return;

    list->backgroundColor = color;
}

static void uiListForwardTouch(UIList *list, UIInput *input, UITransform *transform) {
    UITransform child = *transform;
    child.y += list->scrollY;

    float y = list->scrollY - list->base.h * 0.5f;

    for (UIElement *item = list->base.firstChild; item; item = item->nextSibling) {
        UITransform t = *transform;
        t.y += y + (item->h * 0.5f);

        uiUpdateTree(item, input, &t);

        y += item->h;
    }
}

static void uiListUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UIList* l = (UIList *) e;

    bool inside = uiElementBasicBoundCheck(e, touch, transform);

    uiListForwardTouch(l, touch, transform);
    if (inside) {  
        // Exit if used something
        if (touch->interacted) return;
    }

    // Start dragging
    if (inside && (touch->down & KEY_TOUCH)) {
        l->dragging = true;
        touch->dragging = true;
        l->lastTouchY = touch->touchPos.py;
    }

    // Handle dragging
    if (l->dragging && (touch->held & KEY_TOUCH)) {
        // Apply touch movement
        int delta = touch->touchPos.py - l->lastTouchY;
        l->scrollY += delta;
        l->lastTouchY = touch->touchPos.py;
    }

    if(abs(touch->cpad.dy) > 48){
        l->scrollY += (touch->cpad.dy / 25);
    }

    if(touch->down & (KEY_UP | KEY_DOWN)){
        l->dpadHeldTime = 0;
    }

    if(l->dpadHeldTime > 60){
        l->dpadHeldTime = 60;
    }

    if(touch->held & KEY_UP){
        l->scrollY += 4;
        if(l->dpadHeldTime >= 60){
            l->scrollY += 4;
        }

        l->dpadHeldTime++;
    }
    if(touch->held & KEY_DOWN){
        l->scrollY -= 4;
        if(l->dpadHeldTime >= 60){
            l->scrollY -= 4;
        }

        l->dpadHeldTime++;
    }

    // Handle releasing dragging
    if (touch->up & KEY_TOUCH) {
        l->dragging = false;
        touch->dragging = false;
    }

    // Clamp scrolling
    int minScroll = e->h - l->contentHeight;
    if (l->contentHeight <= e->h) {
        l->scrollY = 0;
    } else {
        if (l->scrollY > 0) l->scrollY = 0;
        if (l->scrollY < minScroll) l->scrollY = minScroll;
    }
}

static void uiListDraw(UIElement* e, UITransform *transform) {
    UIList* l = (UIList *) e;

    float width = e->w * transform->scaleX;
    float height = e->h * transform->scaleY;

    float scissorX = transform->x - (width / 2);
    float scissorY = transform->y - (height / 2);

    // Draw background
    C2D_DrawRectSolid(scissorX, scissorY, 0, width, height, l->backgroundColor);
    
    // Enable clipping
    setScissor(GPU_SCISSOR_NORMAL, scissorX, scissorY, width, height);

    float y = l->scrollY - e->h * 0.5f;

    for (UIElement *item = e->firstChild; item; item = item->nextSibling) {
        UITransform t = *transform;
        t.y += (y + (item->h * 0.5f)) * transform->scaleY;

        item->w = e->w;

        uiDrawTree(item, &t);

        y += item->h;
    }

    // Disable scissor
    C2D_Flush();
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0,0,0,0);
    C2D_Prepare();
}

static void uiListDestroy(UIElement *e) {
    if (e) {
        delete (UIList *)e;
        e = NULL;
    }
}

UIList *uiCreateList(UIScreen *screen) {
    UIList *e = new UIList{};

    if (!e) return NULL;

    e->base.type = UI_LIST;
    e->base.enabled = true;

    e->base.update = uiListUpdate;
    e->base.draw = uiListDraw;
    e->base.destroy = uiListDestroy;

    e->base.drawsChildren = true;
    
    uiListReset(e);

    return e;
}

UIElement *uiCreateListFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIList *list = uiCreateList(screen);

    if (!list) return NULL;
    
    uiElementApplyProperties(&list->base, screen, props);

    uiListSetBgColor(list, uiPropColor(props, "bgColor", ABGR8(0, 0, 0, 0)));

    return &list->base;
}

}