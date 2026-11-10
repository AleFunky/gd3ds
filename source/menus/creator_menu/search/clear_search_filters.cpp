#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "search_filters.hpp"

void action_clear_search_filters(UIElement* e, const UIPropertyList *args) {
    reset_search_filters();
}

static UIActionDef clear_fitlers_actions[] = {
    { "clear", action_clear_search_filters },
};

extern const UIScreenDefPair clear_filters_def = {
    .name = "clear_filters",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/clear_filters.txt",
        .action_list = {
            .actions = clear_fitlers_actions,
            .action_count = ARRAY_LEN(clear_fitlers_actions)
        }
    }
};