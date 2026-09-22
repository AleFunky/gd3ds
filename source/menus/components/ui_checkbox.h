#pragma once

#include "ui_button.h"

#define CHECKBOX_HOVER_SCALE BUTTON_HOVER_SCALE
#define CHECKBOX_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

UICheckBox *ui_create_checkbox(UIScreen *screen);
UIElement *ui_create_checkbox_from_props(UIScreen *screen, const UIPropertyList *props);
void ui_set_checkbox_checked(UICheckBox *e, bool enabled);