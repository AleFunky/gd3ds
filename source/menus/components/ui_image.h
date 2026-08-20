#pragma once
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"

void ui_image_set_tint(UIImage* e, u32 color);
void ui_image_clear_tint(UIImage* e);
void ui_image_set_image(UIImage *e, int sprite_index, int sheet);

void ui_image_update(UIElement* e, UIInput* touch, UITransform *transform);
void ui_image_draw(UIElement* e, UITransform *transform);

UIImage *ui_create_image(const UIContext *ctx);
UIElement *ui_create_image_from_props(const UIContext *ctx, const UIPropertyList *props);