#include <3ds.h>
#include <citro2d.h>

#include "mp3_player.hpp"
#include "save/saving.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/creator_menu/online/online_level_menu.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/updater.hpp"
#include "fonts/goldFont.hpp"
#include "save/config.hpp"

static void twoOptionPopUpInit(UIScreen *s) {
    TwoOptionPopupData *popUpData = (TwoOptionPopupData *)s->pair->data;
    if(!popUpData) return;

    UILabel *title = (UILabel *) uiGetElementByTag(s, "title");
    UILabel *content = (UILabel *) uiGetElementByTag(s, "content");
    UIWindowButton *yesButton = (UIWindowButton *) uiGetElementByTag(s, "yesbutton");
    UIWindowButton *noButton = (UIWindowButton *) uiGetElementByTag(s, "nobutton");

    uiLabelSetText(title, popUpData->title);
    uiLabelSetText(content, popUpData->text);

    uiButtonSetText((UIButton *)yesButton, popUpData->proceedText);
    uiButtonSetText((UIButton *)noButton, popUpData->cancelText);

    float yesButtonTextWidth = getTextLength(&goldFontFontCharset, yesButton->base.textScale, false, yesButton->base.text) + 20;
    float noButtonTextWidth = getTextLength(&goldFontFontCharset, noButton->base.textScale, false, noButton->base.text) + 20;

    yesButton->base.base.w = yesButtonTextWidth;
    yesButton->base.base.x = 0 + (((yesButtonTextWidth / 2) + (noButtonTextWidth / 2)) / 2) - 4;

    noButton->base.base.w = noButtonTextWidth;
    noButton->base.base.x = 0 - (((yesButtonTextWidth / 2) + (noButtonTextWidth / 2)) / 2) + 4;
}

static void twoOptionPopUpFreeData(void *data){
    TwoOptionPopupData *popUpData = (TwoOptionPopupData *)data;
    if(!popUpData) return;
    free((void *)popUpData->text);
    free((void *)popUpData->title);
    free((void *)popUpData->proceedText);
    free((void *)popUpData->cancelText);
    free(popUpData);
}

extern const UIScreenDefPair twoOptionPopUpDef = {
    .name = "two_option_popup",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/two_option_pop_up.txt",
        .init = twoOptionPopUpInit
    },
    .freeData = twoOptionPopUpFreeData,
};

//WARNING POPUP

static void actionOnlineLevelWarning(UIElement *e, const UIPropertyList *args){
    checkWarningsAndPlay();
}

const UIActionDef warningActions[] = {
    { "on_confirm", actionOnlineLevelWarning }
};

extern const UIScreenDefPair warningPopUpDef = {
    .name = "warning_popup",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/two_option_pop_up.txt",
        .init = twoOptionPopUpInit,
        .actionList = {
            .actions = warningActions,
            .actionCount = ARRAY_LEN(warningActions)
        }
    },
    .freeData = twoOptionPopUpFreeData,
};

//DELETE POPUP

static void actionDeleteLevel(UIElement *e, const UIPropertyList *args){
    deleteLevel();
}

const UIActionDef deleteActions[] = {
    { "on_confirm", actionDeleteLevel }
};

extern const UIScreenDefPair deletePopUpDef = {
    .name = "delete_popup",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/delete_pop_up.txt",
        .init = twoOptionPopUpInit,
        .actionList = {
            .actions = deleteActions,
            .actionCount = ARRAY_LEN(deleteActions)
        }
    },
    .freeData = twoOptionPopUpFreeData,
};

//EXIT POPUP

static void actionQuitGame(UIElement *e, const UIPropertyList *args){
    uiStackPushGameState(STATE_EXIT);
    stopMP3();
    return;
}


const UIActionDef quitPopUpActions[] = {
    { "on_confirm", actionQuitGame }
};

extern const UIScreenDefPair quitPopUpDef = {
    .name = "quit_pop_up",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/two_option_pop_up.txt",
        .init = twoOptionPopUpInit,
        .actionList = {
            .actions = quitPopUpActions,
            .actionCount = ARRAY_LEN(quitPopUpActions)
        }
    },
    .freeData = twoOptionPopUpFreeData,
};

// AUTOUPDATE POPUP

static void actionOpenUpdater(UIElement *e, const UIPropertyList *args){
    uiStackPush(&updaterDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_AFTER_CLOSE);
    return;
}

const UIActionDef autoupdatePopUpActions[] = {
    { "on_confirm", actionOpenUpdater }
};

extern const UIScreenDefPair autoupdatePopUpDef = {
    .name = "autoupdate_pop_up",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/two_option_pop_up.txt",
        .init = twoOptionPopUpInit,
        .actionList = {
            .actions = autoupdatePopUpActions,
            .actionCount = ARRAY_LEN(autoupdatePopUpActions)
        }
    },
    .freeData = twoOptionPopUpFreeData,
};

// AUTOUPDATE POPUP

static void actionDisable3d(UIElement *e, const UIPropertyList *args){
    turnOffSetting("stereoEnabled", &settingsState.stereoEnabled);
}

static void actionConfirm(UIElement *e, const UIPropertyList *args){
    cfgSave();
    return;
}

const UIActionDef threeDWarningPopUpActions[] = {
    // { "on_confirm", action_open_updater }
    { "on_decline", actionDisable3d },
    { "on_confirm", actionConfirm }
};

extern const UIScreenDefPair threeDWarningPopUpDef = {
    .name = "3d_warning_pop_up",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/two_option_pop_up.txt",
        .init = twoOptionPopUpInit,
        .actionList = {
            .actions = threeDWarningPopUpActions,
            .actionCount = ARRAY_LEN(threeDWarningPopUpActions)
        }
    },
    .freeData = twoOptionPopUpFreeData,
};