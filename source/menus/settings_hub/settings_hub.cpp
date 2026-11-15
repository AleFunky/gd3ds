#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"

#include "game/audio_manager.hpp"

namespace gd3ds
{

static UISlider *musicSliderBar;
static UISlider *soundSliderBar;

static void settingsHubInit(UIScreen *s) {
    musicSliderBar = (UISlider*) uiGetElementByTag(s, "music_slider");
    soundSliderBar = (UISlider*) uiGetElementByTag(s, "sound_slider");

    if (musicSliderBar) musicSliderBar->value = AudioManager::get()->musicVolume;
    if (soundSliderBar) soundSliderBar->value = AudioManager::get()->soundVolume;
}

static void settingsHubUpdate(UIScreen *s, UIInput *touch) {
    if (musicSliderBar) AudioManager::get()->musicVolume = musicSliderBar->value;
    if (soundSliderBar) AudioManager::get()->soundVolume = soundSliderBar->value;
    AudioManager::get()->applyVolumeSettings();
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