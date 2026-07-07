#pragma once
#include "ui_element.h"
#include <stdbool.h>

UIElement ui_create_online_level_card(int x, int y, bool swap_color, char *name, char *creator, char *song, char *length, int downloads, int likes, int stars, char (*tag)[TAG_LENGTH]);
