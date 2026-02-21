#pragma once
#include "ui_element.h"

#define CHECKBOX_HOVER_SCALE 1.25f
#define CHECKBOX_HOVER_ANIM_TIME 0.5f

UIElement ui_create_checkbox(
    int x, int y, bool enabled,
    UIActionFn action,
    void *action_data,
    char *tag
);
void set_checkbox_enabled(UIElement *e, bool enabled);