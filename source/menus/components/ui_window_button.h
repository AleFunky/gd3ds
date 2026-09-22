#pragma once
#include "ui_button.h"



#define WINDOW_BUTTON_HOVER_SCALE BUTTON_HOVER_SCALE
#define WINDOW_BUTTON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

void ui_window_button_set_tint(UIWindowButton* e, u32 color);
void ui_window_button_set_style(UIWindowButton *e, int style);
UIWindowButton *ui_create_window_button(UIScreen *screen);
UIElement *ui_create_window_button_from_props(UIScreen *screen, const UIPropertyList *props);