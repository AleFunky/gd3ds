#pragma once

#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "graphics.h"
#include "particles/circles.h"

void ui_use_effect_clear(UIUseEffect* e);
void ui_use_effect_update_pos(UIUseEffect* e);
UseEffect *ui_add_use_effect(UIUseEffect* e, float x, float y, const UseEffectDefinition *def);
void ui_set_use_effect_col(UseEffect *effect, float r, float g, float b);
UIUseEffect *ui_create_use_effect(UIScreen *screen);
UIElement *ui_create_use_effect_from_props(UIScreen *screen, const UIPropertyList *props);