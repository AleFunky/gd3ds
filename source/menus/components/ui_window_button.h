#pragma once
#include "ui_button.h"
#include "ui_element.h"


#define WINDOW_BUTTON_HOVER_SCALE BUTTON_HOVER_SCALE
#define WINDOW_BUTTON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

void ui_window_button_set_style(UIElement *e, int style);
UIElement ui_create_window_button(
    int x, int y, float w, float h, int style,
    UIActionFn action,
    char *text,
    char (*tag)[TAG_LENGTH],
    float textScale
);