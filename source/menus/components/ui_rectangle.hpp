#pragma once

#include "menus/core/ui_screen.hpp"

void uiRectangleSetColor(UIRectangle* e, u32 color);
void uiRectangleClearColor(UIRectangle* e);

UIRectangle *uiCreateRectangle(UIScreen *screen);
UIElement *uiCreateRectangleFromProps(UIScreen *screen, const UIPropertyList *props);