#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "search_filters.hpp"

namespace gd3ds
{

void actionClearSearchFilters(UIElement* e, const UIPropertyList *args) {
    resetSearchFilters();
}

static UIActionDef clearFitlersActions[] = {
    { "clear", actionClearSearchFilters },
};

extern const UIScreenDefPair clearFiltersDef = {
    .name = "clear_filters",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/clear_filters.txt",
        .actionList = {
            .actions = clearFitlersActions,
            .actionCount = ARRAY_LEN(clearFitlersActions)
        }
    }
};

}