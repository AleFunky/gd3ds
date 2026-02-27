#pragma once
#include "ui_element.h"
#define TEXTBOX_STYLE 2
#define TEXTBOX_MARGIN 10
UIElement ui_create_textbox(
    int x, int y, int w, char *title,
    char (*tag)[TAG_LENGTH]
);