#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "text.hpp"

#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/settings_hub/info_card.hpp"
#include "menus/updater.hpp"
#include "menus/updater_pop_up.hpp"
#include "menus/updater_settings.hpp"
#include "fonts/chatFont.hpp"

#include "utils/server_utils.hpp"
#include "utils/utils.hpp"
#include "utils/folders.hpp"

static UIScreen *screen;

static UIList *list;
static UILabel *errorLabel;
static UIButton *updaterButton;
static UISpinner *spinner;

GenericTask updaterTask = {
    .func = checkForUpdates
};

Thread updaterThread;
bool checkedForUpdate = false;

static void actionUpdate(UIElement *e, const UIPropertyList *p) {
    if (is3Dsx && isCitra() && !threeDsxPath) {
        InfoCardData *data = (InfoCardData *)malloc(sizeof(InfoCardData));
        if (!data) return;

        data->text = strdup(".3dsx updates aren't supported on\nemulators! Please update the game\nmanually.");
        data->title = strdup("Error");
        data->customTitle = true;
        data->copied = true;

        uiStackPush(&infoCardDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
        uiStackPushData(data);
        return;
    }
    if (updateData->isAvailable && checkedForUpdate) uiStackPush(&updaterPopUpDef, ANIM_NONE, ANIM_ZOOM, PUSH_NEXT); 
}

static void actionRefreshUpdates(UIElement *e, const UIPropertyList *p) {
    if (updaterTask.running) return;
    uiLabelSetText(errorLabel, "");
    uiListReset(list);
    uiEnableElement((UIElement *) spinner);
    uiDisableElement((UIElement *) updaterButton);
    checkedForUpdate = false;
    updaterThread = createGenericThread(&updaterTask);
}

const char *handleUpdaterErrorCodes(int code) {
    switch (code) {
        case -4:
            return "Too many requests!\nPlease try again later.";
            break;
        case -3:
            return "No updates found.";
            break;
        case -2:
            return "Failed to find a release!";
            break;
        case -1:
            return "Failed to parse response!";
            break;
        case 6:
        case 7:
            return "No internet connection!";
            break;
        case 28:
            return "Connection timed out.";
            break;
        case 56: 
            return "Connection interrupted.";
            break;
        case 42:
            break;
        default:
            return "Unknown error.";
            break;
    }
    return "";
}

static UIActionDef updaterActions[] = {
    {"update", actionUpdate },
    {"refresh", actionRefreshUpdates }
};

static void populateInfo() {
    int listWidth = list->base.w / 2;
    int textMarginX = 0;
    int textMarginY = 3;
    float textScale = 0.75;
    int lineHeight = 10;

    // list commits for nightly
    if (useNightlyBranch && latestCommits) {

        // description card
        UIElement *info = (UIElement *)uiCreateRectangle(screen);
        if (info) {
            uiRectangleSetColor((UIRectangle *)info, C2D_Color32(194, 114, 62, 0));
            uiElementSetSize(info, list->base.w, lineHeight + textMarginY * 2);
            // label
            UILabel *label = uiCreateLabel(screen);
            if (label) {
                uiLabelSetText(label, "<#ffff00>Changes since last update:</>");
                uiElementSetScale((UIElement *)label, textScale);
                uiElementSetPosition((UIElement *)label, -listWidth + textMarginX, 0);
                label->font = 1;
                uiElementAddChild(info, (UIElement *)label);
            }
            uiListAdd(list, info);
        }

        for (int i = 0; i < latestCommitCount; i++) {
            float commitScale = textScale * 0.9f;
            char *buf = wrapText(&chatFontFontCharset, commitScale, latestCommits[i], list->base.w - 2 * textMarginX);
            float textHeight = getTextHeight(&chatFontFontCharset, &chatFontSheet, commitScale, true, buf);

            UIElement *card = (UIElement *)uiCreateRectangle(screen);
            if (card && i < 30 - 1) {
                uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 0) : C2D_Color32(194, 114, 62, 63)));
                uiElementSetSize(card, list->base.w, textHeight + textMarginY * 2);
                
                // commit title
                UILabel *title = uiCreateLabel(screen);
                if (title) {
                    uiLabelSetText(title, buf);

                    uiElementSetPosition((UIElement *)title, -listWidth + textMarginX, 0);
                    uiElementSetScale((UIElement *)title, commitScale );

                    title->font = 1;

                    uiElementAddChild(card, (UIElement *)title);
                }

                uiListAdd(list, card);
            } else if (card) {
                uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 0) : C2D_Color32(194, 114, 62, 63)));
                uiElementSetSize(card, list->base.w, lineHeight + textMarginY * 2);
                
                // ... and more
                UILabel *title = uiCreateLabel(screen);
                if (title) {
                    uiLabelSetText(title, "<#27ff00>... and more!</>");

                    uiElementSetPosition((UIElement *)title, -listWidth + textMarginX, 0);
                    uiElementSetScale((UIElement *)title, textScale);

                    title->font = 1;

                    uiElementAddChild(card, (UIElement *)title);
                }

                uiListAdd(list, card);
            }
        }
    } else if (updateData->releaseBody) {
        int baseLineCount = 0;
        char **infoText = splitString(updateData->releaseBody, '\n', &baseLineCount, true);
        // description card
        UIElement *info = (UIElement *)uiCreateRectangle(screen);
        if (info) {
            uiRectangleSetColor((UIRectangle *)info, C2D_Color32(194, 114, 62, 0));
            uiElementSetSize(info, list->base.w, lineHeight + textMarginY * 2);
            // label
            UILabel *label = uiCreateLabel(screen);
            if (label) {
                uiLabelSetText(label, updateData->releaseTitle);
                uiElementSetScale((UIElement *)label, textScale * 1.2);
                uiElementSetPosition((UIElement *)label, -listWidth + textMarginX, 0);
                label->font = 1;
                uiElementAddChild(info, (UIElement *)label);
            }
            uiListAdd(list, info);
        }
        // body 
        for (int i = 0; i < baseLineCount; i++) {
            char *buf = wrapText(&chatFontFontCharset, textScale, infoText[i], list->base.w - 2 * textMarginX);
            float textHeight = getTextHeight(&chatFontFontCharset, &chatFontSheet, textScale, true, buf);

            UIElement *card = (UIElement *)uiCreateRectangle(screen);
            if (card) {
                uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 0) : C2D_Color32(161, 88, 48, 0)));
                uiElementSetSize(card, list->base.w, textHeight + textMarginY * 2);
                
                // commit title
                UILabel *title = uiCreateLabel(screen);
                if (title) {
                    title->base.w = list->base.w - 2 * textMarginX; 
                    uiLabelSetText(title, buf);

                    uiElementSetPosition((UIElement *)title, -listWidth + textMarginX, 0);
                    uiElementSetScale((UIElement *)title, textScale);

                    title->font = 1;

                    uiElementAddChild(card, (UIElement *)title);
                }

                uiListAdd(list, card);
            }   
        }
        freeStringArray(infoText, baseLineCount);
    }
}

static void updaterInit(UIScreen *s) {
    screen = s;
    list = (UIList *)uiGetElementByTag(s, "list");
    updaterButton = (UIButton *)uiGetElementByTag(s, "updatebutton");
    spinner = (UISpinner *)uiGetElementByTag(s, "spinner");
    errorLabel = (UILabel *)uiGetElementByTag(s, "errorlabel");
    if (checkedForUpdate && updateData->isAvailable) {
        uiDisableElement((UIElement *) spinner);
        populateInfo();
    } else {
        uiDisableElement((UIElement *) updaterButton);
        uiEnableElement((UIElement *) spinner);
        if (!updaterTask.running) updaterThread = createGenericThread(&updaterTask);
    }
}

static void updaterUpdate(UIScreen *s, UIInput *i) {
    // Run when finished
    if (updaterTask.finished) {
        uiDisableElement((UIElement *) spinner);
        // Handle result
        if (updaterTask.result == 0) {
            if (updateData->isAvailable) {
                uiEnableElement((UIElement *) updaterButton);
                populateInfo();
                checkedForUpdate = true;
            }
        } else {
            outputLog(handleUpdaterErrorCodes(updaterTask.result));
            uiLabelSetText(errorLabel, handleUpdaterErrorCodes(updaterTask.result));
        }
        updaterTask.finished = false;
    }
}

extern const UIScreenDefPair updaterDef = {
    .name = "updater",
    .btm = {
        .path = "romfs:/menus/updater.txt",
        .init = updaterInit,
        .update = updaterUpdate,
        .actionList = {
            .actions = updaterActions,
            .actionCount = ARRAY_LEN(updaterActions)
        }
    },
};