#include "save/config.hpp"

#include "menus/components/ui_checkbox.hpp"
#include "menus/updater_settings.hpp"

bool useNightlyBranch;
bool doAutoUpdates;

void actionSetNightly(UIElement* e, const UIPropertyList *args) {
    useNightlyBranch = ((UICheckBox *)e)->checked;
}

void actionSetAutoUpdates(UIElement* e, const UIPropertyList *args) {
    doAutoUpdates = ((UICheckBox *)e)->checked;
}

static void updaterSettingsInit(UIScreen *s) {
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_nightly")), useNightlyBranch);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_auto_update")), doAutoUpdates);
}

static void updaterSettingsExit(UIScreen *s) {
    cfgSave();
}

static UIActionDef updaterSettingsActions[] = {
    { "nightly", actionSetNightly },
    { "autoupdate", actionSetAutoUpdates },
};

extern const UIScreenDefPair updaterSettingsDef = {
    .name = "updater_settings",
    .btm = {
        .path = "romfs:/menus/updater_settings.txt",
        .init = updaterSettingsInit,
        .exit = updaterSettingsExit,
        .actionList = {
            .actions = updaterSettingsActions,
            .actionCount = ARRAY_LEN(updaterSettingsActions)
        }
    },
};