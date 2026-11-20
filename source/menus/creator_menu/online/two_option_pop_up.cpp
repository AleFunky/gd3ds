#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "mp3_player.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/creator_menu/online/online_level_menu.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/updater.hpp"
#include "fonts/goldFont.hpp"
#include "save/config.hpp"

#include "fonts/chatFont.hpp"
#include "fonts/goldFont.hpp"
#include "text.hpp"

namespace gd3ds
{

static void twoOptionPopUpInit(UIScreen *s) {
    TwoOptionPopupData *pop_up_data = (TwoOptionPopupData *)s->pair->data;
    if(!pop_up_data) return;

    UIWindow *window = (UIWindow *) uiGetElementByTag(s, "window");
    UILabel *title = (UILabel *) uiGetElementByTag(s, "title");
    UILabel *content = (UILabel *) uiGetElementByTag(s, "content");
    UIWindowButton *yes_button = (UIWindowButton *) uiGetElementByTag(s, "yesbutton");
    UIWindowButton *no_button = (UIWindowButton *) uiGetElementByTag(s, "nobutton");

    int textMarginX = 15.f;
    float textScale = content->base.scaleX;
    float buttonOffset = 30.f;
    float titleOffset = 28.f;
    
    char *buf = wrapText(&chatFontFontCharset, textScale, pop_up_data->text, window->base.w - 2 * textMarginX);
    float textHeight = getTextHeight(&chatFontFontCharset, &chatFontSheet, textScale, true, buf);

    float windowHeight = 90.f + textHeight;

    window->base.h = windowHeight;

    yes_button->base.base.y = (windowHeight / 2) - buttonOffset;
    no_button->base.base.y = (windowHeight / 2) - buttonOffset;
    title->base.y = -(windowHeight / 2) + titleOffset;

    uiLabelSetText(content, buf);
    uiLabelSetText(title, pop_up_data->title);

    uiButtonSetText((UIButton *)yes_button, pop_up_data->proceedText);
    uiButtonSetText((UIButton *)no_button, pop_up_data->cancelText);

    float yes_button_text_width = getTextLength(&goldFontFontCharset, yes_button->base.textScale, false, yes_button->base.text) + 20;
    float no_button_text_width = getTextLength(&goldFontFontCharset, no_button->base.textScale, false, no_button->base.text) + 20;

    yes_button->base.base.w = yes_button_text_width;
    yes_button->base.base.x = 0 + (((yes_button_text_width / 2) + (no_button_text_width / 2)) / 2) - 4;

    no_button->base.base.w = no_button_text_width;
    no_button->base.base.x = 0 - (((yes_button_text_width / 2) + (no_button_text_width / 2)) / 2) + 4;
}

static void twoOptionPopUpFreeData(void *data){
    TwoOptionPopupData *popUpData = (TwoOptionPopupData *)data;
    if(!popUpData) return;
    free((void *)popUpData->text);
    free((void *)popUpData->title);
    free((void *)popUpData->proceedText);
    free((void *)popUpData->cancelText);
    free((void *)popUpData);
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
    uiStackPushGameState(static_cast<int>(GameState::StateExit));
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
    uiStackPush(&updaterDef, UIAnimation::AnimZoom, UIAnimation::AnimZoom, UIStackPushType::PushAfterClose);
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

}