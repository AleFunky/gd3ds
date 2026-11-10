#pragma once

#include "menus/core/ui_screen.hpp"
#define TEXTBOX_STYLE 2
#define TEXTBOX_MARGIN 10

UITextbox *ui_create_textbox(UIScreen *screen);
UIElement *ui_create_textbox_from_props(UIScreen *screen, const UIPropertyList *props);