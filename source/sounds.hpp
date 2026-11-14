#pragma once
#include "wav_player.hpp"
#include "mp3_player.hpp"

namespace gd3ds
{

struct SFXKVP {
    const char *name;
    SFX *sound;
};

extern SFX playSound;
extern SFX quitSound;
extern SFX explodeSound;
extern SFX endSound;
extern SFX honk;
extern SFX coinSound;

extern SFXKVP sounds[];

void loadSfx();
SFX *getSound(const char *name);

}
