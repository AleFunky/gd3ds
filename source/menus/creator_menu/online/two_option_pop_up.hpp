#pragma once
#include "menus/core/ui_stack.hpp"

namespace gd3ds
{

struct TwoOptionPopupData {
    const char *text;
    const char *title;
    const char *cancelText;
    const char *proceedText;
};

extern const UIScreenDefPair twoOptionPopUpDef;
extern const UIScreenDefPair warningPopUpDef;
extern const UIScreenDefPair deletePopUpDef;
extern const UIScreenDefPair quitPopUpDef;
extern const UIScreenDefPair autoupdatePopUpDef;
extern const UIScreenDefPair threeDWarningPopUpDef;

}