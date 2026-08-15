#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "statistics.h"
#include "menus/components/ui_darken.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_label.h"

#include "save/saving.h"

static bool yes_exit = false;
static bool exiting = false;

static UIScreen screen = {
    .isBottom = true
};

static UIList *list;

typedef struct SongEntries {
    char *title;
    char *artist;
} SongEntries;

static const SongEntries songs[] = {
    { "Stereo Madness", "ForeverBound" },
    { "Back on Track", "DJVI" },
    { "Polargeist", "Step" }, 
    { "Dry Out", "DJVI"},
    { "Base after Base", "DJVI" },
    { "Can't let Go", "DJVI" },
    { "Jumper", "Waterflame" },
    { "Time Machine", "Waterflame" },
    { "Cycles", "DJVI" },
    { "xStep", "DJVI" },
    { "Clutterfunk", "Waterflame" },
    { "Theory of Everything", "DJ-Nate" },
    { "Electroman Adventures", "Waterflame" },
    { "Clubstep", "DJ-Nate" },
    { "Electrodynamix", "DJ-Nate" },
    { "Hexagon Force", "Waterflame" },
    { "Blast Processing", "Waterflame" },
    { "Theory of Everything 2", "DJ-Nate" },
};

void exit_songs(UIElement* e) {
    //start exit animation
    exiting = true;
}

static UIAction actions[] = {
    { "exit", exit_songs }
};

void songs_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/soundtrack.txt");
    ui_screen_open(&screen, ANIM_SLIDE_DOWN);

    list = (UIList *) ui_get_element_by_tag(&screen, "list");

    if (list) {
        float list_width = list->base.w * 0.5f;

        for (int i = 0; i < ARRAY_LEN(songs); i++) {
            char *title = songs[i].title;
            char *artist = songs[i].artist;

            UIElement *card = (UIElement *) ui_create_rectangle(&screen.ctx);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
                ui_element_set_size(card, 0, 46);

                // Song name
                UILabel *song = ui_create_label(&screen.ctx);
                if (song) {
                    song->base.w = list->base.w - 12;
                    ui_label_set_text(song, title);
                    ui_element_set_position((UIElement *) song, -list_width + 6, - 5);
                    ui_element_set_scale((UIElement *) song, 0.54f);
                    
                    // song->font = 2;

                    ui_element_add_child(card, (UIElement *) song);
                }

                // Song name
                UILabel *creator = ui_create_label(&screen.ctx);
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
    }

    exiting = false;
    yes_exit = false;
}

int songs_loop() {
    if(exiting){
        if (screen.transition.state != UI_TRANSITION_CLOSING) {
            ui_screen_close(&screen);
        }

        if (screen.loaded) {
            UIDarken *darken = (UIDarken *) ui_get_element_by_tag(&screen, "darken");
            darken->base.opacity = ((0.5f - screen.transition.time) * 2.f) * darken->targetOpacity;
            ui_darken_reset_opacity(darken);
        }
    }

    // The screen is unloaded by the call to ui_screen_close
    if (!screen.loaded) {
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;

    ui_screen_update(&screen, &touch);

    ui_screen_draw(&screen);

    return false;
}
