#include "sounds.hpp"
#include "string.h"

SFX playSound;
SFX quitSound;
SFX explodeSound;
SFX endSound;
SFX honk;
SFX coinSound;

SFXKVP sounds[] = {
    { "play", &playSound },
    { "quit", &quitSound },
    { "explode", &explodeSound },
    { "end", &endSound },
    { "boop", &honk },
    { "coin", &coinSound },
};

void loadSfx() {
    loadWav("romfs:/sfx/playSound_01.wav", &playSound);
    loadWav("romfs:/sfx/quitSound_01.wav", &quitSound);
    loadWav("romfs:/sfx/explode_11.wav", &explodeSound);
    loadWav("romfs:/sfx/endStart_02.wav", &endSound);
    loadWav("romfs:/sfx/honk.wav", &honk);
    loadWav("romfs:/sfx/highscoreGet02.wav", &coinSound);
}

SFX *getSound(const char *name){
    for(size_t i = 0; i < sizeof(sounds) / sizeof(sounds[0]); i++){
        if(strcmp(name, sounds[i].name) == 0){
            return sounds[i].sound;
        }
    }
    return &honk;
}