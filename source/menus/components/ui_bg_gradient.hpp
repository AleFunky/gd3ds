#pragma once

#include "menus/core/ui_screen.hpp"

#define BG_GRADIENT_XSCALE (400.f / 30)
#define BG_GRADIENT_YSCALE (240.f / 320)

namespace gd3ds
{

UIImage *uiCreateBgGradient(UIScreen *screen);
UIElement *uiCreateBgGradientFromProps(UIScreen *screen, const UIPropertyList *props);

}