#include <3ds.h>
#include <citro2d.h>
#include "3ds/services/cfgu.h"
#include "main.hpp"
#include "mp3_player.hpp"
#include "save/config.hpp"
#include "state.hpp"

#include "utils/gfx.hpp"
#include "utils/json_config.hpp"
#include "utils/precise_input.hpp"
#include "utils/utils.hpp"

#include "menus/core/common_setters.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/core/ui_screen.hpp"

#include "menus/settings_hub/settings.hpp"

#include "menus/settings_hub/info_card.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"

namespace gd3ds
{

static int currentPage = 0;

SettingState settingsState;

UIList *list;
UILabel *categoryName;

bool requestListReload = false;

void wideSetting(bool checked);
void stereoSetting(bool checked);
void practiceMusicSyncSetting(bool checked);
void menuMusicSetting(bool checked);

const char *categoryNames[] = {
    "Graphics",
    "Input",
    "Misc",
    "Gameplay",
    "Cosmetic"
};

// Conditions

bool conditionSupportsWide() {
    u8 model = getModel();

    return model != CFG_MODEL_2DS && !isCitra();
}

bool conditionSupports3D() {
    u8 model = getModel();

    return model != CFG_MODEL_2DS && model != CFG_MODEL_N2DSXL;
}

Setting settings[] = {
    {
        .id = "wideEnabled",
        .label = "800 px res",
        .additionalInfo = "Doubles the top screen's horizontal\nresolution.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.wideEnabled,
        .key = CONFIG_GRAPHICS_PATH "wideEnabled",

        .onChanged = wideSetting,
        
        .disabledForceValue = false,
        .condition = conditionSupportsWide
    },
    {
        .id = "stereoEnabled",
        .label = "Stereoscopic 3D",
        .additionalInfo = "Adds depth to the top screen.\nUse the 3D slider, costs some FPS.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.stereoEnabled,
        .key = CONFIG_GRAPHICS_PATH "stereoEnabled",

        .onChanged = stereoSetting,

        .disabledForceValue = false,
        .condition = conditionSupports3D
    },
    {
        .id = "particlesDisabled",
        .label = "Disable particles",
        .additionalInfo = NULL,
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.particlesDisabled,
        .key = CONFIG_GRAPHICS_PATH "particlesDisabled"
    },
    {
        .id = "glowEnabled",
        .label = "Enable object glow",
        .additionalInfo = NULL,
        .page = PAGE_GRAPHICS, 

        .defaultValue = true,
        .var = &settingsState.glowEnabled,
        .key = CONFIG_GRAPHICS_PATH "glowEnabled"
    },
    {
        .id = "disableGravityEffect",
        .label = "Disable gravity effect",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.disableGravityEffect,
        .key = CONFIG_GRAPHICS_PATH "disableGravityEffect",
    },
    {
        .id = "showFpsCounter",
        .label = "FPS counter",
        .additionalInfo = "Displays current FPS in gameplay.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.showFpsCounter,
        .key = CONFIG_GRAPHICS_PATH "showFpsCounter"
    },
    {
        .id = "vsyncEnabled",
        .label = "V-Sync",
        .additionalInfo = "Prevents screen tearing at the cost\nof some performance.\nCaps FPS at 60.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = true,
        .var = &settingsState.vsync,
        .key = CONFIG_GRAPHICS_PATH "vsyncEnabled",
    },
    {
        .id = "yButton",
        .label = "Y to jump",
        .additionalInfo = "Swaps your jump input to Y.",
        .page = PAGE_INPUT, 

        .defaultValue = false,
        .var = &settingsState.yJump,
        .key = CONFIG_INPUT_PATH "yButton"
    },
    {
        .id = "touchEffectEverywhere",
        .label = "Global tap effect",
        .additionalInfo = "Plays the tap effect across all menus.",
        .page = PAGE_INPUT, 

        .defaultValue = false,
        .var = &settingsState.touchEffectEverywhere,
        .key = CONFIG_INPUT_PATH "touchEffectEverywhere"
    },
    {
        .id = "gameplayTapEffect",
        .label = "Gameplay tap effect",
        .additionalInfo = "Plays the tap effect in gameplay.",
        .page = PAGE_INPUT, 

        .defaultValue = true,
        .var = &settingsState.gameplayTapEffect,
        .key = CONFIG_INPUT_PATH "gameplayTapEffect"
    },
    {
        .id = "enableDebugBindings",
        .label = "Enable debug keys",
        .additionalInfo = "Enables debug key shortcuts.\n(B + L, B + R, X)",
        .page = PAGE_INPUT,

        .defaultValue = false,
        .var = &settingsState.enableDebugBindings,
        .key = CONFIG_INPUT_PATH "enableDebugBindings"
    },
    {
        .id = "preciseInput",
        .label = "240Hz input (CBF)",
        .additionalInfo = "Registers jumps on the exact 240Hz\nphysics tick they were pressed on.\nPrecision is reduced below 30fps.",
        .page = PAGE_INPUT,

        .defaultValue = false,
        .var = &piEnabled,
        .key = CONFIG_INPUT_PATH "preciseInput"
    },
    {
        .id = "hitboxesEnabled",
        .label = "Show hitboxes",
        .additionalInfo = "Shows object hitboxes while in a level.\nWARNING: AFFECTS PERFOMANCE!",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxesEnabled,
        .key = CONFIG_MISC_PATH "hitboxesEnabled"
    },
    {
        .id = "hitboxTrail",
        .label = "Enable hitbox trail",
        .additionalInfo = NULL,
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxTrail,
        .key = CONFIG_MISC_PATH "hitboxTrail"
    },
    {
        .id = "hitboxesOnDeath",
        .label = "Show hitboxes\non death",
        .additionalInfo = NULL,
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxesOnDeath,
        .key = CONFIG_MISC_PATH "hitboxesOnDeath"
    },
    {
        .id = "doNot",
        .label = "Do not",
        .additionalInfo = "Doesn't do anything...\nWell, nothing useful.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.doNot,
        .key = CONFIG_MISC_PATH "doNot"
    },
    {
        .id = "practiceMusicSync",
        .label = "Practice music sync",
        .additionalInfo = "Plays the level's music in practice mode.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.practiceMusicSync,
        .key = CONFIG_MISC_PATH "practiceMusicSync",

        .onChanged = practiceMusicSyncSetting
    },
    {
        .id = "menuMusic",
        .label = "Menu Music",
        .additionalInfo = "Toggles the Main Menu Music.",
        .page = PAGE_MISC, 

        .defaultValue = true,
        .var = &settingsState.menuMusic,
        .key = CONFIG_MISC_PATH "menuMusic",

        .onChanged = menuMusicSetting
    },
    {
        .id = "fastMenu",
        .label = "Fast Menu",
        .additionalInfo = "Makes transitions between\nmenu pages faster.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.fastMenu,
        .key = CONFIG_MISC_PATH "fastMenu"
    },
    {
        .id = "skipHighObjWarning",
        .label = "Disable object alert",
        .additionalInfo = "Disables the high object alert for\ncustom levels.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.skipHighObjWarning,
        .key = CONFIG_MISC_PATH "skipHighObjAlert",
    },
    {
        .id = "skipVersionWarning",
        .label = "Disable version alert",
        .additionalInfo = "Disables the incompatible\nGeometry Dash version alert for\ncustom levels.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.skipVersionWarning,
        .key = CONFIG_MISC_PATH "skipVersionAlert",
    },
    {
        .id = "skipSongWarning",
        .label = "Disable song alert",
        .additionalInfo = "Disables the missing song alert for\ncustom levels.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.skipSongWarning,
        .key = CONFIG_MISC_PATH "skipSongAlert",
    },
    {
        .id = "showProgressBar",
        .label = "Show progress bar",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.showProgressBar,
        .key = CONFIG_GAMEPLAY_PATH "showProgressBar"
    },
    {
        .id = "showProgressPercent",
        .label = "Show level percentage",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.showProgressPercent,
        .key = CONFIG_GAMEPLAY_PATH "showProgressPercent"
    },
    {
        .id = "decimalPercent",
        .label = "Accurate percentage",
        .additionalInfo = "Shows level progress with 2 decimals.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.decimalPercent,
        .key = CONFIG_GAMEPLAY_PATH "decimalPercent"
    },
    {
        .id = "ultraDecimalPercent",
        .label = "Ultra percentage",
        .additionalInfo = "But mom, I want more decimals!!!!\n(Why would you want to use this?)",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.ultraDecimalPercent,
        .key = CONFIG_GAMEPLAY_PATH "ultraDecimalPercent"
    },
    {
        .id = "goldenBest",
        .label = "Golden best",
        .additionalInfo = "Changes the percentage counter to\nbe gold when you pass\nyour previous best.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.goldenBest,
        .key = CONFIG_GAMEPLAY_PATH "goldenBest"
    },
    {
        .id = "quickRetry",
        .label = "Quick retry",
        .additionalInfo = "Restarts in 0.5 seconds instead of 1.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.quickRetry,
        .key = CONFIG_GAMEPLAY_PATH "quickRetry"
    },
    {
        .id = "autoCheckpoints",
        .label = "Auto checkpoints",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.autoCheckpoints,
        .key = CONFIG_GAMEPLAY_PATH "autoCheckpoints"
    },
    {
        .id = "quickCheckpoints",
        .label = "Quick checkpoints",
        .additionalInfo = "Makes auto checkpoints appear quicker.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.quickCheckpoints,
        .key = CONFIG_GAMEPLAY_PATH "quickCheckpoints"
    },
    {
        .id = "hidePracticeButtons",
        .label = "Hide Practice Buttons",
        .additionalInfo = "Hides the checkpoint buttons\nshown in practice mode.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.hidePracticeButtons,
        .key = CONFIG_GAMEPLAY_PATH "hidePracticeButtons"
    },
    {
        .id = "swapGameplayScreens",
        .label = "Switch gameplay\nto bottom screen",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.swapGameplayScreens,
        .key = CONFIG_GAMEPLAY_PATH "swapGameplayScreens"
    },
    {
        .id = "defaultMiniIcon",
        .label = "Default mini icon",
        .additionalInfo = "Uses default player icon in\nmini mode.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.defaultMiniIcon,
        .key = CONFIG_COSMETIC_PATH "defaultMiniIcon"
    },
    {
        .id = "switchTrailColor",
        .label = "Switch trail color",
        .additionalInfo = "Makes the player trail use P1\ninstead of P2.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.switchTrailColor,
        .key = CONFIG_COSMETIC_PATH "switchTrailColor"
    },
    {
        .id = "switchWaveTrailColor",
        .label = "Switch wave\ntrail color",
        .additionalInfo = "Makes the wave trail use P1\ninstead of P2.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.switchWaveTrailColor,
        .key = CONFIG_COSMETIC_PATH "switchWaveTrailColor"
    },
    {
        .id = "solidWaveTrail",
        .label = "Solid wave trail",
        .additionalInfo = "Disables blending for the wave trail.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.solidWaveTrail,
        .key = CONFIG_COSMETIC_PATH "solidWaveTrail"
    },
    {
        .id = "noPlayerTrail",
        .label = "Disable player trail",
        .additionalInfo = NULL,
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.noPlayerTrail,
        .key = CONFIG_COSMETIC_PATH "noPlayerTrail"
    },
    {
        .id = "noWaveTrailBehind",
        .label = "No wave trail behind",
        .additionalInfo = "Disables player trail for the wave.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.noWaveTrailBehind,
        .key = CONFIG_COSMETIC_PATH "noWaveTrailBehind"
    },
    {
        .id = "dimSecondaryScreen",
        .label = "Dim secondary screen",
        .additionalInfo = NULL,
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = NULL,
        .key = NULL,

        .buttonIcon = 425,
        .buttonSheet = 0,
        .buttonActions = "open_menu[screen=\"dim_screen\" btm_anim=\"zoom\"]"
    },
};

typedef struct CheckboxData {
    Setting *setting;
} CheckboxData;

typedef struct InfoButtonData {
    const char *text;
} InfoButtonData;

UICheckBox *getSettingCheckboxByID(const char *id) {
    if (list) {
        // Iterate through list children
        for (UIElement *rectangle = list->base.firstChild; rectangle; rectangle = rectangle->nextSibling) {
            // Find checkbox in the rectangle
            UIElement *checkbox = uiGetChildByType(rectangle, UI_CHECKBOX);
            if (checkbox) {
                if (checkbox->userdata) {
                    CheckboxData *data = (CheckboxData *)checkbox->userdata;

                    // Check if the ids match
                    if (strcmp(id, data->setting->id) == 0) {
                        return (UICheckBox *) checkbox;
                    }
                }
            }
        }
    }

    return NULL;
}

// Wide and 3D both want the whole top screen, so only one of them gets it
void turnOffSetting(const char *id, bool *var) {
    *var = false;

    UICheckBox *checkbox = getSettingCheckboxByID(id);

    if (checkbox) uiSetCheckboxChecked(checkbox, false);
}

void wideSetting(bool checked) {
    if (checked) turnOffSetting("stereoEnabled", &settingsState.stereoEnabled);
}

void stereoSetting(bool checked) {
    if (!checked) return;
    if (!threeDWarningPassed) {
        TwoOptionPopupData *threeDWarningData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
        if (!threeDWarningData) return;
        
        threeDWarningData->text = strdup("3D can significantly lower performance.\nAre you sure you want to keep it on?");
        threeDWarningData->title = strdup("Disclaimer");
        threeDWarningData->proceedText = strdup("Enable");
        threeDWarningData->cancelText = strdup("Disable");
        uiStackPush(&threeDWarningPopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
        uiStackPushData(threeDWarningData);
        threeDWarningPassed = true;
    }

    // No 3D on this console, so don't let the box stay ticked
    if (!stereoSupported()) {
        turnOffSetting("stereoEnabled", &settingsState.stereoEnabled);
        return;
    }

    turnOffSetting("wideEnabled", &settingsState.wideEnabled);
}

void practiceMusicSyncSetting(bool checked) {
    // Enable song
    if (state.practiceMode) {
        stopMP3();
        if (checked) {
            playLevelSong(levelInfo.songOffset + state.player.timeElapsed);
        } else {
            playPracticeSong();
        }
        pausePlaybackMP3();
    }
}

void menuMusicSetting(bool checked) {
    if (gameState != STATE_MENU) return;

    if (!checked) {
        if (playingMenuLoop) {
            stopMP3();
            playingMenuLoop = false;
        }
    } else if (!playingMenuLoop) {
        playMenuSong();
    }
}

static void checkboxAction(UIElement *e, const UIPropertyList* args) {
    CheckboxData *data = (CheckboxData *)e->userdata;
    bool checked = ((UICheckBox *)e)->checked;
    if (data) {
        *data->setting->var = checked;
        if (data->setting->onChanged) data->setting->onChanged(checked);
    }
}

static void setButtonStyle(UIElement *e) {
    UIWindowButton *button = (UIWindowButton *) e;
    int page = uiPropInt(&e->customProperties, "page", 0);
    uiWindowButtonSetStyle(button, (page == currentPage ? 10 : 5));
}

void actionOpenInfo(UIElement *e, const UIPropertyList *args){
    InfoButtonData *eData = (InfoButtonData *)e->userdata;
    if(!eData) return;

    const char *text = eData->text;
    if(!text) return;
    
    InfoCardData *data = (InfoCardData *)malloc(sizeof(InfoCardData));
    if(!data) return;
    *data = (InfoCardData){ 0 };
    
    data->text = text;
    data->copied = false;

    uiStackPushData(data);
}

void actionCategory(UIElement *e, const UIPropertyList *args) {
    currentPage = uiPropInt(&e->customProperties, "page", 0);
    requestListReload = true;
}

static UIActionDef settingsActions[] = {
    { "open_info", actionOpenInfo },
    { "category", actionCategory}
};

void createSetting(Setting *setting, int id, UIScreen *s) {
    if (list) {
        float listWidth = list->base.w * 0.5f;

        UIElement *card = (UIElement *) uiCreateRectangle(s);

        if (card) {
            uiRectangleSetColor((UIRectangle *) card, (id & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
            uiElementSetSize(card, 0, 28);

            if (setting->buttonActions) {
                UIButton *setButton = uiCreateButton(s);
                if (setButton) {
                    uiButtonSetImage(setButton, setting->buttonIcon, setting->buttonSheet);
                    uiElementSetPosition((UIElement *) setButton, -listWidth + 13, 0);
                    uiElementSetScale((UIElement *) setButton, (32.f * 0.6f) / setButton->base.w);

                    UIElement *setElement = (UIElement *) setButton;
                    setElement->actions = parseActions(setting->buttonActions, settingsActions, ARRAY_LEN(settingsActions), &setElement->actionCount);

                    uiElementAddChild(card, (UIElement *) setButton);
                }
            } else {
                UICheckBox *checkbox = uiCreateCheckbox(s);
                if (checkbox) {
                    // Store in the user data
                    CheckboxData *data = (CheckboxData *)malloc(sizeof(*data));
                    data->setting = setting;

                    uiElementSetUserdata((UIElement *) checkbox, data);

                    uiElementSetPosition((UIElement *) checkbox, -listWidth + 13, 0);
                    uiElementSetScale((UIElement *) checkbox, 0.6f);
                    uiElementSetAction((UIElement *) checkbox, checkboxAction);

                    uiSetCheckboxChecked(checkbox, *setting->var);
                    uiElementAddChild(card, (UIElement *) checkbox);
                }
            }

            UILabel *name = uiCreateLabel(s);
            if (name) {
                name->base.w = list->base.w - 60;
                uiLabelSetText(name, setting->label);
                uiElementSetPosition((UIElement *) name, -listWidth + 28, 0);
                uiElementSetScale((UIElement *) name, 0.38f);

                uiElementAddChild(card, (UIElement *) name);
            }

            if (setting->additionalInfo) {
                UIButton *info = uiCreateButton(s);
                if (info) {
                    // Store the text pointer in the user data
                    InfoButtonData *data = (InfoButtonData *)malloc(sizeof(*data));
                    data->text = setting->additionalInfo;
                    uiElementSetUserdata((UIElement *) info, data);

                    uiElementSetPosition((UIElement *) info, listWidth -+ 13, 0);
                    uiElementSetScale((UIElement *) info, 0.7f);
                    info->base.actions = parseActions(
                        "open_menu[screen=\"info_card\" btm_anim=\"zoom\"],open_info", 
                        settingsActions, 
                        ARRAY_LEN(settingsActions), 
                        &info->base.actionCount
                    );

                    uiButtonSetImage(info, 90, 0);

                    uiElementAddChild(card, (UIElement *) info);
                }
            }

            uiListAdd(list, card);
        }
    }
}

void loadCategory(SettingPage page, UIScreen *s) {
    uiListReset(list);
    uiLabelSetText(categoryName, categoryNames[page]);
    uiRunFuncOnTag(s, "category", setButtonStyle);

    int count = 0;
    for (size_t i = 0; i < ARRAY_LEN(settings); i++) {
        Setting *setting = &settings[i];

        // Do not make this setting appear if the condition is false
        if (setting->condition && !setting->condition()) {
            continue;
        }

        if (setting->page == page) {
            createSetting(setting, count, s);
            count++;
        }
    }
}

static void settingsInit(UIScreen *s) {
    list = (UIList *) uiGetElementByTag(s, "list");
    categoryName = (UILabel *) uiGetElementByTag(s, "listlabel");
    currentPage = 0;
    loadCategory((SettingPage)currentPage, s);
}

static void settingsUpdate(UIScreen *s, UIInput *i) {
    if (requestListReload) {
        loadCategory((SettingPage)currentPage, s);
        requestListReload = false;
    }
}

static void settingsExit(UIScreen *s) {
    cfgSave();
}


extern const UIScreenDefPair settingsDef = {
    .name = "settings",
    .btm = {
        .path = "romfs:/menus/settings_hub/settings.txt",
        .init = settingsInit,
        .update = settingsUpdate,
        .exit = settingsExit,
        .actionList = {
            .actions = settingsActions,
            .actionCount = ARRAY_LEN(settingsActions)
        }
    }
};

}