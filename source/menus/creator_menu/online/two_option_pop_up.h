#pragma once
#include "menus/core/ui_stack.h"

typedef struct {
    const char *text;
    const char *title;
    const char *cancel_text;
    const char *proceed_text;
} YesNoPopupData;

extern const UIScreenDefPair two_option_pop_up_def;
extern const UIScreenDefPair warning_pop_up_def;