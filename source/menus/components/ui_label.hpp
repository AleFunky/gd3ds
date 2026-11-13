#pragma once

#include "menus/core/ui_screen.hpp"

void uiLabelSetText(UILabel *e, const char *text);
UILabel *uiCreateLabel(UIScreen *screen);
UIElement *uiCreateLabelFromProps(UIScreen *screen, const UIPropertyList *props);