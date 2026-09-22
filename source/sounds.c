#include "sounds.h"
#include "string.h"

SFX play_sound;
SFX quit_sound;
SFX explode_sound;
SFX end_sound;
SFX honk;
SFX coin_sound;

SFXKVP sounds[] = {
    { "play", &play_sound },
    { "quit", &quit_sound },
    { "explode", &explode_sound },
    { "end", &end_sound },
    { "boop", &honk },
    { "coin", &coin_sound },
};

void load_sfx() {
    load_wav("romfs:/sfx/playSound_01.wav", &play_sound);
    load_wav("romfs:/sfx/quitSound_01.wav", &quit_sound);
    load_wav("romfs:/sfx/explode_11.wav", &explode_sound);
    load_wav("romfs:/sfx/endStart_02.wav", &end_sound);
    load_wav("romfs:/sfx/honk.wav", &honk);
    load_wav("romfs:/sfx/highscoreGet02.wav", &coin_sound);
}

SFX *get_sound(const char *name){
    for(size_t i = 0; i < sizeof(sounds) / sizeof(sounds[0]); i++){
        if(strcmp(name, sounds[i].name) == 0){
            return sounds[i].sound;
        }
    }
    return &honk;
}