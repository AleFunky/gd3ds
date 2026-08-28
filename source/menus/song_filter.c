#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "menus/songs.h"
#include "search_filters.h"
#include "utils/server_utils.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

static UITextbox *song_input;
static UIWindowButton *custom_button;
static UIWindowButton *normal_button;

void switch_song(int song) {
    UILabel *label = (UILabel *)ui_get_element_by_tag(&screen, "normal_song_text");
    if(label){
        char tmp[64];
        snprintf(tmp, sizeof(tmp) - 1, "%02d. %s\n", song + 1, main_songs[song].title);
        ui_label_set_text(label, tmp);
    }
}

void action_left_song(UIElement *e) {
    filters.mainSong--;
    if (filters.mainSong < 0) {
        filters.mainSong = ARRAY_LEN(main_songs) - 1;
    }

    switch_song(filters.mainSong);
}

void action_right_song(UIElement *e) {
    filters.mainSong++;
    if (filters.mainSong >= ARRAY_LEN(main_songs)) {
        filters.mainSong = 0;
    }

    switch_song(filters.mainSong);
}

void exit_song_filter(UIElement* e) {
    yes_exit = true;
}

void select_normal() {
    ui_window_button_set_style(custom_button, 5);
    ui_window_button_set_style(normal_button, 10);

    ui_run_func_on_tag(&screen, "songselector", ui_enable_element);
    ui_run_func_on_tag(&screen, "normal_song_text", ui_enable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_disable_element);

    filters.customSongQuery[0] = '\0';
    song_input->text[0] = '\0';

    switch_song(filters.mainSong);
    filters.customSong = false;
}

void select_custom() {
    ui_window_button_set_style(custom_button, 10);
    ui_window_button_set_style(normal_button, 5);

    ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
    ui_run_func_on_tag(&screen, "normal_song_text", ui_disable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_enable_element);
    
    filters.customSong = true;
}

void song_filter(UIElement *e) {
    filters.songFilter = ((UICheckBox *)e)->checked;
    UITextbox *textbox = ((UITextbox *)ui_get_element_by_tag(&screen, "songinput"));

    if (filters.songFilter){
        if (filters.customSong) {
            select_custom();
            snprintf(textbox->text, sizeof(textbox->text), "%s", filters.customSongQuery);

        } else select_normal();
    } else {
        ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
        ui_run_func_on_tag(&screen, "normal_song_text", ui_disable_element);
        ui_run_func_on_tag(&screen, "songinput", ui_disable_element);

        textbox->text[0] = '\0';
        filters.customSongQuery[0] = '\0';

        filters.mainSong = 0;

        switch_song(filters.mainSong);
    }

    ui_run_func_on_tag(&screen, "button", filters.songFilter ? ui_enable_element : ui_disable_element);
}

void action_custom_song_query(UIElement *e){
    snprintf(filters.customSongQuery, sizeof(filters.customSongQuery), "%.*s", (int)sizeof(filters.customSongQuery) - 1, ((UITextbox *)e)->text);
}

static UIAction actions[] = {
    { "song", song_filter},
    { "selectnormal", select_normal },
    { "selectcustom", select_custom },
    { "exit", exit_song_filter },
    { "left", action_left_song },
    { "right", action_right_song },
    { "customsongquery", action_custom_song_query }
};

void song_filter_init() {

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/song_filter_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    
    ui_run_func_on_tag(&screen, "button", filters.songFilter ? ui_enable_element : ui_disable_element);

    song_input = (UITextbox *)ui_get_element_by_tag(&screen, "songinput");
    normal_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "normalbutton");
    custom_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "custombutton");

    UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, "chk_song");
    if (checkbox) {
        checkbox->checked = filters.songFilter;
        ui_set_checkbox_checked(checkbox, checkbox->checked);
    }

    song_filter((UIElement *)checkbox);

    yes_exit = false;
}

int song_filter_loop() {
    if (yes_exit) {
        ui_unload_screen(&screen);

        return true;
    };

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