#pragma once
#include "menus/core/ui_element.h"
#include "ui_button.h"

#define CHECKBOX_HOVER_SCALE BUTTON_HOVER_SCALE
#define CHECKBOX_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

UICheckBox *ui_create_checkbox(const UIContext *ctx);
UIElement *ui_create_checkbox_from_props(const UIContext *ctx, const UIPropertyList *props);
void ui_set_checkbox_checked(UICheckBox *e, bool enabled);