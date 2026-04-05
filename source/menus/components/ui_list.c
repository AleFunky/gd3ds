#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "ui_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"
#include "ui_list.h"
#include "utils/gfx.h"

void ui_list_reset(UIElement *list) {
    UIList* l = &list->list;
    l->scrollY = 0;
    l->itemCount = 0;
    l->contentHeight = 0;
    l->lastTouchY = 0;
}

void ui_list_add(UIElement* list, UIElement* item) {
    UIList* l = &list->list;

    if (l->itemCount >= UI_LIST_MAX_ITEMS) return;

    // Add item and increase content height
    l->items[l->itemCount++] = item;
    l->contentHeight += item->h;
}

static void ui_list_forward_touch(UIElement* list, UIInput* touch) {
    UIList* l = &list->list;

    float initial_y = list->y - (list->h / 2) + l->scrollY;
    
    for (int i = 0; i < l->itemCount; i++) {
        UIElement* item = l->items[i];
        // Set position
        item->y = initial_y + (item->h / 2) + (item->h) * i;

        if (!item->enabled) continue;

        item->update(item, touch);
    }
}

static void ui_list_update(UIElement* e, UIInput* touch) {
    UIList* l = &e->list;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    ui_list_forward_touch(e, touch);
    if (inside) {  
        touch->did_something = true;

        // Exit if used something
        if (touch->interacted) return;
    }

    // Start dragging
    if (inside && (hidKeysDown() & KEY_TOUCH)) {
        l->dragging = true;
        l->lastTouchY = touch->touchPosition.py;
    }

    // Handle dragging
    if (l->dragging && (hidKeysHeld() & KEY_TOUCH)) {
        // Apply touch movement
        int delta = touch->touchPosition.py - l->lastTouchY;
        l->scrollY += delta;
        l->lastTouchY = touch->touchPosition.py;
    }

    // Handle releasing dragging
    if (hidKeysUp() & KEY_TOUCH) {
        l->dragging = false;
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

static void ui_list_draw(UIElement* e) {
    UIList* l = &e->list;

    float x = e->x - (e->w / 2);
    float y = e->y - (e->h / 2);

    // Draw border
    C2D_DrawRectSolid(x, y, 0, e->w, e->h,
                      C2D_Color32(40,40,40,255));

    // Enable clipping
    set_scissor(GPU_SCISSOR_NORMAL, x, y, e->w, e->h);

    float initial_y = y + l->scrollY;

    for (int i = 0; i < l->itemCount; i++) {
        UIElement* item = l->items[i];

        // Set position
        item->x = e->x;
        item->y = initial_y + (item->h / 2) + (item->h) * i;
        item->w = e->w; // Set width to container width

        // Draw item
        item->draw(item);
    }

    // Disable scissor
    C2D_Flush();
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0,0,0,0);
    C2D_Prepare();
}

UIElement ui_create_list(
    int x, int y, int w, int h,
    char (*tag)[TAG_LENGTH]
) {
    UIElement e = {0};

    e.type = UI_LIST;
    e.x = x;
    e.y = y;
    e.w = w;
    e.h = h;
    e.enabled = true;

    // Copy tag
    copy_tag_array(&e, tag);

    e.update = ui_list_update;
    e.draw = ui_list_draw;
    
    ui_list_reset(&e);

    return e;
}