#pragma once
#include "ui_element.hpp"

void ui_element_set_position(UIElement *e, float x, float y);
void ui_element_set_size(UIElement *e, int w, int h);
void ui_element_set_enabled(UIElement *e, bool enabled);

void ui_element_set_scale(UIElement *e, float scale);
void ui_element_set_scale_x(UIElement *e, float scale);
void ui_element_set_scale_y(UIElement *e, float scale);
void ui_element_set_scale_xy(UIElement *e, float x, float y);

void ui_element_set_action(UIElement *e, UIActionFn action);