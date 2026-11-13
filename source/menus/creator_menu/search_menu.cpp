#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.hpp"
#include "graphics.hpp"

#include "menus/core/ui_element.hpp"
#include "utils/server_utils.hpp"

#include "save/saving.hpp"
#include "save/config.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/creator_menu/search/server_switcher.hpp"
#include "menus/creator_menu/search/search_filters.hpp"
#include "menus/creator_menu/search/clear_search_filters.hpp"
#include "menus/creator_menu/search/song_filter.hpp"

bool searchNeedsRefresh = true;
bool gdps = false;

bool userCoinsCounterVisible(void) {
    return !gdps;
}

static UIScreen *btm_screen = NULL;

static void updateDifficultyTint(UIElement *e){
    int tint = (filters.difficultyFilters & (uiPropInt(&e->customProperties, "diffValue", 0))) > 0 ? 255 : 127;
    C2D_PlainImageTint(&((UIButton *)e)->image.tint, C2D_Color32(tint, tint, tint, 255), 1.f);
}

void enableDemons(UIScreen *s){
    if(gdps) return;

    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "easy")), 259, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "normal")), 261, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "hard")), 257, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "harder")), 263, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "insane")), 265, 0);
}

void disableDemons(UIScreen *s){
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "easy")), 252, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "normal")), 253, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "hard")), 254, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "harder")), 255, 0);
    uiButtonSetImage(((UIButton *)uiGetElementByTag(s, "insane")), 256, 0);
}

void updateDifficultyTints(UIScreen *s){
    C2D_PlainImageTint(
        &((UIButton *)uiGetElementByTag(s, "na"))->image.tint, 
        filters.isNA ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)uiGetElementByTag(s, "auto"))->image.tint, 
        filters.isAuto ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)uiGetElementByTag(s, "demon"))->image.tint, 
        filters.isDemon ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    uiRunFuncOnTag(s, "difficulty", updateDifficultyTint);
}

static void actionNa(UIElement *e, const UIPropertyList *args){
    filters.isNA = !filters.isNA;
    if(filters.isNA){
        filters.isAuto = false;
        filters.isDemon = false;
        disableDemons(e->screen);
        filters.difficultyFilters = 0;
    }
    updateDifficultyTints(e->screen);
}

static void actionAuto(UIElement *e, const UIPropertyList *args){
    filters.isAuto = !filters.isAuto;
    if(filters.isAuto){
        filters.isNA = false;
        filters.isDemon = false;
        disableDemons(e->screen);
        filters.difficultyFilters = 0;
    }
    updateDifficultyTints(e->screen);
}

static void actionDemon(UIElement *e, const UIPropertyList *args){
    filters.isDemon = !filters.isDemon;
    if(filters.isDemon){
        filters.isNA = false;
        filters.isAuto = false;
        filters.difficultyFilters = 0;
        enableDemons(e->screen);
    } else{
        filters.difficultyFilters = 0;
        disableDemons(e->screen);
    }
    updateDifficultyTints(e->screen);
}

static void actionSetDifficulty(UIElement *e, const UIPropertyList *args){
    filters.isNA = false;
    filters.isAuto = false;
    int difficultyVal = uiPropInt(&e->customProperties, "diffValue", 0);

    if(filters.isDemon && !gdps) filters.difficultyFilters &= difficultyVal;
    else filters.isDemon = false;

    filters.difficultyFilters ^= difficultyVal;
    updateDifficultyTints(e->screen);
}


void actionSetQuery(UIElement* e, const UIPropertyList *args){
    snprintf(filters.searchQuery, sizeof(filters.searchQuery), "%.*s", (int)sizeof(filters.searchQuery) - 1, ((UITextbox *)e)->text);
}

static void actionClearSearch(UIElement* e, const UIPropertyList *args){
    UITextbox *searchBox = ((UITextbox *)uiGetElementByTag(e->screen, "searchbox"));
    if (!searchBox) return;

    searchBox->text[0] = '\0';
    filters.searchQuery[0] = '\0';
}

static void actionSearch(UIElement* e, const UIPropertyList *args) {
    filters.searchType = uiPropInt(&e->customProperties, "type", 0);
    filters.currentPage = 0;
    searchNeedsRefresh = true;
    uiStackPush(&onlineDef, ANIM_NONE, ANIM_NONE, PUSH_ROOT);
}

static UIActionDef searchActions[] = {
    {"search", actionSearch },
    {"clearsearch", actionClearSearch },
    {"searchtext", actionSetQuery},
    {"difficulty", actionSetDifficulty },
    {"na", actionNa },
    {"auto", actionAuto },
    {"demon", actionDemon },
};

static void searchMenuInit(UIScreen *s) {
    btm_screen = s;

    UITextbox *searchBox = ((UITextbox *)uiGetElementByTag(s, "searchbox"));
    snprintf(searchBox->text, sizeof(searchBox->text), "%s", filters.searchQuery);

    updateDifficultyTints(s);
    if(filters.isDemon) enableDemons(s);
}

static void searchMenuExit(UIScreen *s) {
    cfgSave();
}

extern const UIScreenDefPair searchMenuDef = {
    .name = "search_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/search_menu_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/search_menu.txt",
        .init = searchMenuInit,
        .exit = searchMenuExit,
        .actionList = {
            .actions = searchActions,
            .actionCount = ARRAY_LEN(searchActions)
        }
    }
};
