#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"
#include "search_filters.h"

void action_clear_search_filters(UIElement* e, const UIPropertyList *args) {
    reset_search_filters();
}

static UIActionDef clear_fitlers_actions[] = {
    { "clear", action_clear_search_filters },
};

const UIScreenDefPair clear_filters_def = {
    .name = "clear_filters",
    .btm = {
        .path = "romfs:/menus/creator_menu/clear_filters.txt",
        .action_list = {
            .action_count = ARRAY_LEN(clear_fitlers_actions),
            .actions = clear_fitlers_actions
        }
    }
};