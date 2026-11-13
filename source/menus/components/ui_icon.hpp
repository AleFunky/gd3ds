#pragma once

#include "ui_button.hpp"


#define ICON_HOVER_SCALE BUTTON_HOVER_SCALE
#define ICON_HOVER_ANIM_TIME BUTTON_HOVER_ANIM_TIME

namespace gd3ds
{

void uiIconSetSelected(UIIcon *e, bool selected);
void uiIconSetP1(UIIcon *e, u32 color);
void uiIconSetP2(UIIcon *e, u32 color);
void uiIconSetGlow(UIIcon *e, u32 color);

void uiIconSetGamemodeIndex(UIIcon *e, int gamemode, int index);
UIIcon *uiCreateIcon(UIScreen *screen);
UIElement *uiCreateIconFromProps(UIScreen *screen, const UIPropertyList *props);

}