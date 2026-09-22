#pragma once

#include <citro2d.h>
#include "menus/core/ui_screen.h"

UIPaletteIcons *ui_create_palette_icons(UIScreen *screen);
UIElement *ui_create_palette_icons_from_props(UIScreen *screen, const UIPropertyList *props);