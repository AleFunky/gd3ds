#pragma once


#include "menus/core/ui_screen.hpp"
#include <stdbool.h>

namespace gd3ds
{

extern bool sliding;

UISlider *uiCreateSlider(UIScreen *screen);
UIElement *uiCreateSliderFromProps(UIScreen *screen, const UIPropertyList *props);
float uiSliderGetPercent(UISlider *e);
void uiSliderSetValue(UISlider *e, float value);

}