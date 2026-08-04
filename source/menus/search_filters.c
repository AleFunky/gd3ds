#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "save/saving.h"
#include "save/config.h"
#include "search_filters.h"
#include "length_filter.h"
#include "song_filter.h"

static bool yes_exit = false;
static bool in_song_pop_up = false;
static bool in_length_pop_up = false;

bool uncompletedFilter = false;
bool completedFilter = false;
bool originalFilter = false;
bool unratedFilter = false;
bool ratedFilter = false;
bool featuredFilter = false;
bool song_filter_enabled = false;
bool length_filter_enabled = false;
bool custom_song = false;
int normal_song_id_selected = 0;
char custom_song_id[127];

static UIScreen screen = {
    .isBottom = true
};

static Filter filters[] = {
    {
        "chk_uncompleted", &uncompletedFilter
    },
    {
        "chk_completed", &completedFilter
    },
    {
        "chk_original", &originalFilter
    },
    {
        "chk_unrated", &unratedFilter
    },
    {
        "chk_rated", &ratedFilter
    },
    {
        "chk_featured", &featuredFilter
    },
};

void reset_search_filters() {
    uncompletedFilter = false;
    completedFilter = false;
    originalFilter = false;
    unratedFilter = false;
    ratedFilter = false;
    featuredFilter = false;
    song_filter_enabled = false;
    custom_song = false;
    normal_song_id_selected = 0;
    strncpy(custom_song_id, "", sizeof(custom_song_id) - 1);
    yes_exit = true;
    cfg_save();
}

void exit_search_filters(UIElement* e) {
    yes_exit = true;
}

void uncompleted_filter(UIElement* e) {
    uncompletedFilter = ((UICheckBox *)e)->checked;
}

void completed_filter(UIElement* e) {
    completedFilter = ((UICheckBox *)e)->checked;
}

void original_filter(UIElement* e) {
    originalFilter = ((UICheckBox *)e)->checked;
}

void unrated_filter(UIElement* e) {
    unratedFilter = ((UICheckBox *)e)->checked;
}

void rated_filter(UIElement* e) {
    ratedFilter = ((UICheckBox *)e)->checked;
}

void featured_filter(UIElement* e) {
    featuredFilter = ((UICheckBox *)e)->checked;
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

    for (int i = 0; i < ARRAY_LEN(filters); i++) {
        UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, filters[i].chk_name);
        if (checkbox) {
            checkbox->checked = *filters[i].var;
            ui_set_checkbox_checked(checkbox, checkbox->checked);
        }
    }

    yes_exit = false;
}

int search_filters_loop() {
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

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_length_pop_up && !in_song_pop_up) ui_screen_update(&screen, &touch);

    return false;
}

void search_filters_draw() {
    ui_screen_draw(&screen);
    if (in_length_pop_up) length_filter_draw();
    if (in_song_pop_up) song_filter_draw();
}