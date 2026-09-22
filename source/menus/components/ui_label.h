#pragma once

#include "menus/core/ui_screen.h"

void ui_label_set_text(UILabel *e, const char *text);
UILabel *ui_create_label(UIScreen *screen);
UIElement *ui_create_label_from_props(UIScreen *screen, const UIPropertyList *props);