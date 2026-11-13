#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/search/search_filters.hpp"
#include "menus/creator_menu/search/length_filter.hpp"
#include "menus/creator_menu/search/song_filter.hpp"

#include "utils/server_utils.hpp"
#include "save/config.hpp"
#include "save/saving.hpp"

namespace gd3ds
{

static void toggleGdps(UIScreen *s){
    if(gdps){
        uiRunFuncOnTag(s, "gdps", uiEnableElement);
        uiRunFuncOnTag(s, "no_gdps", uiDisableElement);
    } else{
        uiRunFuncOnTag(s, "gdps", uiDisableElement);
        uiRunFuncOnTag(s, "no_gdps", uiEnableElement);
    }
}

static void resetCheckboxes(UIScreen *s){
    //set checkboxes to their saved values
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_uncompleted")), filters.uncompleted);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_completed")), filters.completed);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_original")), filters.original);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_unrated")), filters.noStar);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_rated")), filters.star);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_featured")), filters.featured);

    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_original_gdps")), filters.original);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_unrated_gdps")), filters.noStar);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_rated_gdps")), filters.star);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_featured_gdps")), filters.featured);
    uiSetCheckboxChecked(((UICheckBox *)uiGetElementByTag(s, "chk_super")), filters.super);
}

void resetSearchFilters() {
    filters.uncompleted = false;
    filters.completed = false;
    filters.original = false;
    filters.noStar = false;
    filters.star = false;
    filters.featured = false;
    filters.songFilter = false;
    filters.customSong = false;
    filters.mainSong = 0;
    filters.lengthFilters = 0;
    filters.difficultyFilters = 0;
    filters.isAuto = false;
    filters.isNA = false;
    filters.customSongQuery[0] = '\0';
    updateDifficultyTints(
        uiStackGetScreen("search_menu", SCREEN_BTM)
    );
}

void uncompletedFilter(UIElement* e, const UIPropertyList *args) {
    filters.uncompleted = ((UICheckBox *)e)->checked;
}

void completedFilter(UIElement* e, const UIPropertyList *args) {
    filters.completed = ((UICheckBox *)e)->checked;
}

void originalFilter(UIElement* e, const UIPropertyList *args) {
    filters.original = ((UICheckBox *)e)->checked;
}

void unratedFilter(UIElement* e, const UIPropertyList *args) {
    filters.noStar = ((UICheckBox *)e)->checked;
}

void ratedFilter(UIElement* e, const UIPropertyList *args) {
    filters.star = ((UICheckBox *)e)->checked;
}

void featuredFilter(UIElement* e, const UIPropertyList *args) {
    filters.featured = ((UICheckBox *)e)->checked;
}

void superFilter(UIElement* e, const UIPropertyList *args) {
    filters.super = ((UICheckBox *)e)->checked;
}

static UIActionDef searchFiltersActions[] = {
    { "uncompleted", uncompletedFilter },
    { "completed", completedFilter },
    { "original", originalFilter },
    { "unrated", unratedFilter },
    { "rated", ratedFilter },
    { "featured", featuredFilter },
    { "super", superFilter },
};

void searchFiltersInit(UIScreen *s) {
    toggleGdps(s);

    resetCheckboxes(s);
}

extern const UIScreenDefPair searchFiltersDef = {
    .name = "search_filters",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/search_filters_pop_up.txt",
        .init = searchFiltersInit,
        .actionList = {
            .actions = searchFiltersActions,
            .actionCount = ARRAY_LEN(searchFiltersActions)
        }
    }
};

}