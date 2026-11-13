#include "ui_screen.hpp"
#include "ui_element.hpp"

void uiElementSetPosition(UIElement *e, float x, float y) {
    e->x = x;
    e->y = y;
}

void uiElementSetSize(UIElement *e, int w, int h) {
    e->w = w;
    e->h = h;
}

void uiElementSetEnabled(UIElement *e, bool enabled) {
    e->enabled = enabled;
}

void uiElementSetScale(UIElement *e, float scale) {
    uiElementSetScaleXy(e, scale, scale);
}

void uiElementSetScaleX(UIElement *e, float scale) {
    uiElementSetScaleXy(e, scale, e->scaleY);
}

void uiElementSetScaleY(UIElement *e, float scale) {
    uiElementSetScaleXy(e, e->scaleX, scale);
}

void uiElementSetScaleXy(UIElement *e, float x, float y) {
    e->scaleX = x;
    e->scaleY = y;
}

void uiElementSetAction(UIElement *e, UIActionFn action) {
    e->action = action;
}