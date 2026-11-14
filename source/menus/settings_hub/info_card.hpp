#pragma once
#include <3ds.h>
#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

struct InfoCardData {
    bool copied;
    const char *text;
    bool customTitle;
    const char *title;
};

extern const UIScreenDefPair infoCardDef;

}