#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.hpp"
#include "save/saving.hpp"
#include "state.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/components/ui_darken.hpp"

#include "menus/main_menu.hpp"
#include "menus/settings_hub/how_to_play.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/settings_hub/credits.hpp"
#include "menus/settings_hub/songs.hpp"

namespace gd3ds
{

static UISlider *musicSliderBar;
static UISlider *soundSliderBar;

static void settingsHubInit(UIScreen *s) {
    musicSliderBar = (UISlider*) uiGetElementByTag(s, "music_slider");
    soundSliderBar = (UISlider*) uiGetElementByTag(s, "sound_slider");

    if (musicSliderBar) musicSliderBar->value = musicVolume;
    if (soundSliderBar) soundSliderBar->value = soundVolume;
}

static void settingsHubUpdate(UIScreen *s, UIInput *touch) {
    if (musicSliderBar) musicVolume = musicSliderBar->value;
    if (soundSliderBar) soundVolume = soundSliderBar->value;
    applyVolumeSettings();
}

extern const UIScreenDefPair settingsHubDef = {
    .name = "settings_hub",
    .btm = {
        .path = "romfs:/menus/settings_hub/settings_hub.txt",
        .init = settingsHubInit,
        .update = settingsHubUpdate,
    }
};

}