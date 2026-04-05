#pragma once

#define AMP_MIN 0.1f
#define AMP_MAX 1.0f

#define AMP_DECAY 0.15f
#define AMP_I_DECAY (1.f - AMP_DECAY)

#define POWER_THRESH_MULTIPLIER 0.75f

extern volatile float amplitude;

void audio_init();
int play_mp3(char *path, bool loop, float seek);
void seek_mp3(float time);
void stop_mp3();
void toggle_playback_mp3();
void pause_playback_mp3();
void unpause_playback_mp3();
void fade_to_amplitude(float new_amplitude);