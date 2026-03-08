#pragma once
#include "ui_element.h"

void ui_image_set_tint(UIElement* e, u32 color);
void ui_image_clear_tint(UIElement* e);
void ui_image_set_image(UIElement *e, int sprite_index, int sheet);
UIElement ui_create_image(int x, int y, int sprite_index, int sheet, float sx, float sy, char (*tag)[TAG_LENGTH]);