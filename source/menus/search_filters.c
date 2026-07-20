#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "save/saving.h"
#include "save/config.h"
#include "search_filters.h"

static bool yes_exit = false;

bool uncompletedFilter = false;
bool completedFilter = false;
bool originalFilter = false;
bool unratedFilter = false;
bool featuredFilter = false;
bool songFilter = false;
char songFilterId[8];

static UIScreen screen = {
    .isBottom = true
};
static UIScreen screen_top = {
};

static UITextbox *song_input;

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
        "chk_featured", &featuredFilter
    },
    {
        "chk_song", &songFilter
    },
};

void reset_search_filters() {
    uncompletedFilter = false;
    completedFilter = false;
    originalFilter = false;
    unratedFilter = false;
    featuredFilter = false;
    songFilter = false;
    strncpy(songFilterId, "", sizeof(songFilterId) - 1);
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

void featured_filter(UIElement* e) {
    featuredFilter = ((UICheckBox *)e)->checked;
}

void song_filter(UIElement* e) {
    songFilter = ((UICheckBox *)e)->checked;
    ui_run_func_on_tag(&screen, "songinput", songFilter ? ui_enable_element : ui_disable_element);
}

static UIAction actions[] = {
    { "exit", exit_search_filters },
    { "uncompleted", uncompleted_filter },
    { "completed", completed_filter },
    { "original", original_filter },
    { "unrated", unrated_filter },
    { "featured", featured_filter },
    { "song", song_filter },
};

void search_filters_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_filters_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    ui_run_func_on_tag(&screen, "songinput", songFilter ? ui_enable_element : ui_disable_element);

    song_input = (UITextbox *)ui_get_element_by_tag(&screen, "songinput");

    strncpy(song_input->text, songFilterId, 8);

    for (int i = 0; i < ARRAY_LEN(filters); i++) {
        UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, filters[i].chk_name);
        if (checkbox) {
            checkbox->checked = *filters[i].var;
            set_checkbox_enabled(checkbox, checkbox->checked);
        }
    }

    yes_exit = false;
}

int search_filters_loop() {
    if (yes_exit) {
        cfg_save();

        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);

        return true;
    }

    strncpy(songFilterId, song_input->text, 8);

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_update(&screen_top, &touch);

    ui_screen_draw(&screen);

    return false;
}