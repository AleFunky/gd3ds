#pragma once

#include "menus/core/ui_screen.h"

#define BG_GRADIENT_XSCALE (400.f / 30)
#define BG_GRADIENT_YSCALE (240.f / 320)

UIImage *ui_create_bg_gradient(UIScreen *screen);
UIElement *ui_create_bg_gradient_from_props(UIScreen *screen, const UIPropertyList *props);