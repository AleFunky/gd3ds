#pragma once
#include "ui_element.h"
#include "ui_button.h"

#define CHECKBOX_HOVER_SCALE BUTTON_HOVER_SCALE
#define CHECKBOX_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

UIElement ui_create_checkbox(
    int x, int y, float sx, float sy, bool enabled,
    UIActionFn action,
    char (*tag)[TAG_LENGTH]
);
void set_checkbox_enabled(UIElement *e, bool enabled);