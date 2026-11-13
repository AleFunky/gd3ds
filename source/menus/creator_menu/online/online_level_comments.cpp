#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "main.hpp"
#include "icons.hpp"
#include "fonts/chatFont.hpp"
#include "fonts/goldFont.hpp"
#include "utils/folders.hpp"
#include "utils/server_utils.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_icon.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "menus/icon_kit.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/palette_kit.hpp"

#include "utils/utils.hpp"

bool commentsNeedRefresh = true;
int commentsSortType = 1;
int currentCommentsPage = 0;

static UIScreen *screen;

static UIList *list;
static UILabel *errorLabel;

static UIButton *sortByRecencyButton;
static UIButton *sortByLikesButton;

static UISpinner *spinner;

static Thread thread;

static GenericTask commentsTask = {
    .func = getComments
};

static void updateCommentArrows(bool disableArrows) {
    if (commentEntriesLength == 10 && !disableArrows) uiRunFuncOnTag(screen, "nextpage", uiEnableElement); else uiRunFuncOnTag(screen, "nextpage", uiDisableElement);
    if ((currentCommentsPage) >= 1 && !disableArrows) uiRunFuncOnTag(screen, "prevpage", uiEnableElement); else uiRunFuncOnTag(screen, "prevpage", uiDisableElement);
}

static void handleCommentErrors(int code) {
    uiDisableElement((UIElement *)spinner);
    uiEnableElement((UIElement *)errorLabel);
    char temp[64];
    switch (code) {
        case -2:
            //gdps returns -2 when no comments are found idky (update, gd does this too i should change the unknown error code later)
            // if (gdps) break;
            // ui_label_set_text(error_label, "An unknown error has\n occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            uiLabelSetText(errorLabel, "No\n<#41e24e>Internet</> connection!");
            updateCommentArrows(true);
            break;

        case 28:
            uiLabelSetText(errorLabel, "Connection timed out!");
            updateCommentArrows(true);
            break;
        case 42:
            break;
        default:
            snprintf(temp, sizeof(temp), "An unknown error has\noccurred.\n\nError code: %d", code);
            uiLabelSetText(errorLabel, temp);
            break;
    }
}

void populateComments() {
    uiListReset(list);
    uiDisableElement((UIElement *) spinner);
    for (int i = 0; i < commentEntriesLength; i++) {
        char username[36] = "-";
        snprintf(username, sizeof(username), "<#%s>%s</>", (commentEntries[i].authorAccountId == 0) ? "5AFFFF" : "FFFFFF", commentEntries[i].name);

        char timestamp[136];
        int percent = commentEntries[i].percent;
        int likes = commentEntries[i].likes;

        snprintf(timestamp, sizeof(timestamp) - 1, "%s ago", commentEntries[i].commentAge);

        float listWidth = list->base.w * 0.5f;
        float listHeight = 86 * 0.5f;

        UIElement *card = (UIElement *)uiCreateRectangle(screen);

        if (card) {
            uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 44, 255)));
            uiElementSetSize(card, 0, 86);

            UIWindow *bgWindow = uiCreateWindow(screen);
            if (bgWindow) {
                uiElementSetSize((UIElement *)bgWindow, 2 * listWidth - 10, 2 * listHeight - 10);
                uiElementSetPosition((UIElement *)bgWindow, 0, 0);

                bgWindow->atlas = C2D_SpriteSheetGetImage(windowSheet, 2);
                bgWindow->border = bgWindow->atlas.subtex->width / 3;
                bgWindow->color = C2D_Color32(130, 64, 33, 100);
                uiElementAddChild(card, (UIElement *)bgWindow);
            }

            // Commenter's icon (robtop server exclusive)
            UIIcon *userIcon = uiCreateIcon(screen);
            if (userIcon && !gdps) {
                //output_log("icon type: %d\nicon id: %d\nicon p1: %d\n icon p2: %d\n\n", comment_entries[i].iconType, comment_entries[i].playerIcon, comment_entries[i].col1, comment_entries[i].col2);
                int iconType = commentEntries[i].iconType;
                int iconIndex = commentEntries[i].playerIcon;

                // check if provided player icon is in bounds
                if (iconType >= ICON_GAMEMODE_COUNT) {
                    iconType = 0;
                    iconIndex = 1;
                } else if (iconIndex >= gamemodeIconCount[iconType]) iconIndex = 1;

                
                userIcon->glow = commentEntries[i].glow;
                uiIconSetGamemodeIndex(userIcon, iconType, iconIndex);

                size_t glowCol = commentEntries[i].glowCol;
                if (glowCol < 0 || glowCol >= numColors) glowCol = 12;
                
                size_t p1Col = commentEntries[i].col1;
                if (p1Col < 0 || p1Col >= numColors) p1Col = 12;
                
                size_t p2Col = commentEntries[i].col2;
                if (p2Col < 0 || p2Col >= numColors) p2Col = 12;

                uiIconSetGlow(userIcon, colors[gdToGd3dsColorTable[glowCol]]);
                uiIconSetP1(userIcon, colors[gdToGd3dsColorTable[p1Col]]);
                uiIconSetP2(userIcon, colors[gdToGd3dsColorTable[p2Col]]);
                
                uiElementSetPosition((UIElement *)userIcon, -listWidth + 20, -listHeight + 15);
                uiElementSetScale((UIElement *)userIcon, 0.5f);

                uiElementAddChild(card, (UIElement *)userIcon);
            }

            // Comment author
            UILabel *usernameLabel = uiCreateLabel(screen);
            if (usernameLabel) {
                uiLabelSetText(usernameLabel, username);
                uiElementSetPosition((UIElement *)usernameLabel, -listWidth + (gdps ? 10 : 30), -listHeight + 15);
                uiElementSetScale((UIElement *)usernameLabel, 0.6f);

                usernameLabel->font = 2;

                uiElementAddChild(card, (UIElement *)usernameLabel);
            }

            // Mod badge

            UIImage *badge = uiCreateImage(screen);
            if (badge && commentEntries[i].modBadge > 0) {
                int badgeOffset = 0;
                badgeOffset += commentEntries[i].modBadge;
                if (commentEntries[i].modBadge > 2) badgeOffset = 2;
                uiImageSetImage(badge, 371 + badgeOffset, 0);
                uiElementSetPosition((UIElement *)badge, -listWidth + (gdps ? 15 : 30) + getTextLength(&goldFontFontCharset, 0.7, false, commentEntries[i].name) + 3, -listHeight + 15);
                uiElementSetScale((UIElement *)badge, 0.7f);

                uiElementAddChild(card, (UIElement *)badge);
            }

            // Comment content
            UILabel *contentLabel = uiCreateLabel(screen);
            if (contentLabel) {
                char *wrappedContent = wrapText(&chatFontFontCharset, contentLabel->base.scaleX, commentEntries[i].content, 270);
                char *desc = strdup(wrappedContent);
                char tintedDesc[strlen(desc) + 13];
                snprintf(tintedDesc, sizeof(tintedDesc), "<%s>%s</>", ((commentEntries[i].modBadge == 0) ? "#FFFFFF" : commentEntries[i].modCommentColor), desc);
                uiLabelSetText(contentLabel, tintedDesc);
                free(desc);
                uiElementSetPosition((UIElement *)contentLabel, -listWidth + 12, 0 - 1.5);
                uiElementSetScale((UIElement *)contentLabel, 0.68f);

                contentLabel->font = 1;

                uiElementAddChild(card, (UIElement *)contentLabel);
            }

            // Comment percent
            UILabel *percentLabel = uiCreateLabel(screen);
            if (percentLabel && percent > 0) {
                char tmpValue[24];

                snprintf(tmpValue, sizeof(tmpValue), "<#00000096>%d%%", percent);
                uiLabelSetText(percentLabel, tmpValue);
                uiElementSetPosition((UIElement *)percentLabel, listWidth - 11, -listHeight + 15);
                uiElementSetScale((UIElement *)percentLabel, 0.63f);

                percentLabel->font = 1;
                percentLabel->alignment = 1;

                uiElementAddChild(card, (UIElement *)percentLabel);
            }

            // Comment likes

            UIImage *likeIcon = uiCreateImage(screen);
            if (likeIcon) {
                uiImageSetImage(likeIcon, 97, 0);
                uiElementSetPosition((UIElement *)likeIcon, -listWidth + 18, listHeight - 16);
                uiElementSetScale((UIElement *)likeIcon, 0.7f);

                if (likes < 0) {
                    uiImageSetImage(likeIcon, DISLIKE_ICON, 0);
                }

                uiElementAddChild(card, (UIElement *)likeIcon);
            }

            UILabel *likeValue = uiCreateLabel(screen);
            if (likeValue) {
                char tmpValue[16];

                snprintf(tmpValue, sizeof(tmpValue), "%d",  likes);

                uiLabelSetText(likeValue, tmpValue);
                uiElementSetPosition((UIElement *)likeValue, -listWidth + 28, listHeight - 16);
                uiElementSetScale((UIElement *)likeValue, 0.35f);

                // like_value->alignment = 1.f;

                uiElementAddChild(card, (UIElement *)likeValue);
            }

            // Comment timestamp
            if(!gdps){
                UILabel *timestampValue = uiCreateLabel(screen);
                if (timestampValue) {
                    char tmpValue[sizeof(timestamp) + 14 - 1];
                    snprintf(tmpValue, sizeof(tmpValue), "<#0000007D>%s", timestamp);

                    uiLabelSetText(timestampValue, tmpValue);
                    uiElementSetPosition((UIElement *)timestampValue, listWidth - 8, listHeight - 15);
                    uiElementSetScale((UIElement *)timestampValue, 0.65f);

                    timestampValue->font = 1;
                    timestampValue->alignment = 1.f;

                    uiElementAddChild(card, (UIElement *)timestampValue);
                }
            }
            uiListAdd(list, card);
        }
    }
}

static void actionRefreshComments(UIElement* e, const UIPropertyList *args) {
    int buttonType = uiPropInt(&e->customProperties, "sorttype", 0);

    if (buttonType != -1) {
        uiButtonSetImage(sortByRecencyButton, 422, 0);
        uiButtonSetImage(sortByLikesButton, 422, 0);
        uiButtonSetImage((UIButton *)e, 423, 0);

        currentCommentsPage = 0;

        commentsSortType = buttonType;
    }
    uiListReset(list);
    uiDisableElement((UIElement *) errorLabel);
    uiEnableElement((UIElement *)spinner);
    updateCommentArrows(true);
    
    thread = createGenericThread(&commentsTask);
}

static void actionChangeCommentsPage(UIElement* e, const UIPropertyList *args) {
    currentCommentsPage += uiPropInt(&e->customProperties, "page", 0);
    
    uiListReset(list);
    updateCommentArrows(true);
    uiEnableElement((UIElement *)spinner);
    thread = createGenericThread(&commentsTask);
}

static UIActionDef onlineCommentsActions[] = {
    { "refresh", actionRefreshComments },
    { "changepage", actionChangeCommentsPage }
};
static void onlineCommentsExit(UIScreen *s) {
    if (commentsTask.running) {
        commentsTask.cancelled = true;
        threadJoin(thread, U64_MAX);
    }
}
void onlineCommentsInit(UIScreen *s) {
    //frick you cloud i'll define a screen just like old times
    screen = s;

    list = (UIList *) uiGetElementByTag(screen, "list");

    errorLabel = (UILabel *) uiGetElementByTag(screen, "errorlabel");
    sortByRecencyButton = (UIButton *) uiGetElementByTag(screen, "sortbyrecency");
    sortByLikesButton = (UIButton *) uiGetElementByTag(screen, "sortbylikes");

    spinner = (UISpinner *) uiGetElementByTag(screen, "spinner");

    uiButtonSetImage(sortByLikesButton, 423, 0);
    uiDisableElement((UIElement *) spinner);
    uiDisableElement((UIElement *) errorLabel);
    updateCommentArrows(true);
    if (commentsNeedRefresh) {
        uiEnableElement((UIElement *)spinner);
        currentCommentsPage = 0;
        commentsSortType = 1;
        createGenericThread(&commentsTask);
    } else {
        if (list) { // No need to fetch new comments
            uiLabelSetText(errorLabel, "");
            populateComments();
            updateCommentArrows(false);
        }
    }
}

static void onlineCommentsUpdate(UIScreen *s, UIInput *i) {
    // Run when finished
    if (commentsTask.finished) {
        int result = commentsTask.result;
        // Handle result
        if (result != 0 && commentsNeedRefresh) {
            handleCommentErrors(result);
        } else { // No errors
            uiLabelSetText(errorLabel, "");
            updateCommentArrows(false);
            populateComments();
            commentsNeedRefresh = false;
        }
        commentsTask.finished = false;
    }
}

extern const UIScreenDefPair onlineLevelCommentsDef = {
    .name = "online_level_comments",
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_comments.txt",
        .init = onlineCommentsInit,
        .update = onlineCommentsUpdate,
        .exit = onlineCommentsExit,
        .actionList = {
            .actions = onlineCommentsActions,
            .actionCount = ARRAY_LEN(onlineCommentsActions)
        }
    }
};