#include "menus/core/ui_stack.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_image.hpp"

#include "mp3_player.hpp"
#include "utils/server_utils.hpp"
#include "utils/utils.hpp"
#include "utils/json_config.hpp"
#include "utils/folders.hpp"
#include "utils/string_helpers.hpp"

namespace gd3ds
{

static UILabel *statusLabel;
static UILabel *speedLabel;
static UILabel *timeLabel;
static UIProgressBar *downloadProgressBar;
static UIButton *exitButton;
static UIWindowButton *finishUpdateButton;
static UISpinner *installSpinner;
static UIImage *homeImage;

static Thread downloadThread;

static DownloadTask downloadTask = {
    .path = CONFIG_ROOT,
    .fileName = "rom"
};

static Thread installThread;

static GenericTask installTask = {
    .func = installUpdate
};

static Thread extractThread;

static GenericTask extractTask = {
    .func = extractUpdate
};


char downloadSpeed[16];
char downloadedUpdateFilePath[256];
int updateResult = 0;
bool canExit;
bool showHomeIcon = false; 
int homeIconOpacity = 255;
int homeIconDelay = 255;

static void actionExit(UIElement *e, const UIPropertyList *p) {
    if (canExit) {
        uiStackPop();
    }
}

static void actionFinishUpdate(UIElement *e, const UIPropertyList *p) {
    if (updateResult == 0) {
        uiStackPushGameState(STATE_EXIT);
        stopMP3();
        return;
    }
}

const char *handleDownloadErrorCodes(int code) {
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

const char *handleInstallErrorCodes(int code) {
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

static void updaterPopUpInit(UIScreen *s) {
    statusLabel = (UILabel *)uiGetElementByTag(s, "statuslabel");
    speedLabel = (UILabel *)uiGetElementByTag(s, "speedlabel");
    timeLabel = (UILabel *)uiGetElementByTag(s, "timelabel");
    downloadProgressBar = (UIProgressBar *)uiGetElementByTag(s, "progressbar");
    exitButton = (UIButton *)uiGetElementByTag(s, "exitbutton");
    installSpinner = (UISpinner *)uiGetElementByTag(s, "spinner");
    finishUpdateButton = (UIWindowButton *)uiGetElementByTag(s, "finishbutton");
    homeImage = (UIImage *)uiGetElementByTag(s, "homeicon");

    uiDisableElement((UIElement *) installSpinner);
    uiDisableElement((UIElement *) finishUpdateButton);
    uiDisableElement((UIElement*) homeImage);
    downloadTask.url = updateData->releaseDownloadUrl;
    snprintf(downloadTask.extension, sizeof(downloadTask.extension), "zip");
    downloadProgressBar->value = 0;
    uiProgressBarSetTint(downloadProgressBar, C2D_Color32(50, 190, 240, 255));
    canExit = true;
    // attempt to remove leftover rom
    char oldPath[256];
    snprintf(oldPath, sizeof(oldPath), "%s%s.%s", CONFIG_ROOT, downloadTask.fileName, downloadTask.extension);
    remove(oldPath);

    downloadThread = createFileDownloadThread(&downloadTask);

    // temporarily disable sleep mode, enter exclusive mode so network won't get interrupted
    enterExclusiveState();
    aptSetSleepAllowed(false);
}

static void updaterPopUpUpdate(UIScreen *s, UIInput *i) {
    if (downloadTask.running) {
        downloadProgressBar->value = downloadTask.progress;
        char *speed = truncateSpeed(downloadTask.speed);
        if (speed) {
            snprintf(downloadSpeed, sizeof(downloadSpeed), "%s", speed);
            uiLabelSetText(speedLabel, downloadSpeed);
            free(speed);
        }
    }
    // Run when finished
    if (downloadTask.finished) {
        downloadTask.finished = false;
        uiDisableElement((UIElement *) downloadProgressBar);
        uiDisableElement((UIElement *) speedLabel);

        // networking is no longer needed, exit exclusive mode
        exitExclusiveState(false);

        // Handle result
        if (downloadTask.result != 0) {
            statusLabel->base.y = 10;
            aptSetSleepAllowed(true);
            outputLog(handleDownloadErrorCodes(downloadTask.result));
            uiLabelSetText(statusLabel, handleDownloadErrorCodes(downloadTask.result));
            return;
        }

        canExit = false;

        // disable home button for the rest of the process
        aptSetHomeAllowed(false);


        uiEnableElement((UIElement *) installSpinner);
        uiDisableElement((UIElement *) exitButton);
        uiLabelSetText(statusLabel, "Extracting...");
        statusLabel->base.y = 45;
        snprintf(downloadedUpdateFilePath, sizeof(downloadedUpdateFilePath), "%s%s.%s", downloadTask.path, downloadTask.fileName, downloadTask.extension);
        extractThread = createGenericThread(&extractTask);
    }

    if (extractTask.finished) {
        extractTask.finished = false;

        // Handle result
        if (extractTask.result != 0) {
            statusLabel->base.y = 10;
            aptSetSleepAllowed(true);
            canExit = false;
            outputLog("failed to extract rom\n");
            uiLabelSetText(statusLabel, "Failed to extract update!");
            statusLabel->base.y = 10;
            uiEnableElement((UIElement *) exitButton);
            return;
        }

        uiEnableElement((UIElement *) installSpinner);
        uiDisableElement((UIElement *) exitButton);
        uiLabelSetText(statusLabel, "Installing...");
        statusLabel->base.y = 45;
        downloadTask.finished = false;
        installThread = createGenericThread(&installTask);
    }

    if (installTask.finished) {
        installTask.finished = false;
        uiDisableElement((UIElement *) installSpinner);
        // reenable home button and sleep, exit exclusive mode
        aptSetHomeAllowed(true);
        aptSetSleepAllowed(true);

        // Handle result
        if (installTask.result != 0) {
            outputLog(handleInstallErrorCodes(installTask.result));
            statusLabel->base.y = 10;
            uiLabelSetText(statusLabel, handleInstallErrorCodes(installTask.result));
            canExit = true;
            uiEnableElement((UIElement *) exitButton);

            return;
        }
        queuedRestart = !is3Dsx;
        statusLabel->base.y = -5;
        uiLabelSetText(statusLabel, is3Dsx ? "Update successful!\nPress OK to quit." : "Update successful!\nPress OK to restart.");
        uiEnableElement((UIElement *) finishUpdateButton);
    }
    

    if (aptCheckHomePressRejected()) {
        showHomeIcon = true;
        uiEnableElement((UIElement *)homeImage); 
        homeIconOpacity = 255;
        homeIconDelay = 120;
    }

    if (showHomeIcon) {
        uiImageSetTint(homeImage, C2D_Color32(255, 255, 255, homeIconOpacity));
        if (homeIconDelay > 0) {
            homeIconDelay -= 10;
        } else {
        homeIconOpacity -= 10;
        }
        if (homeIconOpacity <= 0) {
            uiDisableElement((UIElement*) homeImage);
            showHomeIcon = false;
        }
    }
}

static void updaterPopUpExit(UIScreen *s) {
    if (downloadTask.running) {
        downloadTask.cancelled = true;
        exitExclusiveState(false);
        aptSetSleepAllowed(true);
        threadJoin(downloadThread, U64_MAX);
    }
}

static UIActionDef updaterPopUpActions[] = {
    {"soft_exit", actionExit },
    {"finish_update", actionFinishUpdate }
};

extern const UIScreenDefPair updaterPopUpDef = {
    .name = "updater_pop_up",
    .btm = {
        .path = "romfs:/menus/updater_pop_up.txt",
        .init = updaterPopUpInit,
        .update = updaterPopUpUpdate,
        .exit = updaterPopUpExit,
        .actionList = {
            .actions = updaterPopUpActions,
            .actionCount = ARRAY_LEN(updaterPopUpActions)
        }
    },
};

}