#pragma once

#include "ui_button.hpp"

#define COLOR_BUTTON_HOVER_SCALE BUTTON_HOVER_SCALE
#define COLOR_BUTTON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

void uiColorButtonSetIndex(UIColor *e, int index, int colorIndex);
UIColor *uiCreateColorButton(UIScreen *screen);
UIElement *uiCreateColorButtonFromProps(UIScreen *screen, const UIPropertyList *props);