#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "utils/server_utils.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/creator_menu/search/search_filters.hpp"
#include "menus/settings_hub/songs.hpp"

namespace gd3ds
{

static UITextbox *songInput;
static UIWindowButton *customButton;
static UIWindowButton *normalButton;

void switchSong(int song, UIScreen *s) {
    UILabel *label = (UILabel *)uiGetElementByTag(s, "normal_song_text");
    if(label){
        char tmp[64];
        snprintf(tmp, sizeof(tmp) - 1, "%02d. %s\n", song + 1, currentMainLevelPack->levels[song].songData.title);
        uiLabelSetText(label, tmp);
    }
}

void actionLeftSong(UIElement* e, const UIPropertyList *props) {
    filters.mainSong--;
    if (filters.mainSong < 0) {
        filters.mainSong = currentMainLevelPack->count - 1;
    }

    switchSong(filters.mainSong, e->screen);
}

void actionRightSong(UIElement* e, const UIPropertyList *props) {
    filters.mainSong++;
    if (filters.mainSong >= currentMainLevelPack->count) {
        filters.mainSong = 0;
    }

    switchSong(filters.mainSong, e->screen);
}

void selectNormal(UIElement* e, const UIPropertyList *args) {
    uiWindowButtonSetStyle(customButton, 5);
    uiWindowButtonSetStyle(normalButton, 10);

    uiRunFuncOnTag(e->screen, "songselector", uiEnableElement);
    uiRunFuncOnTag(e->screen, "normal_song_text", uiEnableElement);
    uiRunFuncOnTag(e->screen, "songinput", uiDisableElement);

    filters.customSongQuery[0] = '\0';
    songInput->text[0] = '\0';

    switchSong(filters.mainSong, e->screen);
    filters.customSong = false;
}

void selectCustom(UIElement* e, const UIPropertyList *args) {
    uiWindowButtonSetStyle(customButton, 10);
    uiWindowButtonSetStyle(normalButton, 5);

    uiRunFuncOnTag(e->screen, "songselector", uiDisableElement);
    uiRunFuncOnTag(e->screen, "normal_song_text", uiDisableElement);
    uiRunFuncOnTag(e->screen, "songinput", uiEnableElement);
    
    filters.customSong = true;
}

void songFilter(UIElement* e, const UIPropertyList *props) {
    filters.songFilter = ((UICheckBox *)e)->checked;
    UITextbox *textbox = ((UITextbox *)uiGetElementByTag(e->screen, "songinput"));

    if (filters.songFilter){
        if (filters.customSong) {
            selectCustom(e, NULL);
            snprintf(textbox->text, sizeof(textbox->text), "%s", filters.customSongQuery);

        } else selectNormal(e, NULL);
    } else {
        uiRunFuncOnTag(e->screen, "songselector", uiDisableElement);
        uiRunFuncOnTag(e->screen, "normal_song_text", uiDisableElement);
        uiRunFuncOnTag(e->screen, "songinput", uiDisableElement);

        textbox->text[0] = '\0';
        filters.customSongQuery[0] = '\0';

        filters.mainSong = 0;

        switchSong(filters.mainSong, e->screen);
    }

    uiRunFuncOnTag(e->screen, "button", filters.songFilter ? uiEnableElement : uiDisableElement);
}

void actionCustomSongQuery(UIElement* e, const UIPropertyList *props){
    snprintf(filters.customSongQuery, sizeof(filters.customSongQuery), "%.*s", (int)sizeof(filters.customSongQuery) - 1, ((UITextbox *)e)->text);
}

static UIActionDef songFilterActions[] = {
    { "song", songFilter},
    { "selectnormal", selectNormal },
    { "selectcustom", selectCustom },
    { "left", actionLeftSong },
    { "right", actionRightSong },
    { "customsongquery", actionCustomSongQuery }
};

void songFilterInit(UIScreen *s) {
    uiRunFuncOnTag(s, "button", filters.songFilter ? uiEnableElement : uiDisableElement);

    songInput = (UITextbox *)uiGetElementByTag(s, "songinput");
    normalButton = (UIWindowButton *)uiGetElementByTag(s, "normalbutton");
    customButton = (UIWindowButton *)uiGetElementByTag(s, "custombutton");

    UICheckBox *checkbox = (UICheckBox *)uiGetElementByTag(s, "chk_song");
    if (checkbox) {
        checkbox->checked = filters.songFilter;
        uiSetCheckboxChecked(checkbox, checkbox->checked);
    }

    songFilter((UIElement *)checkbox, NULL);
}

extern const UIScreenDefPair songFilterDef = {
    .name = "song_filter",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/song_filter_pop_up.txt",
        .init = songFilterInit,
        .actionList = {
            .actions = songFilterActions,
            .actionCount = ARRAY_LEN(songFilterActions)
        }
    }
};

}