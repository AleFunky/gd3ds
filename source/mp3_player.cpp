#include <3ds.h>
#include "mp3_player.hpp"
#include "level_loading.hpp"
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include "main.hpp"
#include <unistd.h>
#include "menus/settings_hub/settings.hpp"
#include "state.hpp"

#define THREAD_AFFINITY (isN3Ds ? 2 : 0)
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
static int audioChannels;

static volatile bool quit = false;
static volatile bool looping = false;
static volatile bool skip = false;
static volatile bool paused = false;
static volatile bool restartRequested = false;

volatile float amplitude = 0;


static bool shouldFreeBuf = true;
static void *song_buf = NULL;
static size_t songSz = 0;
static size_t songPos = 0;

static volatile float seekTarget = 0;

static Thread threadId = NULL;

static ndspWaveBuf waveBuf[NUM_BUFS];

static inline u32 samplerateMP3(void) {
    return rate;
}

static inline u32 buffsizeMP3(void) {
    return buffSize;
}

static inline u32 channelsMP3(void) {
    return audioChannels;
}

static inline void *pcmSlot(int i) {
    return (u8 *) audioBuffer + (size_t) i * buffsizeMP3();
}

static void audioCallback(void *const nul) {
    (void)nul;  // Unused

    if(quit || skip) { // Quit flag
        return;
    }
    
    LightEvent_Signal(&soundEvent);
}


void audioInit() {
    LightEvent_Init(&soundEvent, RESET_ONESHOT);

    audioBuffer = (u32*)linearAlloc(NUM_BUFS * buffsizeMP3());

    ndspChnReset(MUSIC_CHANNEL);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(MUSIC_CHANNEL, samplerateMP3());
    ndspChnSetFormat(MUSIC_CHANNEL, channelsMP3() == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
    ndspSetCallback(audioCallback, NULL);

    if (paused) ndspChnSetPaused(MUSIC_CHANNEL, paused);

    for (int i = 0; i < NUM_BUFS; i++) {
        memset(&waveBuf[i], 0, sizeof(ndspWaveBuf));
        waveBuf[i].data_vaddr = pcmSlot(i);
    }

    applyVolumeSettings();
}

void audioExit() {
    ndspChnWaveBufClear(MUSIC_CHANNEL);
    ndspChnReset(MUSIC_CHANNEL);

    if (audioBuffer) {
        linearFree(audioBuffer);
        audioBuffer = NULL;
    }

    if (mh) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        mh = NULL;
    }
}

static ssize_t readMem(void *handle, void *buf, size_t count) {
    (void)handle;

    if (!song_buf || songPos >= songSz) return 0;

    size_t available = songSz - songPos;
    size_t toRead = count < available ? count : available;

    if (toRead > 0) {
        memcpy(buf, (u8*)song_buf + songPos, toRead);
        songPos += toRead;
    }

    return (ssize_t)toRead;
}

static off_t seekMem(void *handle, off_t offset, int whence) {
    (void)handle;

    off_t newPos;

    switch (whence) {
        case SEEK_SET:
            newPos = offset;
            break;
        case SEEK_CUR:
            newPos = (off_t)songPos + offset;
            break;
        case SEEK_END:
            newPos = (off_t)songSz + offset;
            break;
        default:
            return -1;
    }

    if (newPos < 0) newPos = 0;
    if ((size_t)newPos > songSz) newPos = (off_t)songSz;

    songPos = (size_t)newPos;
    return (off_t)songPos;
}

bool mp3Init(const char *file) {
    int err = 0;
    int encoding = 0;

    if ((mh = mpg123_new(NULL, &err)) == NULL) {
        outputLog("Couldn't new it\n");
        return 0;
    }

    mpg123_format_none(mh);

    mpg123_format(mh, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);
    mpg123_format(mh, 44100, MPG123_MONO,   MPG123_ENC_SIGNED_16);

    mpg123_format(mh, 48000, MPG123_STEREO, MPG123_ENC_SIGNED_16);
    mpg123_format(mh, 48000, MPG123_MONO,   MPG123_ENC_SIGNED_16);

    if (song_buf) {
        songPos = 0;
        mpg123_replace_reader_handle(mh, readMem, seekMem, NULL);

        if (mpg123_open_handle(mh, NULL) != MPG123_OK) {
            outputLog("Couldn't open memory stream\n");
            return 0;
        }
    } else {
        if (mpg123_open(mh, file) != MPG123_OK) {
            outputLog("Couldn't open file\n");
            return 0;
        }
    }

    if (mpg123_getformat(mh, (long *)&rate, &audioChannels, &encoding) != MPG123_OK) {
        outputLog("Couldn't get format\n");
        return 0;
    }

    buffSize = MP3_BUF_SIZE * audioChannels;

    return 1;
}

u32 decodeMP3(void* buffer) {
    size_t done = 0;
    mpg123_read(mh, (unsigned char *)(buffer), buffSize, &done);
    return done / (sizeof(int16_t));
}

float calculateAmplitude(float power) {
    if (state.practiceMode && !settingsState.practiceMusicSync) return 0.f;
    if (gameState == STATE_MENU) return 0.5f;

    static float prev = 0.0f;
    static float pulse = 0.0f;
    static float avgDelta = 0.0f;
    static float prevPower = 0.0f;

    float delta = power - prev;
    float absDelta = fabsf(delta);

    // Low-pass filter the delta
    avgDelta = avgDelta + ((absDelta - avgDelta) * 0.1f);

    // Detect note change, large RMS increase indicates a new note
    float rmsDelta = power - prevPower;
    float absRmsDelta = fabsf(rmsDelta);
    float rms_thresh = avgDelta * POWER_THRESH_MULTIPLIER;

    if (absRmsDelta > rms_thresh && rmsDelta > 0.f) {
        // Note changed, do pulse
        pulse = AMP_MAX;
    }

    pulse *= AMP_I_DECAY;

    if (pulse > AMP_MAX) pulse = AMP_MAX;
    if (pulse < AMP_MIN) pulse = AMP_MIN;

    prev = power;
    prevPower = power;

    return pulse;
}

float calculatePower(int16_t *samples, size_t frames, int channels) {
    uint64_t sumSquares = 0;
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

        sumSquares += mono * mono;
        count++;
    }

    if (count == 0) return 0.f;

    return ((float)sumSquares / count) / (32768.f * 32768.f);
}

static float getSongLength() {
    if (samplerateMP3() <= 0.f) return 0.f; // Shouldn't happen but just in case

    return (float) mpg123_length(mh) / samplerateMP3();
}

void audioThread(void *const file) {
    skip = false;
    bool lastbuf = false;

    if(!mp3Init((const char *)file)) return;

    audioInit();

    // If song offset greater than the length, start at 0
    if (levelInfo.songOffset >= getSongLength()) {
        levelInfo.songOffset = 0;
    }
    
    // Do the same with seek target
    if (seekTarget >= getSongLength()) {
        seekTarget = 0;
        LightEvent_Signal(&seekEvent);
    }

    // Do initial seek
    if (seekTarget > 0) {   
        seekMP3(seekTarget);    
        seekTarget = -1;
    }

    while (!quit && !skip) {
        for (int i = 0; i < NUM_BUFS; i++) {
            ndspWaveBuf *buf = &waveBuf[i];

            if (buf->status == NDSP_WBUF_DONE || buf->status == NDSP_WBUF_FREE) {
                LightLock_Lock(&decoderLock);
                size_t read = decodeMP3(buf->data_pcm16);
                LightLock_Unlock(&decoderLock);

                if (read > 0) {
                    size_t frames = read / (sizeof(int16_t) * channelsMP3());

                    float rms = calculatePower((int16_t*)buf->data_pcm16,
                                            frames,
                                            channelsMP3());

                    amplitude = calculateAmplitude(rms);
                }

                // Check for the end of song
                if (read == 0) {
                    if (looping) {
                        seekMP3(0);
                    } else {
                        lastbuf = true;
                        continue;
                    }
                }

                buf->nsamples = read / channelsMP3();

                ndspChnWaveBufAdd(MUSIC_CHANNEL, buf);
            }
        }

        // Handle song end
        if (lastbuf) {
            // Wait for either song restart or level quit, while waiting, make pulses scale to 0
            while (!restartRequested && !quit) {
                amplitude = calculateAmplitude(0);
                svcSleepThread((long) 1.666666666666e+7);
            }

            if (quit) break;

            restartRequested = false;
            lastbuf = false;
            continue;
        }

        if (ndspChnIsPaused(MUSIC_CHANNEL)) {
            LightEvent_Wait(&soundEvent);
            continue;
        }

        LightEvent_Wait(&soundEvent);
    }
}

// Play an mp3 file defined by a path
int playMP3(const char *path, bool loop, float seek) {
    // Not found
    if (access(path, F_OK) != 0) {
        return 0;
    }

    stopMP3();

    LightEvent_Init(&seekEvent, RESET_ONESHOT);
    quit = false;
    paused = false;
    looping = loop;

    shouldFreeBuf = true;

    seekTarget = seek;

    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority -= 10;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    threadId = threadCreate(audioThread, (void *)path,
                                          THREAD_STACK_SZ, priority,
                                          THREAD_AFFINITY, true);

    // Wait for seek to finish
    if (seek > 0) LightEvent_Wait(&seekEvent);
    return 1;
}

void seek(u32 location) {
    mpg123_seek(mh, location, SEEK_SET);
}

// Set position in seconds
void seekMP3(float time) {
    if (time < 0) time = 0;

    int location = time * samplerateMP3();
    if (!quit && audioBuffer) {
        bool oldstate = ndspChnIsPaused(MUSIC_CHANNEL);
        ndspChnSetPaused(MUSIC_CHANNEL, true); //Pause playback...
        
        LightLock_Lock(&decoderLock);
        // Flush old audio
        ndspChnReset(MUSIC_CHANNEL);
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
        ndspChnSetRate(MUSIC_CHANNEL, samplerateMP3());
        ndspChnSetFormat(MUSIC_CHANNEL, channelsMP3() == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
        ndspSetCallback(audioCallback, NULL);

        seek(location);

        for (int i = 0; i < NUM_BUFS; i++) {
            memset(&waveBuf[i], 0, sizeof(ndspWaveBuf));
            waveBuf[i].data_vaddr = pcmSlot(i);
        }
        LightLock_Unlock(&decoderLock);
        ndspChnSetPaused(MUSIC_CHANNEL, oldstate); //once the seeking is done, playback can continue.

        applyVolumeSettings();
        
        restartRequested = true;
        LightEvent_Signal(&seekEvent);
    }
}

// Pause or unpause playback
void togglePlaybackMP3() {
    paused = !ndspChnIsPaused(MUSIC_CHANNEL);
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void pausePlaybackMP3() {
    paused = true;
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void unpausePlaybackMP3() {
    paused = false;
    ndspChnSetPaused(MUSIC_CHANNEL, paused);
}

void fadeToAmplitude(float newAmplitude) {
    amplitude = calculateAmplitude(newAmplitude);
}

// Play an mp3 from a pre-loaded memory buffer
int playMP3Buf(void *buf, size_t sz, bool loop, float seek, bool freeBuf) {
    if (!buf || sz == 0) return 0;

    stopMP3();

    LightEvent_Init(&seekEvent, RESET_ONESHOT);
    quit = false;
    looping = loop;
    seekTarget = seek;

    shouldFreeBuf = freeBuf;

    song_buf = buf;
    songSz = sz;
    songPos = 0;

    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority -= 10;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    threadId = threadCreate(audioThread, NULL, THREAD_STACK_SZ, priority, THREAD_AFFINITY, true);
    if (!threadId) {
        if (shouldFreeBuf) free(song_buf);
        song_buf = NULL;
        songSz = 0;
        songPos = 0;
        return 0;
    }

    if (seek > 0) LightEvent_Wait(&seekEvent);
    return 1;
}

// Stop playback
void stopMP3() {
    if (threadId) {
        quit = true;
        
        LightEvent_Signal(&soundEvent);
        
        threadJoin(threadId, U64_MAX);
        
        threadId = NULL;
        audioExit();
    }

    if (shouldFreeBuf) {
        free(song_buf);
    }
    song_buf = NULL;
    songSz = 0;
    songPos = 0;
}