#pragma once

#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

void uiDarkenSetOpacity(UIDarken* e, float opacity);
void uiDarkenResetOpacity(UIDarken *e);
UIDarken *uiCreateDarken(UIScreen *screen);
UIElement *uiCreateDarkenFromProps(UIScreen *screen, const UIPropertyList *props);

}