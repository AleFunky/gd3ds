#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_darken.h"

#include "main.h"
#include "main_menu.h"

#include "save/saving.h"

#include "how_to_play.h"
#include "settings.h"
#include "credits.h"
#include "songs.h"
#include "state.h"

static bool exiting = false;

static bool in_credits = false;
static bool in_settings = false;
bool in_how_to_play = false;
static bool in_songs = false;
static bool switch_to_soundtrack = false;

static UIScreen screen = {
    .isBottom = true,
};

static UISlider *music_slider_bar;
static UISlider *sound_slider_bar;

void exit_settings_hub(UIElement* e) {
    exiting = true;
}


static void open_settings(UIElement *e) {
    in_settings = true;
    settings_init();
}

static void open_credits(UIElement *e) {
    in_credits = true;
    credits_init();
}

static void open_songs(UIElement *e) {
    switch_to_soundtrack = true;
    exiting = true;
}

static void open_how_to_play(UIElement *e) {
    in_how_to_play = true;
    how_to_play_init();
}

static UIAction actions[] = {
    { "exit", exit_settings_hub },
    { "credits", open_credits },
    { "settings", open_settings },
    { "howtoplay", open_how_to_play },
    { "songs", open_songs },
};

void settings_hub_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/settings_hub.txt");

    ui_screen_open(&screen, ANIM_SLIDE_DOWN);

    music_slider_bar = (UISlider*) ui_get_element_by_tag(&screen, "music_slider");
    sound_slider_bar = (UISlider*) ui_get_element_by_tag(&screen, "sound_slider");

    if (music_slider_bar) music_slider_bar->value = music_volume;
    if (sound_slider_bar) sound_slider_bar->value = sound_volume;

    exiting = false;
    in_credits = false;
    in_how_to_play = false;
    in_settings = false;
    in_songs = false;
}

int settings_hub_loop() {

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
        if (switch_to_soundtrack) open_soundtrack();
        switch_to_soundtrack = false;
        return true;
    }

    if (music_slider_bar) music_volume = music_slider_bar->value;
    if (sound_slider_bar) sound_volume = sound_slider_bar->value;

    apply_volume_settings();

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_settings && !in_credits && !in_how_to_play && !in_songs) ui_screen_update(&screen, &touch);

    ui_screen_draw(&screen);

    if (in_songs) {
        int returned = songs_loop();
        if (returned) {
            in_songs = false;
        }
    } 
    if (in_settings) {
        int returned = settings_loop();
        if (returned) {
            in_settings = false;
        }
    } 
    if (in_how_to_play) {
        int returned = how_to_play_loop();
        if (returned) {
            in_how_to_play = false;
        }
    } 
    if (in_credits) {
        int returned = credits_loop();
        if (returned) {
            in_credits = false;
        }
    } 
    return false;
}
