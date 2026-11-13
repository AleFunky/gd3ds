#pragma once

#include "ui_button.hpp"

#define CHECKBOX_HOVER_SCALE BUTTON_HOVER_SCALE
#define CHECKBOX_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

UICheckBox *uiCreateCheckbox(UIScreen *screen);
UIElement *uiCreateCheckboxFromProps(UIScreen *screen, const UIPropertyList *props);
void uiSetCheckboxChecked(UICheckBox *e, bool enabled);