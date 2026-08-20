#pragma once
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"

UISpinner *ui_create_spinner(const UIContext *ctx);
UIElement *ui_create_spinner_from_props(const UIContext *ctx, const UIPropertyList *props);