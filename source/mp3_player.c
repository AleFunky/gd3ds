#include <3ds.h>
#include "mp3_player.h"
#include "math_helpers.h"
#include <mpg123.h>
#include <stdio.h>
#include <string.h>

#define THREAD_AFFINITY -1           // Execute thread on any core
#define THREAD_STACK_SZ 32 * 1024    // 32kB stack for audio thread

#define MUSIC_CHANNEL 0
#define MP3_BUF_SIZE 4096
#define NUM_BUFS 2

static u32 *audioBuffer;
static LightEvent soundEvent;
static LightLock decoderLock;

static mpg123_handle* mh;
static size_t buffSize;
static int32_t rate;
static int audio_channels;

static volatile bool quit = false;
static volatile bool looping = false;
static volatile bool skip = false;
static volatile bool paused = false;

static Thread threadId = NULL;

static ndspWaveBuf waveBuf[NUM_BUFS];

u32 samplerate_mp3(void)
{
	return rate;
}

u32 buffsize_mp3(void)
{
	return buffSize;
}

u32 channels_mp3(void)
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

    if((mh = mpg123_new(NULL, &err)) == NULL) {
        printf("Couldn't new it\n");
        return 0;
    }

    if(mpg123_open(mh, file) != MPG123_OK || mpg123_getformat(mh, &rate, &audio_channels, &encoding) != MPG123_OK) {
        printf("Couldn't open or get format\n");
        return 0;
    }

	buffSize = MP3_BUF_SIZE * channels_mp3();

    return 1;
}

u32 decode_mp3(void* buffer)
{
	size_t done = 0;
	mpg123_read(mh, (unsigned char *)(buffer), buffSize, &done);
	return done / (sizeof(int16_t));
}

void audio_thread(void *const file) {
    skip = false;
    bool lastbuf = false;

    if(!mp3_init(file)) return;

    audio_init();

    while (!quit && !skip) {
        for (int i = 0; i < NUM_BUFS; i++) {
            ndspWaveBuf *buf = &waveBuf[i];

            if (buf->status == NDSP_WBUF_DONE || buf->status == NDSP_WBUF_FREE) {
                LightLock_Lock(&decoderLock);
                size_t read = decode_mp3(buf->data_pcm16);
                LightLock_Unlock(&decoderLock);

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
            
            DSP_FlushDataCache(
                buf->data_pcm16,
                buf->nsamples * channels_mp3() * sizeof(int16_t)
            );
        }

        if (lastbuf)
            break;

        if (ndspChnIsPaused(MUSIC_CHANNEL)) {
            LightEvent_Wait(&soundEvent);
            continue;
        }

        LightEvent_Wait(&soundEvent);
    }
}

// Play an mp3 file defined by a path
int play_mp3(char *path, bool loop) {
    quit = false;
    looping = loop;

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
    return 0;
}

void seek(u32 location) {
    LightLock_Lock(&decoderLock);

    if (location <= mpg123_length(mh)) {
        mpg123_seek(mh, location, SEEK_SET);
    }

    LightLock_Unlock(&decoderLock);
}

// Set position in seconds
void seek_mp3(float time) {
    if (time < 0) time = 0;

    int location = time * samplerate_mp3();
    if (!quit) {
		bool oldstate = ndspChnIsPaused(MUSIC_CHANNEL);
		ndspChnSetPaused(MUSIC_CHANNEL, true); //Pause playback...
		seek(location);
		ndspChnSetPaused(MUSIC_CHANNEL, oldstate); //once the seeking is done, playback can continue.
	}
}

// Pause or unpause playback
void toggle_playback_mp3() {
    paused = !ndspChnIsPaused(MUSIC_CHANNEL);
	ndspChnSetPaused(MUSIC_CHANNEL, paused);
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