#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "save/saving.h"
#include "save/config.h"
#include "search_filters.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

static UITextbox *song_input;
static UIWindowButton *custom_button;
static UIWindowButton *normal_button;

const char *songs_tags[] = {
    "page1",
    "page2",
    "page3",
    "page4",
    "page5",
    "page6",
    "page7",
    "page8",
    "page9",
    "page10",
    "page11",
    "page12",
    "page13",
    "page14",
    "page15",
    "page16",
    "page17",
    "page18",
};


void switch_song(int song) {
    for (int i = 0; i < ARRAY_LEN(songs_tags); i++) {
        if (i == song) {
            ui_run_func_on_tag(&screen, songs_tags[song], ui_enable_element);
        } else {
            ui_run_func_on_tag(&screen, songs_tags[i], ui_disable_element);
        }
    }
}

void action_left_song(UIElement *e) {
    normalSongId--;
    if (normalSongId < 0) {
        normalSongId = ARRAY_LEN(songs_tags) - 1;
    }

    switch_song(normalSongId);
}

void action_right_song(UIElement *e) {
    normalSongId++;
    if (normalSongId >= ARRAY_LEN(songs_tags)) {
        normalSongId = 0;
    }

    switch_song(normalSongId);
}

void exit_song_filter(UIElement* e) {
    yes_exit = true;
}

void select_normal() {
    ui_window_button_set_style(custom_button, 5);
    ui_window_button_set_style(normal_button, 10);
    strncpy(songFilterId, "", sizeof(songFilterId) - 1);
    strncpy(song_input->text, "", sizeof(songFilterId) - 1);
    ui_run_func_on_tag(&screen, "songselector", ui_enable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_disable_element);
    switch_song(normalSongId);
    customSelected = false;
}

void select_custom() {
    ui_window_button_set_style(custom_button, 10);
    ui_window_button_set_style(normal_button, 5);
    ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_enable_element);
    for (int i = 0; i < ARRAY_LEN(songs_tags); i++) {
        ui_run_func_on_tag(&screen, songs_tags[i], ui_disable_element);
    }
    normalSongId = 0;
    customSelected = true;
}

void song_filter(UIElement *e) {
    songFilter = ((UICheckBox *)e)->checked;

    ui_run_func_on_tag(&screen, "button", songFilter ? ui_enable_element : ui_disable_element);

    if (songFilter && customSelected) select_custom(); else select_normal();
}

static UIAction actions[] = {
    { "song", song_filter},
    { "selectnormal", select_normal },
    { "selectcustom", select_custom },
    { "exit", exit_song_filter },
    { "left", action_left_song},
    { "right", action_right_song}
};

void song_filter_init() {

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/song_filter_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    
    ui_run_func_on_tag(&screen, "button", songFilter ? ui_enable_element : ui_disable_element);

    song_input = (UITextbox *)ui_get_element_by_tag(&screen, "songinput");
    normal_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "normalbutton");
    custom_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "custombutton");

    strncpy(song_input->text, songFilterId, 127);

    if (songFilter){
        if (customSelected) {
            select_custom();
            ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
            for (int i = 0; i < ARRAY_LEN(songs_tags); i++)
            {
                ui_run_func_on_tag(&screen, songs_tags[i], ui_disable_element);
            }
        } else select_normal();
    } else {
        ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
        ui_run_func_on_tag(&screen, "songinput", ui_disable_element);
        for (int i = 0; i < ARRAY_LEN(songs_tags); i++)
        {
            ui_run_func_on_tag(&screen, songs_tags[i], ui_disable_element);
        }
    }

    UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, "chk_song");
    if (checkbox) {
            checkbox->checked = songFilter;
            ui_set_checkbox_checked(checkbox, checkbox->checked);
        }

    yes_exit = false;
}

int song_filter_loop() {
    if (yes_exit) {
        cfg_save();
        ui_unload_screen(&screen);

        return true;
    };

    
    if (!songFilter) {
        strncpy(songFilterId, "", sizeof(songFilterId) - 1);
        ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
        ui_run_func_on_tag(&screen, "songinput", ui_disable_element);
        normalSongId = 0;
        switch_song(normalSongId);
        for (int i = 0; i < ARRAY_LEN(songs_tags); i++)
        {
            ui_run_func_on_tag(&screen, songs_tags[i], ui_disable_element);
        }
    }
    strncpy(songFilterId, song_input->text, 127);

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);

    return false;
}

void song_filter_draw(){
    ui_screen_draw(&screen);
}