#include "level_options.hpp"

#include "main.hpp"
#include "menus/core/ui_element.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "save/saving.hpp"

static void actionToggleLdm(UIElement *e, const UIPropertyList *args) {
    if (!currentLevelEntry) return;

    currentLevelEntry->data.lowDetail = ((UICheckBox *)e)->checked;
    saveCurrentSaveFile(currentLevelList);
}

static const UIActionDef onlineLevelOptionsActions[] = {
    {"ldm", actionToggleLdm},
};

static void onlineLevelOptionsInit(UIScreen *s) {
    UICheckBox *checkbox = (UICheckBox *) uiGetElementByTag(s, "chk_ldm");
    if (checkbox) uiSetCheckboxChecked(checkbox, currentLevelEntry && currentLevelEntry->data.lowDetail);
}

extern const UIScreenDefPair onlineLevelOptionsDef = {
    .name = "online_level_options",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_options_pop_up.txt",
        .init = onlineLevelOptionsInit,
        .actionList = {
            .actions = onlineLevelOptionsActions,
            .actionCount = ARRAY_LEN(onlineLevelOptionsActions)
        }
    }
};
