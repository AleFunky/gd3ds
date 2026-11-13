#pragma once
#include "ui_button.hpp"



#define WINDOW_BUTTON_HOVER_SCALE BUTTON_HOVER_SCALE
#define WINDOW_BUTTON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

namespace gd3ds
{

void uiWindowButtonSetTint(UIWindowButton* e, u32 color);
void uiWindowButtonSetStyle(UIWindowButton *e, int style);
UIWindowButton *uiCreateWindowButton(UIScreen *screen);
UIElement *uiCreateWindowButtonFromProps(UIScreen *screen, const UIPropertyList *props);

}