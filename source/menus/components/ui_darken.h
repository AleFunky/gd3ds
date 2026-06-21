#pragma once
#include "ui_element.h"

void ui_darken_reset_opacity(UIElement *e);
UIElement ui_create_darken(float x, float y, float width, float height, float opacity, float darkenTime, char (*tag)[TAG_LENGTH]);