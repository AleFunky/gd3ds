#include <3ds.h>
#include "audio_manager.hpp"
#include "constants.hpp"
#include <3ds/ndsp/channel.h>
#include <3ds/services/hid.h>
#include <cmath>
#include <memory>

namespace gd3ds
{
std::unique_ptr<AudioManager> AudioManager::s_instance = nullptr;

float AudioManager::getVolumeSlider() {
    u8 slider = 63;
    HIDUSER_GetSoundVolume(&slider);

    return slider / 63.f;
}

float AudioManager::sliderToVolume(float v) {
    return expm1f(v * log1pf(constants::MaxAudioDB)) / constants::MaxAudioDB;
}

void AudioManager::setChannelVolume(int channel, float volume) {
    float mix[12] = {0};

    float gain = sliderToVolume(volume);

    mix[0] = gain;
    mix[1] = gain;

    ndspChnSetMix(channel, mix);
}

void AudioManager::applyVolumeSettings() {
    setChannelVolume(0, musicVolume);

    for (int i = 1; i <= 7; i++) {
        setChannelVolume(i, soundVolume);
    }
}

}