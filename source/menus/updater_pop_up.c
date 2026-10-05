#include "menus/core/ui_stack.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_spinner.h"
#include "menus/components/ui_window_button.h"

#include "menus/updater.h"

#include "utils/server_utils.h"
#include "utils/utils.h"
#include "utils/json_config.h"
#include "utils/folders.h"
#include "utils/string_helpers.h"

static UILabel *status_label;
static UILabel *speed_label;
static UILabel *time_label;
static UIProgressBar *download_progress_bar;
static UIButton *exit_button;
static UIWindowButton *finish_update_button;
static UISpinner *install_spinner;

static Thread download_thread;

static DownloadTask download_task = {
    .path = CONFIG_ROOT,
    .file_name = "rom"
};

static Thread install_thread;

static GenericTask install_task = {
    .func = install_update
};

char downloadSpeed[16];
char downloadedUpdateFilePath[256];
int updateResult = 0;
bool canExit;

static void action_exit(UIElement *e, const UIPropertyList *p) {
    if (canExit) {
        ui_stack_pop();
    }
}

static void action_finish_update(UIElement *e, const UIPropertyList *p) {
    if (updateResult == 0) {
        ui_stack_push_game_state(STATE_EXIT);
        stop_mp3();
        return;
    }
}

char *handle_download_error_codes(int code) {
    switch (code) {
        case 6:
        case 7:
            return "Download failed:\nNo internet connection!";
            break;
        case 28:
            return "Download failed:\nConnection timed out.";
            break;
        case 56: 
            return "Download failed:\nConnection reset by peer.";
            break;
        case 42:
            break;
        default:
            return "Download failed:\nUnknown error.";
            break;
    }
    return "";
}

char *handle_install_error_codes(int code) {
    switch (code) {
        case -1:
            return "Install failed:\nFilesystem error.";
            break;
        case 1:
            return "Install failed:\nROM file not found.";
            break;
        case 2: 
            return "Install failed:\nAM init failure.";
            break;
        case 3:
            return "Install failed:\nFailed to open write handle.";
            break;
        case 4:
            return "Install failed:\nError during file streaming.";
            break;
    }
    return "";
}

static void updater_pop_up_init(UIScreen *s) {
    status_label = (UILabel *)ui_get_element_by_tag(s, "statuslabel");
    speed_label = (UILabel *)ui_get_element_by_tag(s, "speedlabel");
    time_label = (UILabel *)ui_get_element_by_tag(s, "timelabel");
    download_progress_bar = (UIProgressBar *)ui_get_element_by_tag(s, "progressbar");
    exit_button = (UIButton *)ui_get_element_by_tag(s, "exitbutton");
    install_spinner = (UISpinner *)ui_get_element_by_tag(s, "spinner");
    finish_update_button = (UIWindowButton *)ui_get_element_by_tag(s, "finishbutton");

    ui_disable_element((UIElement *) install_spinner);
    ui_disable_element((UIElement *) finish_update_button);
    download_task.url = update_data->releaseDownloadUrl;
    snprintf(download_task.extension, sizeof(download_task.extension), is_3DSX ? "3dsx" : "cia");
    download_progress_bar->value = 0;
    ui_progress_bar_set_tint(download_progress_bar, C2D_Color32(50, 190, 240, 255));
    canExit = true;

    download_thread = create_file_download_thread(&download_task);
}

static void updater_pop_up_update(UIScreen *s, UIInput *i) {
    if (download_task.running) {
        download_progress_bar->value = download_task.progress;
        char *speed = truncate_speed(download_task.speed);
        if (speed) {
            snprintf(downloadSpeed, sizeof(downloadSpeed), "%s", speed);
            ui_label_set_text(speed_label, downloadSpeed);
            free(speed);
        }
    }
    // Run when finished
    if (download_task.finished) {
        // Handle result
        if (download_task.result != 0) {
            status_label->base.y = 10;
            output_log(handle_download_error_codes(download_task.result));
            ui_disable_element((UIElement *) install_spinner);
            ui_label_set_text(status_label, handle_download_error_codes(download_task.result));
            return;
        }

        canExit = false;

        // disable home button for the duration of the install
        aptSetHomeAllowed(false);

        ui_enable_element((UIElement *) install_spinner);
        ui_disable_element((UIElement *) download_progress_bar);
        ui_disable_element((UIElement *) speed_label);
        ui_disable_element((UIElement *) exit_button);
        ui_label_set_text(status_label, "Installing...");
        status_label->base.y = 45;
        download_task.finished = false;
        snprintf(downloadedUpdateFilePath, sizeof(downloadedUpdateFilePath), "%s%s.%s", download_task.path, download_task.file_name, download_task.extension);
        install_thread = create_generic_thread(&install_task);
    }

    if (install_task.finished) {
        ui_disable_element((UIElement *) install_spinner);
        // reenable home button
        aptSetHomeAllowed(true);
        // Handle result
        if (install_task.result != 0) {
            output_log(handle_install_error_codes(install_task.result));
            status_label->base.y = 10;
            ui_label_set_text(status_label, handle_install_error_codes(install_task.result));
            canExit = true;
            ui_enable_element((UIElement *) exit_button);
            return;
        }
        queued_restart = !is_3DSX;
        status_label->base.y = -5;
        install_task.finished = false;
        ui_label_set_text(status_label, is_3DSX ? "Update successful!\nPress OK to quit." : "Update successful!\nPress OK to restart.");
        ui_enable_element((UIElement *) finish_update_button);
    }
}

static void updater_pop_up_exit(UIScreen *s) {
    if (download_task.running) {
            download_task.cancelled = true;
            threadJoin(download_thread, U64_MAX);
        }
}

static UIActionDef updater_pop_up_actions[] = {
    {"soft_exit", action_exit },
    {"finish_update", action_finish_update }
};

const UIScreenDefPair updater_pop_up_def = {
    .name = "updater_pop_up",
    .btm = {
        .path = "romfs:/menus/updater_pop_up.txt",
        .init = updater_pop_up_init,
        .update = updater_pop_up_update,
        .exit = updater_pop_up_exit,
        .action_list = {
            .action_count = ARRAY_LEN(updater_pop_up_actions),
            .actions = updater_pop_up_actions
        }
    },
};