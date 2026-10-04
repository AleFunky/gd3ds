#include "menus/core/ui_stack.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_spinner.h"

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
static UISpinner *install_spinner;

static Thread download_thread;

static DownloadTask download_task = {
    .path = CONFIG_ROOT,
    .file_name = "rom"
};

char downloadSpeed[16];
bool canExit;

static void action_exit(UIElement *e, const UIPropertyList *p) {
    if (canExit) {
        ui_stack_pop();
    }
}

static void update_game() {
    if (is_3DSX) {
        output_log("3dsx path: %s\nrom path: %s%s.%s", _3dsx_path, download_task.path, download_task.file_name, download_task.extension);
        char target_path[280];
        // char target_path[280] = "/3ds/gd3ds/rom.3dsx";
        // char tmp_path2[280] = "/3ds/gd3ds/rom_renamed2.3dsx";
        snprintf(target_path, sizeof(target_path), "%s%s.%s", download_task.path, download_task.file_name, download_task.extension);
        remove(_3dsx_path);
        rename(target_path, _3dsx_path);
        ui_label_set_text(status_label, "Success!");
    } else {
        Result res = 0;
        FILE *cia = NULL;
        Handle ciaHandle = 0;
        char target_path[280];
        snprintf(target_path, sizeof(target_path), "%s%s.%s", download_task.path, download_task.file_name, download_task.extension);
        // char target_path[280] = "/3ds/gd3ds/rom.cia";
        res = amInit();
        if (R_FAILED(res)) {
            output_log("failed to init application manager: %d", res);
            return;
        }
        cia = fopen(target_path, "rb");
        if (!cia) {
            amExit();
            output_log("file not found");
            ui_label_set_text(status_label, "cia not found");
            return;
        }
        res = AM_StartCiaInstall(MEDIATYPE_SD, &ciaHandle);
        if (R_FAILED(res)) {
            fclose(cia);
            amExit();
            ui_label_set_text(status_label, "failed to start cia install");
            output_log("failed to start cia install: %d", res);
            return;
        }
        size_t bufferSize = 1024 * 64;
        u8* buffer = malloc(bufferSize);
        u32 bytesWritten = 0;
        size_t bytesRead = 0;

        while ((bytesRead = fread(buffer, 1, bufferSize, cia)) > 0)
        {
            res = FSFILE_Write(ciaHandle, &bytesWritten, 0, buffer, bytesRead, FS_WRITE_FLUSH);

            if (R_FAILED(res))
            {
                AM_CancelCIAInstall(ciaHandle);
                free(buffer);
                fclose(cia);
                amExit();
                output_log("failed during cia install: %d", res);
                ui_label_set_text(status_label, "failed during cia install");
                return;
            }
        }

    res = AM_FinishCiaInstall(ciaHandle);
    
    free(buffer);
    fclose(cia);
    amExit();
    output_log("SUCCESS!!!!!!!");
    ui_label_set_text(status_label, "Success!");
    }
}

static void updater_pop_up_init(UIScreen *s) {
    status_label = (UILabel *)ui_get_element_by_tag(s, "statuslabel");
    speed_label = (UILabel *)ui_get_element_by_tag(s, "speedlabel");
    time_label = (UILabel *)ui_get_element_by_tag(s, "timelabel");
    download_progress_bar = (UIProgressBar *)ui_get_element_by_tag(s, "progressbar");
    exit_button = (UIButton *)ui_get_element_by_tag(s, "exitbutton");
    install_spinner = (UISpinner *)ui_get_element_by_tag(s, "spinner");

    ui_disable_element((UIElement *) install_spinner);
    download_task.url = update_data->releaseDownloadUrl;
    snprintf(download_task.extension, sizeof(download_task.extension), is_3DSX ? "3dsx" : "cia");
    download_progress_bar->value = 0;
    ui_progress_bar_set_tint(download_progress_bar, C2D_Color32(50, 190, 240, 255));
    canExit = true;

    download_thread = create_file_download_thread(&download_task);
    // download_task.finished = true;
    // download_task.result = 0;
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
            char buf[64];
            snprintf(buf, sizeof(buf), "download failure. code: %d\nfile path: %s%s.%s", download_task.result, download_task.path, download_task.file_name, download_task.extension);
            output_log(buf);
            ui_label_set_text(status_label, buf);
            return;
        }

        canExit = false;
        ui_enable_element((UIElement *) install_spinner);
        ui_disable_element((UIElement *) download_progress_bar);
        ui_disable_element((UIElement *) speed_label);
        ui_disable_element((UIElement *) exit_button);
        ui_label_set_text(status_label, "Installing...");
        status_label->base.y = 45;
        download_task.finished = false;
        update_game();
    }
}

static void updater_pop_up_exit(UIScreen *s) {
    if (download_task.running) {
            download_task.cancelled = true;
            threadJoin(download_thread, U64_MAX);
        }
}

static UIActionDef updater_pop_up_actions[] = {
    {"soft_exit", action_exit }
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