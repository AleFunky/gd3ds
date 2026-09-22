#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.h"
#include "save/saving.h"
#include "state.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_darken.h"

#include "menus/main_menu.h"
#include "menus/settings_hub/how_to_play.h"
#include "menus/settings_hub/settings.h"
#include "menus/settings_hub/credits.h"
#include "menus/settings_hub/songs.h"

static UISlider *music_slider_bar;
static UISlider *sound_slider_bar;

static void settings_hub_init(UIScreen *s) {
    music_slider_bar = (UISlider*) ui_get_element_by_tag(s, "music_slider");
    sound_slider_bar = (UISlider*) ui_get_element_by_tag(s, "sound_slider");

    if (music_slider_bar) music_slider_bar->value = music_volume;
    if (sound_slider_bar) sound_slider_bar->value = sound_volume;
}

static void settings_hub_update() {
    if (music_slider_bar) music_volume = music_slider_bar->value;
    if (sound_slider_bar) sound_volume = sound_slider_bar->value;
    apply_volume_settings();
}

const UIScreenDefPair settings_hub_def = {
    .name = "settings_hub",
    .btm = {
        .path = "romfs:/menus/settings_hub/settings_hub.txt",
        .init = settings_hub_init,
        .update = settings_hub_update,
    }
};
