#include "level_options.h"

#include "main.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_checkbox.h"
#include "save/saving.h"

static void action_toggle_ldm(UIElement *e, const UIPropertyList *args) {
    if (!current_level_entry) return;

    current_level_entry->data.low_detail = ((UICheckBox *)e)->checked;
    save_current_save_file(current_level_list);
}

static const UIActionDef online_level_options_actions[] = {
    {"ldm", action_toggle_ldm},
};

static void online_level_options_init(UIScreen *s) {
    UICheckBox *checkbox = (UICheckBox *) ui_get_element_by_tag(s, "chk_ldm");
    if (checkbox) ui_set_checkbox_checked(checkbox, current_level_entry && current_level_entry->data.low_detail);
}

const UIScreenDefPair online_level_options_def = {
    .name = "online_level_options",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_options_pop_up.txt",
        .init = online_level_options_init,
        .action_list = {
            .action_count = ARRAY_LEN(online_level_options_actions),
            .actions = online_level_options_actions
        }
    }
};
