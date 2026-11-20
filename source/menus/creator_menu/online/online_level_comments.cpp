#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "main.hpp"
#include "icons.hpp"
#include "fonts/chatFont.hpp"
#include "fonts/goldFont.hpp"
#include "menus/core/common_setters.hpp"
#include "menus/core/ui_element.hpp"
#include "text.hpp"
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

namespace gd3ds
{

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
            // uiLabelSetText(error_label, "An unknown error has\n occured.");
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
        // entry width

        float entry_width = list->base.w;
        float entry_center = 0.f;
        float entry_left = -entry_width / 2;
        float entry_right = entry_width / 2;


        // label scaling
        float usernameScale = 0.6f;
        float contentScale = 0.68f;
        float percentScale = 0.63f;
        float timestampScale = 0.65f;
        float likeScale = 0.35f;

        float contentMargin = 6.f;

        // comment content

        char *wrappedContent = wrapText(&chatFontFontCharset, contentScale, commentEntries[i].content, entry_width - 10 - 2 * contentMargin);

        float textHeight = getTextHeight(&chatFontFontCharset, &chatFontSheet, contentScale, true, wrappedContent);

        // entry height

        float entry_height = 52.f + textHeight;
        float entry_top = -entry_height / 2;
        float entry_bottom = entry_height / 2;

        // style username
        char username[36] = "-";
        snprintf(username, sizeof(username), "<#%s>%s</>", (commentEntries[i].authorAccountId == 0) ? "5AFFFF" : "FFFFFF", commentEntries[i].name);

        // style timestamp
        char timestamp[136];
        snprintf(timestamp, sizeof(timestamp) - 1, "%s ago", commentEntries[i].commentAge);


        int percent = commentEntries[i].percent;
        int likes = commentEntries[i].likes;

        UIElement *card = (UIElement *)uiCreateRectangle(screen);

        if (card) {
            uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 44, 255)));
            uiElementSetSize(card, 0, entry_height);

            UIWindow *bg_window = uiCreateWindow(screen);
            if (bg_window) {
                uiElementSetSize((UIElement *)bg_window, entry_width - 10, entry_height - 10);
                uiElementSetPosition((UIElement *)bg_window, 0, 0);

                bg_window->atlas = C2D_SpriteSheetGetImage(windowSheet, 2);
                bg_window->border = bg_window->atlas.subtex->width / 3;
                bg_window->color = C2D_Color32(130, 64, 33, 100);
                uiElementAddChild(card, (UIElement *)bg_window);
            }

            // Commenter's icon (robtop server exclusive)
            UIIcon *user_icon = uiCreateIcon(screen);
            if (user_icon && !gdps) {
                //output_log("icon type: %d\nicon id: %d\nicon p1: %d\n icon p2: %d\n\n", comment_entries[i].iconType, comment_entries[i].playerIcon, comment_entries[i].col1, comment_entries[i].col2);
                int iconType = commentEntries[i].iconType;
                int iconIndex = commentEntries[i].playerIcon;

                // check if provided player icon is in bounds
                if (iconType >= ICON_GAMEMODE_COUNT) {
                    iconType = 0;
                    iconIndex = 1;
                } else if (iconIndex >= gamemodeIconCount[iconType]) iconIndex = 1;

                
                user_icon->glow = commentEntries[i].glow;
                uiIconSetGamemodeIndex(user_icon, iconType, iconIndex);

                size_t glow_col = commentEntries[i].glowCol;
                if (glow_col < 0 || glow_col >= numColors) glow_col = 12;
                
                size_t p1_col = commentEntries[i].col1;
                if (p1_col < 0 || p1_col >= numColors) p1_col = 12;
                
                size_t p2_col = commentEntries[i].col2;
                if (p2_col < 0 || p2_col >= numColors) p2_col = 12;

                uiIconSetGlow(user_icon, colors[gdToGd3dsColorTable[glow_col]]);
                uiIconSetP1(user_icon, colors[gdToGd3dsColorTable[p1_col]]);
                uiIconSetP2(user_icon, colors[gdToGd3dsColorTable[p2_col]]);
                
                uiElementSetPosition((UIElement *)user_icon, entry_left + 20, entry_top + 15);
                uiElementSetScale((UIElement *)user_icon, 0.5f);

                uiElementAddChild(card, (UIElement *)user_icon);
            }

            // Comment author
            UILabel *username_label = uiCreateLabel(screen);
            if (username_label) {
                uiLabelSetText(username_label, username);
                uiElementSetPosition((UIElement *)username_label, entry_left + (gdps ? 10 : 30), entry_top + 15);
                uiElementSetScale((UIElement *)username_label, usernameScale);

                username_label->font = 2;

                uiElementAddChild(card, (UIElement *)username_label);
            }

            // Mod badge
            UIImage *badge = uiCreateImage(screen);
            if (badge && commentEntries[i].modBadge > 0) {
                int badgeOffset = 0;
                badgeOffset += commentEntries[i].modBadge;
                if (commentEntries[i].modBadge > 2) badgeOffset = 2;
                uiImageSetImage(badge, 371 + badgeOffset, 0);
                uiElementSetPosition((UIElement *)badge, entry_left + (gdps ? 15 : 30) + getTextLength(&goldFontFontCharset, 0.7, false, commentEntries[i].name) + 3, -entry_center + 15);
                uiElementSetScale((UIElement *)badge, 0.7f);

                uiElementAddChild(card, (UIElement *)badge);
            }

            // Comment content
            UILabel *content_label = uiCreateLabel(screen);
            if (content_label) {                
                char tinted_desc[strlen(wrappedContent) + 13];
                snprintf(tinted_desc, sizeof(tinted_desc), "<%s>%s</>", ((commentEntries[i].modBadge == 0) ? "#FFFFFF" : commentEntries[i].modCommentColor), wrappedContent);
                uiLabelSetText(content_label, tinted_desc);
                // free(wrappedContent);
                uiElementSetPosition((UIElement *)content_label, entry_left + 12, 0 - 1.5f);
                uiElementSetScale((UIElement *)content_label, contentScale);

                content_label->font = 1;

                uiElementAddChild(card, (UIElement *)content_label);
            }

            // Comment percent
            UILabel *percent_label = uiCreateLabel(screen);
            if (percent_label && percent > 0) {
                char tmp_value[24];

                snprintf(tmp_value, sizeof(tmp_value), "<#00000096>%d%%", percent);
                uiLabelSetText(percent_label, tmp_value);
                uiElementSetPosition((UIElement *)percent_label, entry_right - 11, entry_top + 15);
                uiElementSetScale((UIElement *)percent_label, percentScale);

                percent_label->font = 1;
                percent_label->alignment = 1;

                uiElementAddChild(card, (UIElement *)percent_label);
            }

            // Comment likes

            UIImage *like_icon = uiCreateImage(screen);
            if (like_icon) {
                uiImageSetImage(like_icon, 97, 0);
                uiElementSetPosition((UIElement *)like_icon, entry_left + 18, entry_bottom - 16);
                uiElementSetScale((UIElement *)like_icon, 0.7f);

                if (likes < 0) {
                    uiImageSetImage(like_icon, DISLIKE_ICON, 0);
                }

                uiElementAddChild(card, (UIElement *)like_icon);
            }

            UILabel *like_value = uiCreateLabel(screen);
            if (like_value) {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "%d",  likes);

                uiLabelSetText(like_value, tmp_value);
                uiElementSetPosition((UIElement *)like_value, entry_left + 28, entry_bottom - 16);
                uiElementSetScale((UIElement *)like_value, likeScale);

                // like_value->alignment = 1.f;

                uiElementAddChild(card, (UIElement *)like_value);
            }

            // Comment timestamp
            if(!gdps){
                UILabel *timestamp_value = uiCreateLabel(screen);
                if (timestamp_value) {
                    char tmp_value[sizeof(timestamp) + 14 - 1];
                    snprintf(tmp_value, sizeof(tmp_value), "<#0000007D>%s", timestamp);

                    uiLabelSetText(timestamp_value, tmp_value);
                    uiElementSetPosition((UIElement *)timestamp_value, entry_right - 8, entry_bottom - 15);
                    uiElementSetScale((UIElement *)timestamp_value, timestampScale);

                    timestamp_value->font = 1;
                    timestamp_value->alignment = 1.f;

                    uiElementAddChild(card, (UIElement *)timestamp_value);
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

}