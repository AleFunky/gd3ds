#pragma once

#include <citro2d.h>
#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

UIPaletteIcons *uiCreatePaletteIcons(UIScreen *screen);
UIElement *uiCreatePaletteIconsFromProps(UIScreen *screen, const UIPropertyList *props);

}