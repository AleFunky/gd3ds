#pragma once
#include <3ds.h>

typedef struct {
    int16_t* data;
    u32 sampleCount;
    int sampleRate;
    int channels;
} SFX;

bool load_wav(const char* path, SFX* sfx);
void play_sfx(SFX* sfx, int channel);