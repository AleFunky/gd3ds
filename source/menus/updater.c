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
bool checkedForUpdate = false;

static void action_update(UIElement *e, const UIPropertyList *p) {
    if (update_data->isAvailable && checkedForUpdate) ui_stack_push(&updater_pop_up_def, ANIM_NONE, ANIM_ZOOM, PUSH_NEXT); 
}

static void action_refresh_updates(UIElement *e, const UIPropertyList *p) {
    ui_label_set_text(error_label, "");
    ui_enable_element((UIElement *) spinner);
    ui_disable_element((UIElement *) updater_button);
    checkedForUpdate = false;
    updater_thread = create_generic_thread(&updater_task);
}

char *handle_updater_error_codes(int code) {
    switch (code) {
        case -3:
            return "No updates found.";
            break;
        case -2:
            return "Failed to find\nrelease!";
            break;
        case -1:
            return "Failed to parse\nresponse!";
            break;
        case 6:
        case 7:
            return "No internet\nconnection!";
            break;
        case 28:
            return "Connection timed\nout.";
            break;
        case 56: 
            return "Connection reset by\npeer.";
            break;
        case 42:
            break;
        default:
            return "Unknown error.";
            break;
    }
    return "";
}

static UIActionDef updater_actions[] = {
    {"update", action_update },
    {"refresh", action_refresh_updates }
};

static void updater_init(UIScreen *s) {
    list = (UIList *)ui_get_element_by_tag(s, "list");
    updater_button = (UIWindowButton *)ui_get_element_by_tag(s, "updatebutton");
    spinner = (UISpinner *)ui_get_element_by_tag(s, "spinner");
    error_label = (UILabel *)ui_get_element_by_tag(s, "errorlabel");
    if (checkedForUpdate && update_data->isAvailable) {
        ui_disable_element((UIElement *) spinner);
        ui_label_set_text(error_label, update_data->releaseTitle);
    } else {
        ui_disable_element((UIElement *) updater_button);
        ui_enable_element((UIElement *) spinner);
        updater_thread = create_generic_thread(&updater_task);
    }
}

static void updater_update(UIScreen *s, UIInput *i) {
    // Run when finished
    if (updater_task.finished) {
        ui_disable_element((UIElement *) spinner);
        // Handle result
        if (updater_task.result == 0) {
            if (update_data->isAvailable) {
                ui_enable_element((UIElement *) updater_button);
                ui_label_set_text(error_label, update_data->releaseTitle);
                checkedForUpdate = true;
            } else output_log("no updates found");
        } else {
            output_log(handle_updater_error_codes(updater_task.result));
            ui_label_set_text(error_label, handle_updater_error_codes(updater_task.result));
        }
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