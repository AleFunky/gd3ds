#pragma once

#include "menus/core/ui_screen.hpp"
#define TEXTBOX_STYLE 2
#define TEXTBOX_MARGIN 10

UITextbox *uiCreateTextbox(UIScreen *screen);
UIElement *uiCreateTextboxFromProps(UIScreen *screen, const UIPropertyList *props);