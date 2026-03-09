#pragma once
#include "ui_element.h"


#define BUTTON_HOVER_SCALE 1.25f
#define BUTTON_HOVER_ANIM_TIME 0.4f

UIElement ui_create_button(
    int x, int y, float sx, float sy, int sprite_index, int sheet, 
    UIActionFn action,
    char *text,
    char (*tag)[TAG_LENGTH]
);