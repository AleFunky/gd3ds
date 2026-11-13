#pragma once
#include <3ds.h>

typedef struct {
    int16_t* data;
    u32 sampleCount;
    int sampleRate;
    int channels;
} SFX;

bool loadWav(const char* path, SFX* sfx);
void playSfx(SFX* sfx, int channel);