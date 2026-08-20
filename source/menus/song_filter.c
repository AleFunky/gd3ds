#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "menus/songs.h"
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

void switch_song(int song) {
    UILabel *label = (UILabel *)ui_get_element_by_tag(&screen, "normal_song_text");
    if(label){
        char tmp[64];
        snprintf(tmp, sizeof(tmp) - 1, "%02d. %s\n", song + 1, main_songs[song].title);
        ui_label_set_text(label, tmp);
    }
}

void action_left_song(UIElement *e) {
    normal_song_id_selected--;
    if (normal_song_id_selected < 0) {
        normal_song_id_selected = ARRAY_LEN(main_songs) - 1;
    }

    switch_song(normal_song_id_selected);
}

void action_right_song(UIElement *e) {
    normal_song_id_selected++;
    if (normal_song_id_selected >= ARRAY_LEN(main_songs)) {
        normal_song_id_selected = 0;
    }

    switch_song(normal_song_id_selected);
}

void exit_song_filter(UIElement* e) {
    yes_exit = true;
}

void select_normal() {
    ui_window_button_set_style(custom_button, 5);
    ui_window_button_set_style(normal_button, 10);
    strncpy(custom_song_id, "", sizeof(custom_song_id) - 1);
    strncpy(song_input->text, "", sizeof(custom_song_id) - 1);
    ui_run_func_on_tag(&screen, "songselector", ui_enable_element);
    ui_run_func_on_tag(&screen, "normal_song_text", ui_enable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_disable_element);
    switch_song(normal_song_id_selected);
    custom_song = false;
}

void select_custom() {
    ui_window_button_set_style(custom_button, 10);
    ui_window_button_set_style(normal_button, 5);
    strncpy(song_input->text, custom_song_id, sizeof(custom_song_id) - 1);
    ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
    ui_run_func_on_tag(&screen, "normal_song_text", ui_disable_element);
    ui_run_func_on_tag(&screen, "songinput", ui_enable_element);
    normal_song_id_selected = 0;
    custom_song = true;
}

void song_filter(UIElement *e) {
    song_filter_enabled = ((UICheckBox *)e)->checked;

    if (song_filter_enabled){
        if (custom_song) {
            select_custom();
            ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
            ui_run_func_on_tag(&screen, "normal_song_text", ui_disable_element);
        } else select_normal();
    } else {
        ui_run_func_on_tag(&screen, "songselector", ui_disable_element);
        ui_run_func_on_tag(&screen, "songinput", ui_disable_element);
        ui_run_func_on_tag(&screen, "normal_song_text", ui_disable_element);

        strncpy(custom_song_id, "", sizeof(custom_song_id) - 1);

        normal_song_id_selected = 0;
        switch_song(normal_song_id_selected);
    }

    ui_run_func_on_tag(&screen, "button", song_filter_enabled ? ui_enable_element : ui_disable_element);
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
    
    ui_run_func_on_tag(&screen, "button", song_filter_enabled ? ui_enable_element : ui_disable_element);

    song_input = (UITextbox *)ui_get_element_by_tag(&screen, "songinput");
    normal_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "normalbutton");
    custom_button = (UIWindowButton *)ui_get_element_by_tag(&screen, "custombutton");

    strncpy(song_input->text, custom_song_id, 127);

    UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, "chk_song");
    if (checkbox) {
        checkbox->checked = song_filter_enabled;
        ui_set_checkbox_checked(checkbox, checkbox->checked);
    }

    song_filter((UIElement *)checkbox);

    yes_exit = false;
}

int song_filter_loop() {
    if (yes_exit) {
        cfg_save();
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