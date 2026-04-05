#pragma once
#include "ui_element.h"

UIElement ui_create_external_level_card(int x, int y, int image, int sheet, char *text, char *level_path, UIActionFn action, char (*tag)[TAG_LENGTH]);
void ui_external_level_card_set_image(UIElement *e, int sprite_index, int sheet);