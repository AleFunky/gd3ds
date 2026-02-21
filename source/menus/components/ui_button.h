#pragma once
#include "ui_element.h"


#define BUTTON_HOVER_SCALE 1.25f
#define BUTTON_HOVER_ANIM_TIME 0.5f

UIElement ui_create_button(
    int x, int y, int sprite_index, 
    UIActionFn action,
    void *action_data,
    char *text,
    char *tag
);