#pragma once


#include "menus/core/ui_screen.h"
#include <stdbool.h>

extern bool sliding;

UISlider *ui_create_slider(UIScreen *screen);
UIElement *ui_create_slider_from_props(UIScreen *screen, const UIPropertyList *props);
float ui_slider_get_percent(UISlider *e);
void ui_slider_set_value(UISlider *e, float value);