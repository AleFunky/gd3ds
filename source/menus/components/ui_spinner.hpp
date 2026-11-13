#pragma once

#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

UISpinner *uiCreateSpinner(UIScreen *s);
UIElement *uiCreateSpinnerFromProps(UIScreen *s, const UIPropertyList *props);

}