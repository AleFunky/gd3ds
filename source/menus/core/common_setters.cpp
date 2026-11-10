#include "ui_screen.hpp"
#include "ui_element.hpp"

void ui_element_set_position(UIElement *e, float x, float y) {
    e->x = x;
    e->y = y;
}

void ui_element_set_size(UIElement *e, int w, int h) {
    e->w = w;
    e->h = h;
}

void ui_element_set_enabled(UIElement *e, bool enabled) {
    e->enabled = enabled;
}

void ui_element_set_scale(UIElement *e, float scale) {
    ui_element_set_scale_xy(e, scale, scale);
}

void ui_element_set_scale_x(UIElement *e, float scale) {
    ui_element_set_scale_xy(e, scale, e->scaleY);
}

void ui_element_set_scale_y(UIElement *e, float scale) {
    ui_element_set_scale_xy(e, e->scaleX, scale);
}

void ui_element_set_scale_xy(UIElement *e, float x, float y) {
    e->scaleX = x;
    e->scaleY = y;
}

void ui_element_set_action(UIElement *e, UIActionFn action) {
    e->action = action;
}