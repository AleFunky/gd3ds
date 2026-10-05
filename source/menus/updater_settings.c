#include "save/config.h"

#include "menus/components/ui_checkbox.h"
#include "menus/updater_settings.h"

bool useNightlyBranch;
bool doAutoUpdates;

void action_set_nightly(UIElement* e, const UIPropertyList *args) {
    useNightlyBranch = ((UICheckBox *)e)->checked;
}

void action_set_auto_updates(UIElement* e, const UIPropertyList *args) {
    doAutoUpdates = ((UICheckBox *)e)->checked;
}

static void updater_settings_init(UIScreen *s) {
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_nightly")), useNightlyBranch);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_auto_update")), doAutoUpdates);
}

static void updater_settings_exit(UIScreen *s) {
    cfg_save();
}

static UIActionDef updater_settings_actions[] = {
    { "nightly", action_set_nightly },
    { "autoupdate", action_set_auto_updates },
};

const UIScreenDefPair updater_settings_def = {
    .name = "updater_settings",
    .btm = {
        .path = "romfs:/menus/updater_settings.txt",
        .init = updater_settings_init,
        .exit = updater_settings_exit,
        .action_list = {
            .action_count = ARRAY_LEN(updater_settings_actions),
            .actions = updater_settings_actions
        }
    },
};