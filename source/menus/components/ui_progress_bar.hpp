#pragma once

#include "menus/core/ui_screen.hpp"

void uiProgressBarSetTint(UIProgressBar* e, u32 color);
void uiProgressBarClearTint(UIProgressBar* e);
UIProgressBar *uiCreateProgressBar(UIScreen *screen);
UIElement *uiCreateProgressBarFromProps(UIScreen *screen, const UIPropertyList *props);