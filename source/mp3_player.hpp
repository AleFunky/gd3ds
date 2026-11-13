#pragma once

#include <stddef.h>

#define AMP_MIN 0.1f
#define AMP_MAX 1.0f

#define AMP_DECAY 0.15f
#define AMP_I_DECAY (1.f - AMP_DECAY)

#define POWER_THRESH_MULTIPLIER 0.75f

extern volatile float amplitude;

void audioInit();
int playMP3(const char *path, bool loop, float seek);
int playMP3Buf(void *buf, size_t sz, bool loop, float seek, bool freeBuf);
void seekMP3(float time);
void stopMP3();
void togglePlaybackMP3();
void pausePlaybackMP3();
void unpausePlaybackMP3();
void fadeToAmplitude(float newAmplitude);