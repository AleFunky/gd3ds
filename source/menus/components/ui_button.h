#pragma once

#include "menus/core/ui_screen.h"

#define BUTTON_HOVER_SCALE 1.25f
#define BUTTON_HOVER_ANIM_TIME 0.4f

void ui_button_draw_text(UIElement *e, UITransform *transform);
void ui_button_update(UIElement* e, UIInput* touch, UITransform *transform);

void ui_button_set_image(UIButton *e, int sprite_index, int sheet);
void ui_button_set_text(UIButton *e, const char *text);

void ui_button_modify_transform(UIElement *e, UITransform *t);

UIButton *ui_create_button(UIScreen *screen);
UIElement *ui_create_button_from_props(UIScreen *screen, const UIPropertyList *props);