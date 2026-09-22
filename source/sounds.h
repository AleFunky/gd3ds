#pragma once
#include "wav_player.h"
#include "mp3_player.h"

typedef struct {
    char *name;
    SFX *sound;
} SFXKVP;

extern SFX play_sound;
extern SFX quit_sound;
extern SFX explode_sound;
extern SFX end_sound;
extern SFX honk;
extern SFX coin_sound;

extern SFXKVP sounds[];

void load_sfx();
SFX *get_sound(const char *name);