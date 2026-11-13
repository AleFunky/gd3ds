#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <citro2d.h>
#include "3ds/thread.h"

#include "level/main_levels.hpp"
#include "level_loading.hpp"
#include "main.hpp"
#include "save/saving.hpp"
#include "state.hpp"

#include "utils/folders.hpp"
#include "utils/network.hpp"
#include "utils/server_utils.hpp"
#include "utils/string_helpers.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/core/common_setters.hpp"
#include "menus/core/ui_element.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/creator_menu/online/online_level_menu.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/settings_hub/info_card.hpp"

#include "fonts/bigFont.hpp"
#include "fonts/goldFont.hpp"
#include "utils/utils.hpp"

int currSearchID;
int redownload = false;

static UILabel *errorLabel;
static UILabel *pageInfoLabel;

static UISpinner *spinner;

static UIList *list;

static Thread thread;

static GenericTask searchTask = {
    .func = searchLevels
};

int searchResult = -2;

extern const int demonFaces[6] = {
    NA_FACE,
    EASY_DEMON_FACE,
    MEDIUM_DEMON_FACE,
    HARD_DEMON_FACE,
    INSANE_DEMON_FACE,
    EXTREME_DEMON_FACE
};

extern const int difficultyFaces[6] = {
    NA_FACE,
    EASY_FACE,
    NORMAL_FACE,
    HARD_FACE,
    HARDER_FACE,
    INSANE_FACE
};

extern const int epics[4] = {
    0,
    MYTHIC_GLOW,
    EPIC_GLOW,
    LEGENDARY_GLOW,
};

static void actionClearData(UIElement* e, const UIPropertyList *args) {
    if (searchEntries) {
        for (int i = 0; i < searchEntriesLength; i++) {
            free(searchEntries[i].description);
        }
        free(searchEntries);
        searchEntries = NULL;
    }
    if (creatorEntries) {
        free(creatorEntries);
        creatorEntries = NULL;
    }
    if (songEntries) {
        free(songEntries);
        songEntries = NULL;
    }
    if(pageEntry) {
        free(pageEntry);
        pageEntry = NULL;
    }
}

static void actionOpenOnlineLevelMenu(UIElement* e, const UIPropertyList *args) {
    OnlineCardData *entry = (OnlineCardData *)e->userdata;
    currSearchID = entry->entryId;
    onlineMenuLevelID = entry->levelId;
    redownload = entry->redownload; 
    uiStackPush(&onlineLevelMenuDef, ANIM_NONE, ANIM_NONE, PUSH_ROOT);
}

static void updateArrows(UIScreen *s) {
    if (searchEntriesLength == pageEntry->amount) uiRunFuncOnTag(s, "nextpage", uiEnableElement); else uiRunFuncOnTag(s, "nextpage", uiDisableElement);
    if ((filters.currentPage) >= 1) uiRunFuncOnTag(s, "prevpage", uiEnableElement); else uiRunFuncOnTag(s, "prevpage", uiDisableElement);

    int total = pageEntry->totalPages * pageEntry->amount - 1;
    int current = pageEntry->currentOffset + 1;
    int to = pageEntry->currentOffset + pageEntry->amount;

    // Cap to amount
    if (current > total) {
        current = total;
    }

    // Cap to amount
    if (to > total) {
        to = total;
    }

    char pageInfo[42];
    snprintf(pageInfo, sizeof(pageInfo), "%d to %d of %d", current, to, total);
    uiLabelSetText(pageInfoLabel, pageInfo);
}

static void actionChangePage(UIElement* e, const UIPropertyList *args) {
    filters.currentPage += uiPropInt(&e->customProperties, "page", 0);
    searchNeedsRefresh = true;
    uiRunFuncOnTag(e->screen, "nextpage", uiDisableElement);
    uiRunFuncOnTag(e->screen, "prevpage", uiDisableElement);
    thread = createGenericThread(&searchTask);
    uiEnableElement((UIElement *) spinner);
    if (list) uiListReset(list);
}

static void actionVersionWarning(UIElement* e, const UIPropertyList *args){
    VersionWarningData *eData = (VersionWarningData *)e->userdata;
    if(!eData) return;

    char buffer[256];
    if (eData->wasUpdated) {
        snprintf(buffer, sizeof(buffer), "This level was uploaded <#60abef>before or in</> %.1f,\nbut was updated in a later version. It\n<#ffa54b>might not be playable</>.", GD_VERSION);
    } else {
        snprintf(buffer, sizeof(buffer), "This level was uploaded <#60abef>after</> %.1f.\nIt will most likely <#f25149>not be playable</>.", GD_VERSION);
    }

    InfoCardData *data = (InfoCardData *)malloc(sizeof(InfoCardData));
    if(!data) return;

    data->text = strdup(buffer);
    data->title = strdup("Version Warning");
    data->customTitle = true;
    data->copied = true;

    uiStackPush(&infoCardDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    uiStackPushData(data);
}

static UIActionDef onlineActions[] = {
    { "open_level_menu", actionOpenOnlineLevelMenu },
    { "changepage", actionChangePage },
    { "clear_data", actionClearData },
    { "open_version_warning", actionVersionWarning }
};

static void populateList() {
    uiDisableElement((UIElement *) spinner);
    uiListReset(list);
    for (int i = 0; i < searchEntriesLength; i++) {
        char tmpName[32];
        char tmpCreator[36];
        char tmpSong[256];

        SearchEntry *entry = &searchEntries[i];

        strncpy(tmpName, entry->name, sizeof(tmpName) - 1);
        
        // Get creator name
        const char *creatorName = "-";

        bool creatorInBounds = entry->creatorIndex >= 0 && entry->creatorIndex < creatorEntriesLength;
        
        if (creatorInBounds) {
            creatorName = creatorEntries[entry->creatorIndex].creatorName;
        }

        snprintf(tmpCreator, sizeof(tmpCreator), "<#%s>By %s</>", (!creatorInBounds || creatorEntries[entry->creatorIndex].userId == 0) ? "5AFFFF" : "FFFFFF", creatorName);

        // Get song name
        const char *songName = "Unknown";

        if (entry->songId != 0) {
            if (entry->songIndex >= 0 && entry->songIndex < songEntriesLength) {
                songName = songEntries[entry->songIndex].songTitle;
            }
        } else {
            if (entry->mainSongId >= 0 && entry->mainSongId < currentMainLevelPack->count) {
                songName = (char *) currentMainLevelPack->levels[entry->mainSongId].songData.title;
            }
        }

        const char *songColor = (entry->songId != 0 ? "<#f982ff>" : "<#27d2ff>");

        snprintf(tmpSong, sizeof(tmpSong) - 1, "%s%s", songColor, songName);

        truncateFilename(tmpSong, 35);

        float listWidth = list->base.w * 0.5f;

        UIElement *card = (UIElement *)uiCreateRectangle(&defaultScreen);

        if (card) {
            uiRectangleSetColor((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
            uiElementSetSize(card, 0, 60);

            // Level name
            UILabel *nameLabel = uiCreateLabel(&defaultScreen);

            if (nameLabel) {
                uiLabelSetText(nameLabel, tmpName);
                uiElementSetPosition((UIElement *)nameLabel, -listWidth + 48, -17);
                uiElementSetScale((UIElement *)nameLabel, 0.54f);
                nameLabel->base.w = 120;
                uiElementAddChild(card, (UIElement *)nameLabel);
            }

            // Level creator
            UILabel *creatorLabel = uiCreateLabel(&defaultScreen);
            if (creatorLabel) {
                uiLabelSetText(creatorLabel, tmpCreator);
                uiElementSetPosition((UIElement *)creatorLabel, -listWidth + 48, -4.5f);
                uiElementSetScale((UIElement *)creatorLabel, 0.45f);

                creatorLabel->font = 2;

                uiElementAddChild(card, (UIElement *)creatorLabel);
            }

            // copy icon
            UIImage *collaborationIcon = uiCreateImage(&defaultScreen);
            if (collaborationIcon && entry->originalId != 0) {
                uiImageSetImage(collaborationIcon, 213, 0);
                uiElementSetPosition((UIElement *)collaborationIcon, -listWidth + 48 + getTextLength(&goldFontFontCharset, 0.45f, false, creatorName) + 27, -4.5f);
                uiElementSetScale((UIElement *)collaborationIcon, 0.7f);

                uiElementAddChild(card, (UIElement *)collaborationIcon);
            }

            // high object count icon
            UIImage *highObjectIcon = uiCreateImage(&defaultScreen);
            if (highObjectIcon && (entry->objCount >= (isN3Ds ? 44000 : 14000))) {
                uiImageSetImage(highObjectIcon, 362, 0);
                uiElementSetPosition((UIElement *)highObjectIcon, -listWidth + 48 + getTextLength(&goldFontFontCharset, 0.45f, false, creatorName) + 27 + ((entry->originalId != 0) ? 11 : 0), -4.5f);
                uiElementSetScale((UIElement *)highObjectIcon, 0.7f);

                uiElementAddChild(card, (UIElement *)highObjectIcon);
            }

            // Level song
            UILabel *songLabel = uiCreateLabel(&defaultScreen);
            if (songLabel) {
                uiLabelSetText(songLabel, tmpSong);
                uiElementSetPosition((UIElement *)songLabel, -listWidth + 48, 7);
                uiElementSetScale((UIElement *)songLabel, 0.35f);
                songLabel->base.w = 130;
                uiElementAddChild(card, (UIElement *)songLabel);
            }

            // Level length
            UILabel *lengthLabel = uiCreateLabel(&defaultScreen);
            if (lengthLabel) {
                const char *levelLength = "Unkn.";
                if (IN_BOUNDS((size_t)entry->lengthNum, levelLengths)) {
                    levelLength = (const char *) levelLengths[entry->lengthNum];
                }

                uiLabelSetText(lengthLabel, levelLength);
                uiElementSetPosition((UIElement *)lengthLabel, -listWidth + 60, 19.3f);
                uiElementSetScale((UIElement *)lengthLabel, 0.35f);
                lengthLabel->base.w = 30;
                uiElementAddChild(card, (UIElement *)lengthLabel);
            }

            // Downloads
            UILabel *downloadValue = uiCreateLabel(&defaultScreen);
            if (downloadValue) {
                char *tmpValue = truncateNumber(entry->downloads);

                uiLabelSetText(downloadValue, tmpValue);
                uiElementSetPosition((UIElement *)downloadValue, -listWidth + 110, 19.3f);
                uiElementSetScale((UIElement *)downloadValue, 0.35f);

                uiElementAddChild(card, (UIElement *)downloadValue);
            }

            // Likes
            UILabel *likeValue = uiCreateLabel(&defaultScreen);
            if (likeValue) {
                char *tmpValue = truncateNumber(entry->likes);

                uiLabelSetText(likeValue, tmpValue);
                uiElementSetPosition((UIElement *)likeValue, -listWidth + 160, 19.3f);
                uiElementSetScale((UIElement *)likeValue, 0.35f);

                uiElementAddChild(card, (UIElement *)likeValue);
            }

            // Stars
            UILabel *starValue = uiCreateLabel(&defaultScreen);
            if (starValue && entry->stars > 0) {
                char tmpValue[16];

                snprintf(tmpValue, sizeof(tmpValue), "%d", entry->stars);

                uiLabelSetText(starValue, tmpValue);
                uiElementSetPosition((UIElement *)starValue, -listWidth + 22, 20.3f);
                uiElementSetScale((UIElement *)starValue, 0.35f);

                starValue->alignment = 1.f;

                uiElementAddChild(card, (UIElement *)starValue);
            }

            UIImage *featuredGlow = uiCreateImage(&defaultScreen);
            if (featuredGlow && entry->featureScore > 0) {
                int featuredID = 0;
                int yOffset = 0;

                if(entry->epic > 0 && IN_BOUNDS((size_t)entry->epic, epics)){
                    featuredID = (gdps ? SUPER_GLOW : epics[entry->epic]);
                    yOffset = (gdps ? -1 : -2);
                } else if(entry->featureScore > 0) {
                    featuredID = FEATURED_GLOW;
                }

                if(!entry->stars) yOffset += 4;

                uiImageSetImage(featuredGlow, featuredID, 0);
                uiElementSetPosition((UIElement *)featuredGlow, -listWidth + 23 + (gdps ? 0.3 : 0), -8.5f + yOffset);
                uiElementSetScale((UIElement *)featuredGlow, 0.82f);

                uiElementAddChild(card, (UIElement *)featuredGlow);
            }

            UIImage *difficultyFace = uiCreateImage(&defaultScreen);
            if (difficultyFace) {
                int difficulty_id = NA_FACE;

                if(entry->isAuto) {
                    difficulty_id = AUTO_FACE;
                } else if (entry->isDemon && gdps) {
                    difficulty_id = 258;
                } else if(entry->isDemon && IN_BOUNDS((size_t)entry->difficulty, demonFaces)) {
                    difficulty_id = demonFaces[entry->difficulty];
                } else if (IN_BOUNDS((size_t)entry->difficulty, difficultyFaces)) {
                    difficulty_id = difficultyFaces[entry->difficulty];
                }

                uiImageSetImage(difficultyFace, difficulty_id, 0);
                uiElementSetPosition((UIElement *)difficultyFace, -listWidth + 23, (entry->stars > 0) ? -4 : 0);
                uiElementSetScale((UIElement *)difficultyFace, 0.82f);

                uiElementAddChild(card, (UIElement *)difficultyFace);
            }

            UIImage *starIcon = uiCreateImage(&defaultScreen);
            if (starIcon && entry->stars > 0) {
                uiImageSetImage(starIcon, 170, 0);
                uiElementSetPosition((UIElement *)starIcon, -listWidth + 29, 20);
                uiElementSetScale((UIElement *)starIcon, 0.71f);

                uiElementAddChild(card, (UIElement *)starIcon);
            }

            UIImage *lengthIcon = uiCreateImage(&defaultScreen);
            if (lengthIcon) {
                uiImageSetImage(lengthIcon, 197, 0);
                uiElementSetPosition((UIElement *)lengthIcon, -listWidth + 53, 20);
                uiElementSetScale((UIElement *)lengthIcon, 0.5f);

                uiElementAddChild(card, (UIElement *)lengthIcon);
            }

            UIImage *downloadIcon = uiCreateImage(&defaultScreen);
            if (downloadIcon) {
                uiImageSetImage(downloadIcon, 163, 0);
                uiElementSetPosition((UIElement *)downloadIcon, -listWidth + 103, 20);
                uiElementSetScale((UIElement *)downloadIcon, 0.7f);

                uiElementAddChild(card, (UIElement *)downloadIcon);
            }

            UIImage *likeIcon = uiCreateImage(&defaultScreen);
            if (likeIcon) {
                uiImageSetImage(likeIcon, 97, 0);
                uiElementSetPosition((UIElement *)likeIcon, -listWidth + 153, 19);
                uiElementSetScale((UIElement *)likeIcon, 0.5f);

                if (entry->likes < 0) {
                    uiImageSetImage(likeIcon, DISLIKE_ICON, 0);
                }

                uiElementAddChild(card, (UIElement *)likeIcon);
            }

            // Exclamation mark
            float version = deriveGJVersion(entry->gameVersion);
            if (version > GD_VERSION) {
                UIButton *vWarnButton = uiCreateButton(&defaultScreen);
                if (vWarnButton) {
                    VersionWarningData *data = (VersionWarningData *)malloc(sizeof(*data));
                    data->version = version;
                    if (entry->levelId <= LAST_GD_VERSION_ID) {
                        // Made in or before GD_VERSION but updated after it
                        data->wasUpdated = true;
                        uiButtonSetImage(vWarnButton, 426, 0);
                    } else {
                        data->wasUpdated = false;
                        uiButtonSetImage(vWarnButton, 315, 0);
                    }
                    float tmp = getTextLength(&bigFontFontCharset, 0.54f, false, tmpName);
                    // level name width is capped at 130 so we replicate that here
                    if (tmp > 120) tmp = 120;
                    uiElementSetPosition((UIElement *)vWarnButton, -listWidth + 48 + tmp + 10, -17);
                    uiElementSetScale((UIElement *)vWarnButton, 0.3f);
                    vWarnButton->base.actions = parseActions(
                        "open_menu[screen=\"info_card\" btm_anim=\"zoom\"],open_version_warning", 
                        onlineActions, 
                        ARRAY_LEN(onlineActions), 
                        &vWarnButton->base.actionCount
                    );
                    uiElementSetUserdata((UIElement *)vWarnButton, data);
                    uiElementAddChild(card, (UIElement *)vWarnButton);
                }
            }

            UIWindowButton *button = uiCreateWindowButton(&defaultScreen);
            if (button) {
                // Store in the user data
                OnlineCardData *data = (OnlineCardData *)malloc(sizeof(*data));

                data->entryId = i;
                data->levelId = entry->levelId;
                data->redownload = false;

                SavedLevelDataEntry *savedData = getSavedLevelData(entry->levelId);
                if (!savedData) {
                    uiWindowButtonSetStyle(button, 10);
                    uiButtonSetText((UIButton *)button, "Get");
                    button->base.textScale = 0.48f;
                } else {
                    if (savedData->searchEntry.levelVersion != entry->levelVersion) {
                        uiWindowButtonSetStyle(button, 14);
                        uiButtonSetText((UIButton *)button, "Update");
                        button->base.textScale = 0.30f;
                        data->redownload = true;
                    } else if (savedData->version != SAVED_LEVELS_VERSION) {
                        uiWindowButtonSetStyle(button, 10);
                        uiButtonSetText((UIButton *)button, "Refresh");
                        button->base.textScale = 0.30f;
                        data->redownload = true;
                    } else {
                        uiWindowButtonSetStyle(button, 5);
                        uiButtonSetText((UIButton *)button, "View");
                        button->base.textScale = 0.48f;
                    }
                }


                uiElementSetPosition((UIElement *)button, listWidth - 32, 0);
                uiElementSetSize((UIElement *)button, 48, 28);
                uiElementSetAction((UIElement *)button, actionOpenOnlineLevelMenu);
                uiElementSetUserdata((UIElement *) button, data);
                uiElementAddChild(card, (UIElement *)button);
            }

            LevelDataEntry *levelData = getOnlineLevelData(entry->levelId);
            if (levelData && levelData->data.normalProgress == 100) {
                UIImage *completedIcon = uiCreateImage(&defaultScreen);
                if (completedIcon) {
                    uiImageSetImage(completedIcon, 40, 0);
                    uiElementSetPosition((UIElement *)completedIcon, listWidth - 62, -17);
                    uiElementSetScale((UIElement *)completedIcon, 0.7f);

                    uiElementAddChild(card, (UIElement *)completedIcon);
                }
            }

            uiListAdd(list, card);
        }
    }  
}

static void handleErrors(int code) {
    uiDisableElement((UIElement *) spinner);
    char temp[64];
    switch (code) {
        case -2:
            //me when gdps returns -2 when no levels are found
            if (gdps) break;
            uiLabelSetText(errorLabel, "An unknown error has\n occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            uiLabelSetText(errorLabel, "No\n<#41e24e>Internet</> connection!");
            break;
        case 28:
            uiLabelSetText(errorLabel, "Connection timed out!");
            break;
        case 42:
            break;
        default:
            snprintf(temp, sizeof(temp), "An unknown error has\noccurred.\n\nError code: %d", code);
            uiLabelSetText(errorLabel, temp);

    }
}

static void onlineMenuInit(UIScreen *s) {
    spinner = (UISpinner *) uiGetElementByTag(s, "spinner");

    list = (UIList *) uiGetElementByTag(s, "list");
    errorLabel = (UILabel *)uiGetElementByTag(s, "errorLabel");
    pageInfoLabel = (UILabel *)uiGetElementByTag(&s->pair->screens[SCREEN_TOP], "pageinfo");

    uiRunFuncOnTag(s, "nextpage", uiDisableElement);
    uiRunFuncOnTag(s, "prevpage", uiDisableElement);

    searchResult = -2;
    
    if (searchNeedsRefresh) {
        thread = createGenericThread(&searchTask);
    } else {
        if (list) { // No errors
            populateList();
            updateArrows(s);
        }
    }

    playMenuSong();
}

static void onlineMenuUpdate(UIScreen *s, UIInput *i) {
    // Run when finished
        if (searchTask.finished) {
            searchResult = searchTask.result;
            // Handle result
            if (searchResult != 0 && searchNeedsRefresh) {
                handleErrors(searchResult);
            } else if (list) { // No errors
                populateList();
                updateArrows(s);
                searchNeedsRefresh = false;
            }
            searchTask.finished = false;
        }
}

static void onlineMenuExit(UIScreen *s) {
    if (searchTask.running) {
        searchTask.cancelled = true;
        threadJoin(thread, U64_MAX);
    }
}


extern const UIScreenDefPair onlineDef = {
    .name = "online_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_levels_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_levels.txt",
        .init = onlineMenuInit,
        .update = onlineMenuUpdate,
        .exit = onlineMenuExit,
        .actionList = {
            .actions = onlineActions,
            .actionCount = ARRAY_LEN(onlineActions)
        }
    }
};
