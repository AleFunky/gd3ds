#pragma once

#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

void uiWindowSetAtlas(UIWindow* e, int index);
void uiWindowSetTint(UIWindow* e, u32 color);
UIWindow *uiCreateWindow(UIScreen *screen);
UIElement *uiCreateWindowFromProps(UIScreen *screen, const UIPropertyList *props);

}