#pragma once
#include "wav_player.hpp"
#include "mp3_player.hpp"

namespace gd3ds
{

typedef struct {
    const char *name;
    SFX *sound;
} SFXKVP;

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
