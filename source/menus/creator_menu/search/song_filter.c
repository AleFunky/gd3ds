#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "utils/server_utils.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "menus/creator_menu/search/search_filters.h"
#include "menus/settings_hub/songs.h"

static UITextbox *song_input;
static UIWindowButton *custom_button;
static UIWindowButton *normal_button;

void switch_song(int song, UIScreen *s) {
    UILabel *label = (UILabel *)ui_get_element_by_tag(s, "normal_song_text");
    if(label){
        char tmp[64];
        snprintf(tmp, sizeof(tmp) - 1, "%02d. %s\n", song + 1, main_songs[song].title);
        ui_label_set_text(label, tmp);
    }
}

void action_left_song(UIElement* e, const UIPropertyList *props) {
    filters.mainSong--;
    if (filters.mainSong < 0) {
        filters.mainSong = ARRAY_LEN(main_songs) - 1;
    }

    switch_song(filters.mainSong, e->screen);
}

void action_right_song(UIElement* e, const UIPropertyList *props) {
    filters.mainSong++;
    if (filters.mainSong >= ARRAY_LEN(main_songs)) {
        filters.mainSong = 0;
    }

    switch_song(filters.mainSong, e->screen);
}

void select_normal(UIElement* e, const UIPropertyList *args) {
    ui_window_button_set_style(custom_button, 5);
    ui_window_button_set_style(normal_button, 10);

    ui_run_func_on_tag(e->screen, "songselector", ui_enable_element);
    ui_run_func_on_tag(e->screen, "normal_song_text", ui_enable_element);
    ui_run_func_on_tag(e->screen, "songinput", ui_disable_element);

    filters.customSongQuery[0] = '\0';
    song_input->text[0] = '\0';

    switch_song(filters.mainSong, e->screen);
    filters.customSong = false;
}

void select_custom(UIElement* e, const UIPropertyList *args) {
    ui_window_button_set_style(custom_button, 10);
    ui_window_button_set_style(normal_button, 5);

    ui_run_func_on_tag(e->screen, "songselector", ui_disable_element);
    ui_run_func_on_tag(e->screen, "normal_song_text", ui_disable_element);
    ui_run_func_on_tag(e->screen, "songinput", ui_enable_element);
    
    filters.customSong = true;
}

void song_filter(UIElement* e, const UIPropertyList *props) {
    filters.songFilter = ((UICheckBox *)e)->checked;
    UITextbox *textbox = ((UITextbox *)ui_get_element_by_tag(e->screen, "songinput"));

    if (filters.songFilter){
        if (filters.customSong) {
            select_custom(e, NULL);
            snprintf(textbox->text, sizeof(textbox->text), "%s", filters.customSongQuery);

        } else select_normal(e, NULL);
    } else {
        ui_run_func_on_tag(e->screen, "songselector", ui_disable_element);
        ui_run_func_on_tag(e->screen, "normal_song_text", ui_disable_element);
        ui_run_func_on_tag(e->screen, "songinput", ui_disable_element);

        textbox->text[0] = '\0';
        filters.customSongQuery[0] = '\0';

        filters.mainSong = 0;

        switch_song(filters.mainSong, e->screen);
    }

    ui_run_func_on_tag(e->screen, "button", filters.songFilter ? ui_enable_element : ui_disable_element);
}

void action_custom_song_query(UIElement* e, const UIPropertyList *props){
    snprintf(filters.customSongQuery, sizeof(filters.customSongQuery), "%.*s", (int)sizeof(filters.customSongQuery) - 1, ((UITextbox *)e)->text);
}

static UIActionDef song_filter_actions[] = {
    { "song", song_filter},
    { "selectnormal", select_normal },
    { "selectcustom", select_custom },
    { "left", action_left_song },
    { "right", action_right_song },
    { "customsongquery", action_custom_song_query }
};

void song_filter_init(UIScreen *s) {
    ui_run_func_on_tag(s, "button", filters.songFilter ? ui_enable_element : ui_disable_element);

    song_input = (UITextbox *)ui_get_element_by_tag(s, "songinput");
    normal_button = (UIWindowButton *)ui_get_element_by_tag(s, "normalbutton");
    custom_button = (UIWindowButton *)ui_get_element_by_tag(s, "custombutton");

    UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(s, "chk_song");
    if (checkbox) {
        checkbox->checked = filters.songFilter;
        ui_set_checkbox_checked(checkbox, checkbox->checked);
    }

    song_filter((UIElement *)checkbox, NULL);
}

const UIScreenDefPair song_filter_def = {
    .name = "song_filter",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/song_filter_pop_up.txt",
        .init = song_filter_init,
        .action_list = {
            .action_count = ARRAY_LEN(song_filter_actions),
            .actions = song_filter_actions
        }
    }
};