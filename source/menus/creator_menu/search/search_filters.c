#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/creator_menu/search_menu.h"
#include "menus/creator_menu/search/search_filters.h"
#include "menus/creator_menu/search/length_filter.h"
#include "menus/creator_menu/search/song_filter.h"

#include "utils/server_utils.h"
#include "save/config.h"
#include "save/saving.h"

static void toggle_gdps(UIScreen *s){
    if(gdps){
        ui_run_func_on_tag(s, "gdps", ui_enable_element);
        ui_run_func_on_tag(s, "no_gdps", ui_disable_element);
    } else{
        ui_run_func_on_tag(s, "gdps", ui_disable_element);
        ui_run_func_on_tag(s, "no_gdps", ui_enable_element);
    }
}

static void reset_checkboxes(UIScreen *s){
    //set checkboxes to their saved values
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_uncompleted")), filters.uncompleted);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_completed")), filters.completed);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_original")), filters.original);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_unrated")), filters.noStar);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_rated")), filters.star);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_featured")), filters.featured);

    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_original_gdps")), filters.original);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_unrated_gdps")), filters.noStar);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_rated_gdps")), filters.star);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_featured_gdps")), filters.featured);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(s, "chk_super")), filters.super);
}

void reset_search_filters() {
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
    filters.customSongQuery[0] = '\0';
    update_difficulty_tints(
        ui_stack_get_screen("search_menu", SCREEN_BTM)
    );
}

void uncompleted_filter(UIElement* e, const UIPropertyList *args) {
    filters.uncompleted = ((UICheckBox *)e)->checked;
}

void completed_filter(UIElement* e, const UIPropertyList *args) {
    filters.completed = ((UICheckBox *)e)->checked;
}

void original_filter(UIElement* e, const UIPropertyList *args) {
    filters.original = ((UICheckBox *)e)->checked;
}

void unrated_filter(UIElement* e, const UIPropertyList *args) {
    filters.noStar = ((UICheckBox *)e)->checked;
}

void rated_filter(UIElement* e, const UIPropertyList *args) {
    filters.star = ((UICheckBox *)e)->checked;
}

void featured_filter(UIElement* e, const UIPropertyList *args) {
    filters.featured = ((UICheckBox *)e)->checked;
}

void super_filter(UIElement* e, const UIPropertyList *args) {
    filters.super = ((UICheckBox *)e)->checked;
}

static UIActionDef search_filters_actions[] = {
    { "uncompleted", uncompleted_filter },
    { "completed", completed_filter },
    { "original", original_filter },
    { "unrated", unrated_filter },
    { "rated", rated_filter },
    { "featured", featured_filter },
    { "super", super_filter },
};

void search_filters_init(UIScreen *s) {
    toggle_gdps(s);

    reset_checkboxes(s);
}

const UIScreenDefPair search_filters_def = {
    .name = "search_filters",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/search_filters_pop_up.txt",
        .init = search_filters_init,
        .action_list = {
            .action_count = ARRAY_LEN(search_filters_actions),
            .actions = search_filters_actions
        }
    }
};