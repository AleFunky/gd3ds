#pragma once
#include <3ds.h>

namespace gd3ds
{

struct SFX {
    int16_t* data;
    u32 sampleCount;
    int sampleRate;
    int channels;
};

bool loadWav(const char* path, SFX* sfx);
void playSfx(SFX* sfx, int channel);

}
