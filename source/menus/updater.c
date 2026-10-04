#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.h"

#include "menus/components/ui_list.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_spinner.h"
#include "menus/components/ui_window_button.h"
#include "menus/updater.h"
#include "menus/updater_pop_up.h"

#include "utils/server_utils.h"
#include "utils/utils.h"
#include "utils/json_config.h"


static UIList *list;
static UILabel *error_label;
static UIWindowButton *updater_button;
static UISpinner *spinner;

static GenericTask updater_task = {
    .func = check_for_updates
};

static Thread updater_thread;
bool checkedForUpdate;

static void action_update(UIElement *e, const UIPropertyList *p) {
    if (!checkedForUpdate) {
        ui_enable_element((UIElement *) spinner);
        updater_thread = create_generic_thread(&updater_task);
        return;
    }
    if (update_data->isAvailable) ui_stack_push(&updater_pop_up_def, ANIM_NONE, ANIM_ZOOM, PUSH_NEXT); 
}

static UIActionDef updater_actions[] = {
    {"update", action_update }
};

static void updater_init(UIScreen *s) {
    list = (UIList *)ui_get_element_by_tag(s, "list");
    updater_button = (UIWindowButton *)ui_get_element_by_tag(s, "updatebutton");
    spinner = (UISpinner *)ui_get_element_by_tag(s, "spinner");
    error_label = (UILabel *)ui_get_element_by_tag(s, "errorlabel");
    ui_disable_element((UIElement *) spinner);
}

static void updater_update(UIScreen *s, UIInput *i) {
    // Run when finished
    if (updater_task.finished) {
        ui_disable_element((UIElement *) spinner);
        // Handle result
        if (updater_task.result == 0) {
            if (update_data->isAvailable) {
                ui_button_set_text((UIButton *) updater_button, "Install Update");
                ui_label_set_text(error_label, update_data->releaseTitle);
                updater_button->base.base.w = 164;
                checkedForUpdate = true;
            } else output_log("no updates found");
        } else output_log("failure, code: %d", updater_task.result);
        updater_task.finished = false;
    }
}

const UIScreenDefPair updater_def = {
    .name = "updater",
    .btm = {
        .path = "romfs:/menus/updater.txt",
        .init = updater_init,
        .update = updater_update,
        .action_list = {
            .action_count = ARRAY_LEN(updater_actions),
            .actions = updater_actions
        }
    },
};