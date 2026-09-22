#pragma once

#include "menus/core/ui_screen.h"

void ui_rectangle_set_color(UIRectangle* e, u32 color);
void ui_rectangle_clear_color(UIRectangle* e);

UIRectangle *ui_create_rectangle(UIScreen *screen);
UIElement *ui_create_rectangle_from_props(UIScreen *screen, const UIPropertyList *props);