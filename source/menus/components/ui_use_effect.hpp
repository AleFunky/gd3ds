#pragma once

#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "graphics.hpp"
#include "particles/circles.hpp"

namespace gd3ds
{

void uiUseEffectClear(UIUseEffect* e);
void uiUseEffectUpdatePos(UIUseEffect* e);
UseEffect *uiAddUseEffect(UIUseEffect* e, float x, float y, const UseEffectDefinition *def);
void uiSetUseEffectCol(UseEffect *effect, float r, float g, float b);
UIUseEffect *uiCreateUseEffect(UIScreen *screen);
UIElement *uiCreateUseEffectFromProps(UIScreen *screen, const UIPropertyList *props);

}