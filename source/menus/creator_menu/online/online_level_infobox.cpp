#include <3ds.h>
#include <citro2d.h>

#include "save/saving.hpp"
#include "utils/server_utils.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "online_level_menu.hpp"

namespace gd3ds
{

static UILabel *levelName;
static UILabel *levelCreator;
static UILabel *uploadedAgo;
static UILabel *updatedAgo;
static UILabel *requestedStars;
static UILabel *gameVer;

static UILabel *attempts;
static UILabel *jumps;
static UILabel *normalPercent;
static UILabel *practicePercent;

static void populateOnlineInfo() {
    SearchEntry *currEntry;
    CreatorEntry *creatorEntry;
    LevelEntry *lvlEntry;
    SavedLevelDataEntry *data = getSavedLevelData(onlineMenuLevelID);
    if (data) {
        currEntry = &data->searchEntry;
        creatorEntry = &data->creatorEntry;
        lvlEntry = &data->levelEntry;
    } else {
        currEntry = &searchEntries[currSearchID];
        creatorEntry = &creatorEntries[currEntry->creatorIndex];
        lvlEntry = levelEntry;
    }

    char buffer[256];

    snprintf(buffer, sizeof(buffer), "<#ffff00>%s</>", currEntry->name);
    uiLabelSetText(levelName, buffer);

    snprintf(buffer, sizeof(buffer), "By: <#ffff00>%s</>", creatorEntry->creatorName);
    uiLabelSetText(levelCreator, buffer);
    
    if (lvlEntry && lvlEntry->updateDate[0] != '\0') {
        snprintf(
            buffer, sizeof(buffer), 
            gdps ? "Uploaded: <#ffff00>%s</>" : "Uploaded: <#ffff00>%s ago</>", 
            lvlEntry->uploadDate
        );
        uiLabelSetText(uploadedAgo, buffer);
        
        snprintf(buffer, sizeof(buffer), "Updated: <#ffff00>%s ago</>", lvlEntry->updateDate);
        uiLabelSetText(updatedAgo, buffer);
    } else {
        uiLabelSetText(uploadedAgo, "Uploaded: <#ffff00>N/A</>");
        uiLabelSetText(updatedAgo, "Updated: <#ffff00>N/A</>");
    }

    snprintf(buffer, sizeof(buffer), "Stars Requested: <#ffff00>%d</>", currEntry->reqStars);
    uiLabelSetText(requestedStars, buffer);
    
    snprintf(buffer, sizeof(buffer), "Game Version: <#ffff00>%.1f</>", deriveGJVersion(currEntry->gameVersion));
    uiLabelSetText(gameVer, buffer);
}

static void populateStats() {
    LevelData *data = &currentLevelEntry->data;

    char attemptsCount[256];
    snprintf(attemptsCount, sizeof(attemptsCount), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumpsCount[256];
    snprintf(jumpsCount, sizeof(jumpsCount), "<#60abef>Total Jumps</>: %d", data->jumps);

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normalProgress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practiceProgress);

    uiLabelSetText(attempts, attemptsCount);
    uiLabelSetText(jumps, jumpsCount);
    uiLabelSetText(normalPercent, normal);
    uiLabelSetText(practicePercent, practice);
}

static void onlineLevelInfoboxInitTop(UIScreen *screenTop) {
    levelName = (UILabel *) uiGetElementByTag(screenTop, "name");
    levelCreator = (UILabel *) uiGetElementByTag(screenTop, "creator");
    uploadedAgo = (UILabel *) uiGetElementByTag(screenTop, "uploaded");
    updatedAgo = (UILabel *) uiGetElementByTag(screenTop, "lastupdated");
    requestedStars = (UILabel *) uiGetElementByTag(screenTop, "requestedstars");
    gameVer = (UILabel *) uiGetElementByTag(screenTop, "gdversion");
    
    populateOnlineInfo();
}

static void onlineLevelInfoboxInit (UIScreen *screen) {
    attempts = (UILabel *) uiGetElementByTag(screen, "totalattempts");
    jumps = (UILabel *) uiGetElementByTag(screen, "totaljumps");
    normalPercent = (UILabel *) uiGetElementByTag(screen, "normalprogressvalue");
    practicePercent = (UILabel *) uiGetElementByTag(screen, "practiceprogressvalue");

    populateStats();
}

extern const UIScreenDefPair onlineInfoboxDef = {
    .name = "online_infobox",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_level_info_pop_up_top.txt",
        .init = onlineLevelInfoboxInitTop
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_info_pop_up.txt",
        .init = onlineLevelInfoboxInit,
    }
};

}