#include <3ds.h>
#include "mp3_player.h"
#include "math_helpers.h"
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include <unistd.h>

#define THREAD_AFFINITY -1           // Execute thread on any core
#define THREAD_STACK_SZ 32 * 1024    // 32kB stack for audio thread

#define MUSIC_CHANNEL 0
#define MP3_BUF_SIZE 4096
#define NUM_BUFS 4

static u32 *audioBuffer;
static LightEvent soundEvent;
static LightEvent seekEvent;
static LightLock decoderLock;

static mpg123_handle* mh;
static size_t buffSize;
static int32_t rate;
static int audio_channels;

static volatile bool quit = false;
static volatile bool looping = false;
static volatile bool skip = false;
static volatile bool paused = false;

volatile float amplitude = 0;

static volatile float seek_target = 0;

static Thread threadId = NULL;

static ndspWaveBuf waveBuf[NUM_BUFS];

static inline u32 samplerate_mp3(void)
{
    return rate;
}

static inline u32 buffsize_mp3(void)
{
    return buffSize;
}

static inline u32 channels_mp3(void)
{
    return audio_channels;
}

static void audioCallback(void *const nul_) {
    (void)nul_;  // Unused

    if(quit || skip) { // Quit flag
        return;
    }
    
    LightEvent_Signal(&soundEvent);
}


void audio_init() {
    LightEvent_Init(&soundEvent, RESET_ONESHOT);

    audioBuffer = (u32*)linearAlloc(NUM_BUFS * buffsize_mp3() * channels_mp3() * sizeof(int16_t));
    
    ndspChnReset(MUSIC_CHANNEL);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(MUSIC_CHANNEL, samplerate_mp3());
    ndspChnSetFormat(MUSIC_CHANNEL, channels_mp3() == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
    ndspSetCallback(audioCallback, NULL);

    if (paused) ndspChnSetPaused(MUSIC_CHANNEL, paused);

    for (int i = 0; i < NUM_BUFS; i++) {
        memset(&waveBuf[i], 0, sizeof(ndspWaveBuf));
        waveBuf[i].data_vaddr = audioBuffer + i * buffsize_mp3() * channels_mp3();
    }
}

void audio_exit() {
    ndspChnReset(MUSIC_CHANNEL);
    linearFree(audioBuffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
}

bool mp3_init(void *file) {
    int err = 0;
    int encoding = 0;

    if ((mh = mpg123_new(NULL, &err)) == NULL) {
        output_log("Couldn't new it\n");
        return 0;
    }

    mpg123_format_none(mh);

    mpg123_format(mh, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);
    mpg123_format(mh, 44100, MPG123_MONO,   MPG123_ENC_SIGNED_16);

    mpg123_format(mh, 48000, MPG123_STEREO, MPG123_ENC_SIGNED_16);
    mpg123_format(mh, 48000, MPG123_MONO,   MPG123_ENC_SIGNED_16);

    if (mpg123_open(mh, file) != MPG123_OK) {
        output_log("Couldn't open file\n");
        return 0;
    }

    if (mpg123_getformat(mh, &rate, &audio_channels, &encoding) != MPG123_OK) {
        output_log("Couldn't get format\n");
        return 0;
    }

    buffSize = MP3_BUF_SIZE * audio_channels;

    return 1;
}

u32 decode_mp3(void* buffer)
{
    size_t done = 0;
    mpg123_read(mh, (unsigned char *)(buffer), buffSize, &done);
    return done / (sizeof(int16_t));
}

float calculate_amplitude(float power) {
    static float prev = 0.0f;
    static float pulse = 0.0f;
    static float avg_delta = 0.0f;
    static float prev_power = 0.0f;

    float delta = power - prev;
    float abs_delta = fabsf(delta);

    // Low-pass filter the delta
    avg_delta = avg_delta + ((abs_delta - avg_delta) * 0.1f);

    // Detect note change: large RMS increase indicates a new note
    float rms_delta = power - prev_power;
    float abs_rms_delta = fabsf(rms_delta);
    float rms_thresh = avg_delta * POWER_THRESH_MULTIPLIER;

    if (abs_rms_delta > rms_thresh && rms_delta > 0.0f) {
        // Note changed, do pulse
        pulse = AMP_MAX;
    }

    pulse *= AMP_I_DECAY;

    if (pulse > AMP_MAX) pulse = AMP_MAX;
    if (pulse < AMP_MIN) pulse = AMP_MIN;

    prev = power;
    prev_power = power;

    return pulse;
}

float calculate_power(int16_t *samples, size_t frames, int channels) {
    uint64_t sum_squares = 0;
    uint32_t count = 0;

    for (size_t i = 0; i < frames; i++) {
        int16_t mono;

        if (channels == 2) {
            int16_t left  = samples[i*2];
            int16_t right = samples[i*2 + 1];
            mono = (left + right) / 2;
        } else {
            mono = samples[i];
        }

        sum_squares += mono * mono;
        count++;
    }

    if (count == 0) return 0.0f;

    return ((float)sum_squares / count) / (32768.0f * 32768.0f);
}

void audio_thread(void *const file) {
    skip = false;
    bool lastbuf = false;

    if(!mp3_init(file)) return;

    audio_init();

    if (seek_target > 0) {   
        seek_mp3(seek_target);    
        seek_target = -1;
    }

    while (!quit && !skip) {
        for (int i = 0; i < NUM_BUFS; i++) {
            ndspWaveBuf *buf = &waveBuf[i];

            if (buf->status == NDSP_WBUF_DONE || buf->status == NDSP_WBUF_FREE) {
                LightLock_Lock(&decoderLock);
                size_t read = decode_mp3(buf->data_pcm16);
                LightLock_Unlock(&decoderLock);

                if (read > 0) {
                    size_t frames = read / (sizeof(int16_t) * channels_mp3());

                    float rms = calculate_power((int16_t*)buf->data_pcm16,
                                            frames,
                                            channels_mp3());

                    amplitude = calculate_amplitude(rms);
                }

                if (read == 0) {
                    if (looping) {
                        seek_mp3(0);
                    } else {
                        lastbuf = true;
                        continue;
                    }
                }

                buf->nsamples = read / channels_mp3();

                ndspChnWaveBufAdd(MUSIC_CHANNEL, buf);
            }
        }

        if (lastbuf)
            break;

        if (ndspChnIsPaused(MUSIC_CHANNEL)) {
            LightEvent_Wait(&soundEvent);
            continue;
        }

        LightEvent_Wait(&soundEvent);

    }

    while (amplitude > AMP_MIN && !quit) {
        amplitude = calculate_amplitude(0);
        svcSleepThread((long) 1.666666666666e+7);
    }
}

// Play an mp3 file defined by a path
int play_mp3(char *path, bool loop, float seek) {
    // Not found
    if (access(path, F_OK) != 0) {
        return 0;
    }

    LightEvent_Init(&seekEvent, RESET_ONESHOT);
    quit = false;
    looping = loop;

    seek_target = seek;

    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    // ... then subtract 1, as lower number => higher actual priority ...
    priority -= 1;
    // ... finally, clamp it between 0x18 and 0x3F to guarantee that it's valid.
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    threadId = threadCreate(audio_thread, path,
                                          THREAD_STACK_SZ, priority,
                                          THREAD_AFFINITY, true);

    if (seek > 0) LightEvent_Wait(&seekEvent);
    return 1;
}

void seek(u32 location) {
    if (location <= mpg123_length(mh)) {
        mpg123_seek(mh, location, SEEK_SET);
    }
}

// Set position in seconds
void seek_mp3(float time) {
    if (time < 0) time = 0;

    int location = time * samplerate_mp3();
    if (!quit) {
        bool oldstate = ndspChnIsPaused(MUSIC_CHANNEL);
        ndspChnSetPaused(MUSIC_CHANNEL, true); //Pause playback...
        
        LightLock_Lock(&decoderLock);
        // Flush old audio
        ndspChnReset(MUSIC_CHANNEL);
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
        ndspChnSetRate(MUSIC_CHANNEL, samplerate_mp3());
        ndspChnSetFormat(MUSIC_CHANNEL, channels_mp3() == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
        ndspSetCallback(audioCallback, NULL);

        seek(location);

        for (int i = 0; i < NUM_BUFS; i++) {
            memset(&waveBuf[i], 0, sizeof(ndspWaveBuf));
            waveBuf[i].data_vaddr = audioBuffer + i * buffsize_mp3() * channels_mp3();
        }
        LightLock_Unlock(&decoderLock);
        ndspChnSetPaused(MUSIC_CHANNEL, oldstate); //once the seeking is done, playback can continue.

        LightEvent_Signal(&seekEvent);
    }
}

// Pause or unpause playback
void toggle_playback_mp3() {
    paused = !ndspChnIsPaused(MUSIC_CHANNEL);
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void pause_playback_mp3() {
    paused = true;
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void unpause_playback_mp3() {
    paused = false;
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void fade_to_amplitude(float new_amplitude) {
    amplitude = calculate_amplitude(new_amplitude);
}

// Stop playback
void stop_mp3() {
    if (!quit && threadId) {
        quit = true;
        
        LightEvent_Signal(&soundEvent);
        
        threadJoin(threadId, U64_MAX);
        
        threadId = NULL;
        audio_exit();
    }
}