#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "math_helpers.hpp"
#include "main.hpp"
#include "easing.hpp"
#include "triggers.hpp"
#include "menus/core/ui_element.hpp"
#include "mp3_player.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "save/config.hpp"
#include "utils/folders.hpp"
#include "level_loading.hpp"

#include "menus/core/common_setters.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_textbox.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_button.hpp"

#include "menus/settings_hub/settings.hpp"
#include "menus/settings_hub/info_card.hpp"
#include "menus/creator_menu/external/external_levels.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "menus/main_menu.hpp"
#include "menus/level_select.hpp"
#include "menus/gameplay.hpp"

#include "fonts/bigFont.hpp"

#include "utils/network.hpp"
#include "utils/utils.hpp"

namespace gd3ds
{

static bool reloadPending;
static char reloadPath[320];

char currentPath[320] = { 0 };
char lastPath[320] = { 1 };

typedef struct {
    char path[256];
    bool isDir;
} LevelCardData;

static UILabel *path_label = NULL;
static UIList *list = NULL;
static UISpinner *spinner = NULL;

static Thread thread;

static FileOrFolder *entries = NULL;
static int count = 0;

static char *tmpFolder;

static int loadLevels(GenericTask *task) {
    entries = loadFolder(task, tmpFolder, &count);
    return entries == NULL;
}

static GenericTask task = {
    .func = loadLevels
};

static void openFolder(UIElement *e, const UIPropertyList* args);

static void openExternalPopup(UIElement *e, const UIPropertyList* args) {
    LevelCardData *entry = (LevelCardData *)e->userdata;
    strcpy(state.customLevelPath, entry->path);
    uiStackPush(&externalPopupDef, ANIM_ZOOM_SUBTLE, ANIM_ZOOM_SUBTLE, PUSH_NEXT);
}

void startLoad(char *folder, UIScreen *s) {
    tmpFolder = folder;

    char path[320+5];
    sprintf(path, "Root/%s", currentPath);
    truncateFilenameStart(path, 27, sizeof(path));
    
    uiLabelSetText(path_label,path);
    thread = createGenericThread(&task);
    uiListReset(list);
    uiRunFuncOnTag(s, "no_levels", uiDisableElement);
    uiRunFuncOnTag(s, "spinner", uiEnableElement);
}

void loadLevelFolder(char *folder, UIScreen *s) { 
    uiRunFuncOnTag(s, "spinner", uiDisableElement);
    uiRunFuncOnTag(s, "no_levels", uiDisableElement);

    char levelName[525];

    uiListReset(list);
    
    if (entries && list) {
        for (int i = 0; i < count; i++) {
            FileOrFolder *entry = &entries[i];
            strncpy(levelName, entry->name, sizeof(levelName) - 1);

            UIElement *card = NULL;

            u32 color = (i & 1 ? C2D_Color32(50,50,50,255) :  C2D_Color32(75,75,75,255));
            char *name = NULL;
            if (entry->isDir) {
                // Folder
                name = stripFilename(levelName);
                truncateFilename(name, 16);
            } else {
                // File
                stripExtension(levelName);
                name = stripFilename(levelName);
                truncateFilename(name, 16);
            }

            float listWidth = list->base.w * 0.5f;

            card = (UIElement *) uiCreateRectangle(s);

            if (card) {
                uiRectangleSetColor((UIRectangle *) card, color);
                uiElementSetSize(card, 0, 28);
                
                UIButton *button = uiCreateButton(s);
                if (button) {
                    // Store in the user data
                    LevelCardData *data = (LevelCardData *)malloc(sizeof(*data));

                    strcpy(data->path, entry->name);
                    data->isDir = entry->isDir;

                    uiElementSetUserdata((UIElement *) button, data);

                    uiButtonSetImage(button, (entry->isDir ? 7 : 6), 0);
                    uiElementSetPosition((UIElement *) button, listWidth - 15, 0);
                    uiElementSetScaleXy((UIElement *) button, -0.5f, 0.5f);
                    uiElementSetAction((UIElement *) button, (entry->isDir ? openFolder : openExternalPopup));

                    uiElementAddChild(card, (UIElement *) button);
                }

                UIImage *icon = uiCreateImage(s);
                if (icon) {
                    uiImageSetImage(icon, (entry->isDir ? 320 : 420), 0);
                    uiElementSetPosition((UIElement *) icon, -listWidth + 15, 0);
                    uiElementSetScale((UIElement *) icon, 0.58f);

                    uiElementAddChild(card, (UIElement *) icon);
                }

                // Name
                UILabel *label = uiCreateLabel(s);
                if (label) {
                    uiLabelSetText(label, name);
                    uiElementSetPosition((UIElement *) label, -listWidth + 29, 1);
                    uiElementSetScale((UIElement *) label, 0.54f);
                    
                    uiElementAddChild(card, (UIElement *) label);
                }

                uiListAdd(list, card);
            }
        }
        
        if (count == 0) {
            uiRunFuncOnTag(s, "no_levels", uiEnableElement);
        }
    } else {
        uiRunFuncOnTag(s, "no_levels", uiEnableElement);
    }

    strncpy(lastPath, currentPath, sizeof(lastPath));
}

static void actionGoBack(UIElement *e, const UIPropertyList *args) {
    if (strlen(currentPath) > 0) {
        if (task.running) {
            task.cancelled = true;
            threadJoin(thread, U64_MAX);
        }
        goBackDirectory(currentPath);
        startLoad(currentPath, e->screen);
    }
}

static void actionRefresh(UIElement *e, const UIPropertyList *args) {
    if (task.running) {
        task.cancelled = true;
        threadJoin(thread, U64_MAX);
    }
    reloadPending = true;
    strcpy(reloadPath, currentPath);
}

// Go my warning suppresion gadget
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

static void openFolder(UIElement *e, const UIPropertyList* args) {
    LevelCardData *entry = (LevelCardData *)e->userdata;
    char tmp[320];

    if (currentPath[0] == '\0') {
        // First level: no leading slash
        snprintf(tmp, sizeof(tmp), "%s", entry->path);
    } else {
        snprintf(tmp, sizeof(tmp), "%s/%s", currentPath, entry->path);
    }

    strncpy(currentPath, tmp, sizeof(currentPath) - 1);
    currentPath[sizeof(currentPath) - 1] = '\0';

    reloadPending = true;
    strcpy(reloadPath, currentPath);
}

#pragma GCC diagnostic pop

static UIActionDef externalActions[] = {
    {"go_back", actionGoBack },
    {"refresh", actionRefresh },
};

static void externalLevelsInit(UIScreen *s) {
    list = (UIList *) uiGetElementByTag(s, "list");
    spinner = (UISpinner *) uiGetElementByTag(s, "spinner");
    path_label = (UILabel *) uiGetElementByTag(s, "path");

    if (!isCitra()) {
        UIElement *refreshButton = uiGetElementByTag(s, "refresh");
        if (refreshButton) uiDisableElement(refreshButton);
    }

    startLoad(currentPath, s);

    playMenuSong();
}

static void externalLevelsUpdate(UIScreen *s, UIInput *i) {
    if (reloadPending) {
        startLoad(reloadPath, s);

        reloadPending = false;
    }

    if (task.finished) {
        loadLevelFolder(tmpFolder, s);
        task.finished = false;
    }
}

static void externalLevelsExit(UIScreen *s) {
    if (task.running) {
        task.cancelled = true;
        threadJoin(thread, U64_MAX);
    }
}

extern const UIScreenDefPair externalDef = {
    .name = "external_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/external/external_levels_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/external/external_levels.txt",
        .init = externalLevelsInit,
        .update = externalLevelsUpdate,
        .exit = externalLevelsExit,
        .actionList = {
            .actions = externalActions,
            .actionCount = ARRAY_LEN(externalActions)
        }
    }
};

}