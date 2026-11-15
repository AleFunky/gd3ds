#pragma once

#include <memory>

namespace gd3ds
{

class AudioManager {
public:
    static AudioManager* get() {
        if (!s_instance) {
            s_instance = std::make_unique<AudioManager>();
        }
        return s_instance.get();
    }

    float getVolumeSlider();
    float sliderToVolume(float v);
    void setChannelVolume(int channel, float volume);
    void applyVolumeSettings();

public: // state
    float globalVolume = 0.0f;
    float musicVolume = 0.0f;
    float soundVolume = 0.0f;

private:
    static std::unique_ptr<AudioManager> s_instance;
};

}