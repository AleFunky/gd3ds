#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "level/main_levels.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_darken.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_label.h"

#include "menus/statistics.h"

#include "save/saving.h"
#include "menus/settings_hub/songs.h"

static UIList *list;

static void add_entry(char *title, char *artist, float list_width, UIScreen *s, int i) {

    UIElement *card = (UIElement *) ui_create_rectangle(s);

    if (card) {
        ui_rectangle_set_color((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
        ui_element_set_size(card, 0, 46);

        // Song name
        UILabel *song = ui_create_label(s);
        if (song) {
            song->base.w = list->base.w - 12;
            ui_label_set_text(song, title);
            ui_element_set_position((UIElement *) song, -list_width + 6, - 5);
            ui_element_set_scale((UIElement *) song, 0.54f);
            
            // song->font = 2;

            ui_element_add_child(card, (UIElement *) song);
        }

        // Song name
        UILabel *creator = ui_create_label(s);
        if (creator) {
            ui_label_set_text(creator, artist);
            ui_element_set_position((UIElement *) creator, -list_width + 6, + 7);
            ui_element_set_scale((UIElement *) creator, 0.54f);
            
            creator->font = 2;

            ui_element_add_child(card, (UIElement *) creator);
        }

        ui_list_add(list, card);
    }
}

void songs_init(UIScreen *s) {
    list = (UIList *) ui_get_element_by_tag(s, "list");
    if (list) {
        float list_width = list->base.w * 0.5f;
        int i = 0;
        for (; i < current_main_level_pack->count; i++) {
            char *title = current_main_level_pack->levels[i].song_data.title;
            char *artist = current_main_level_pack->levels[i].song_data.artist;

            add_entry(title, artist, list_width, s, i);
        }
        
        add_entry("Practice: Stay Inside Me", "OcularNebula", list_width, s, i);
    }
}

const UIScreenDefPair songs_def = {
    .name = "songs",
    .btm = {
        .path = "romfs:/menus/settings_hub/soundtrack.txt",
        .init = songs_init,
    }
};