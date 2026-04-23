#pragma once
#include "ui_element.h"


#define BUTTON_HOVER_SCALE 1.25f
#define BUTTON_HOVER_ANIM_TIME 0.4f

void ui_button_set_image(UIElement *e, int sprite_index, int sheet);
UIElement ui_create_button(
    int x, int y, float sx, float sy, int sprite_index, int sheet, float opacity,
    UIActionFn action,
    char *text,
    char (*tag)[TAG_LENGTH],
    float textScale
);