#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wav_player.hpp"
#include "main.hpp"
static ndspWaveBuf waveBufs[24];

bool load_wav(const char* path, SFX* sfx) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    char chunkId[4];
    u32 chunkSize;
    char format[4];

    fread(chunkId, 1, 4, f); // "RIFF"
    fread(&chunkSize, 4, 1, f);
    fread(format, 1, 4, f);  // "WAVE"

    if (strncmp(chunkId, "RIFF", 4) || strncmp(format, "WAVE", 4)) {
        // Not a real wav file
        fclose(f);
        return false;
    }

    // Read chunks
    while (!feof(f)) {
        fread(chunkId, 1, 4, f);
        fread(&chunkSize, 4, 1, f);

        if (!strncmp(chunkId, "fmt ", 4)) {
            u16 audioFormat, numChannels, bitsPerSample;
            u32 sampleRate;

            fread(&audioFormat,   2, 1, f);
            fread(&numChannels,   2, 1, f);
            fread(&sampleRate,    4, 1, f);

            fseek(f, 6, SEEK_CUR); // Skip byteRate + blockAlign

            fread(&bitsPerSample, 2, 1, f);

            if (audioFormat != 1 || bitsPerSample != 16) {
                fclose(f);
                return false;
            }

            sfx->channels = numChannels;
            sfx->sampleRate = sampleRate;

            // Skip rest if needed
            if (chunkSize > 16)
                fseek(f, chunkSize - 16, SEEK_CUR);

        } else if (!strncmp(chunkId, "data", 4)) {
            sfx->data = (int16_t*)linearAlloc(chunkSize);
            fread(sfx->data, 1, chunkSize, f);

            sfx->sampleCount = chunkSize / sizeof(int16_t);
            fclose(f);
            return true;
        } else {
            fseek(f, chunkSize, SEEK_CUR);
        }
    }

    fclose(f);
    return false;
}

void play_sfx(SFX* sfx, int channel) {
    ndspChnReset(channel);

    ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(channel, sfx->sampleRate);
    ndspChnSetFormat(channel,
        sfx->channels == 2 ? NDSP_FORMAT_STEREO_PCM16
                           : NDSP_FORMAT_MONO_PCM16);

    memset(&waveBufs[channel], 0, sizeof(ndspWaveBuf));

    waveBufs[channel].data_vaddr = sfx->data;
    waveBufs[channel].nsamples   = sfx->sampleCount / sfx->channels;

    DSP_FlushDataCache(sfx->data,
        sfx->sampleCount * sizeof(int16_t));

    ndspChnWaveBufAdd(channel, &waveBufs[channel]);
    
    apply_volume_settings();
}