#pragma once
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"

void ui_label_set_text(UILabel *e, const char *text);
void ui_label_set_scale_from_width(UILabel *e, const char *text);
UILabel *ui_create_label(const UIContext *ctx);
UIElement *ui_create_label_from_props(const UIContext *ctx, const UIPropertyList *props);