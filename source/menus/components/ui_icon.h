#pragma once

#include "ui_button.h"


#define ICON_HOVER_SCALE BUTTON_HOVER_SCALE
#define ICON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

void ui_icon_set_selected(UIIcon *e, bool selected);
void ui_icon_set_p1(UIIcon *e, u32 color);
void ui_icon_set_p2(UIIcon *e, u32 color);
void ui_icon_set_glow(UIIcon *e, u32 color);

void ui_icon_set_gamemode_index(UIIcon *e, int gamemode, int index);
UIIcon *ui_create_icon(UIScreen *screen);
UIElement *ui_create_icon_from_props(UIScreen *screen, const UIPropertyList *props);