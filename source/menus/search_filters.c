#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "save/saving.h"
#include "save/config.h"
#include "search_menu.h"
#include "search_filters.h"
#include "length_filter.h"
#include "song_filter.h"
#include "utils/server_utils.h"

static bool yes_exit = false;
static bool in_song_pop_up = false;
static bool in_length_pop_up = false;

static UIScreen screen = {
    .isBottom = true
};

static void reset_checkboxes(){
    //set checkboxes to their saved values
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_uncompleted")), filters.uncompleted);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_completed")), filters.completed);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_original")), filters.original);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_unrated")), filters.noStar);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_rated")), filters.star);
    ui_set_checkbox_checked(((UICheckBox *)ui_get_element_by_tag(&screen, "chk_featured")), filters.featured);
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
    update_difficulty_tints();

    reset_checkboxes();

    yes_exit = true;
    cfg_save();
}

void exit_search_filters(UIElement* e) {
    yes_exit = true;
}

void uncompleted_filter(UIElement* e) {
    filters.uncompleted = ((UICheckBox *)e)->checked;
}

void completed_filter(UIElement* e) {
    filters.completed = ((UICheckBox *)e)->checked;
}

void original_filter(UIElement* e) {
    filters.original = ((UICheckBox *)e)->checked;
}

void unrated_filter(UIElement* e) {
    filters.noStar = ((UICheckBox *)e)->checked;
}

void rated_filter(UIElement* e) {
    filters.star = ((UICheckBox *)e)->checked;
}

void featured_filter(UIElement* e) {
    filters.featured = ((UICheckBox *)e)->checked;
}

void open_song(UIElement* e) {
    in_song_pop_up = true;
    song_filter_init();
}

void open_length(UIElement* e) {
    in_length_pop_up = true;
    length_filter_init();
}

static UIAction actions[] = {
    { "exit", exit_search_filters },
    { "uncompleted", uncompleted_filter },
    { "completed", completed_filter },
    { "original", original_filter },
    { "unrated", unrated_filter },
    { "rated", rated_filter },
    { "featured", featured_filter },
    { "song", open_song },
    { "length", open_length },
};

void search_filters_init() {
    in_song_pop_up = false;
    in_length_pop_up = false;

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_filters_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    reset_checkboxes();

    yes_exit = false;
}

int search_filters_loop() {
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;

    if (!in_length_pop_up && !in_song_pop_up) ui_screen_update(&screen, &touch);

    if (yes_exit) {
        cfg_save();

        ui_unload_screen(&screen);

        return true;
    }

    if (in_length_pop_up) {
        int returned = length_filter_loop();
        if (returned) {
            in_length_pop_up = false;
        }
    }

    if (in_song_pop_up) {
        int returned = song_filter_loop();
        if (returned) {
            in_song_pop_up = false;
        }
    }

    return false;
}

void search_filters_draw() {
    ui_screen_draw(&screen);
    if (in_length_pop_up) length_filter_draw();
    if (in_song_pop_up) song_filter_draw();
}