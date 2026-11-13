#include <citro2d.h>
#include <citro3d.h>

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <3ds.h>
#include "3ds/svc.h"
#include <archive.h>
#include "groups.hpp"
#include "level_loading.hpp"
#include "main.hpp"
#include "graphics.hpp"
#include "icon_cache.hpp"
#include "particles/rays.hpp"
#include "sounds.hpp"
#include "triggers.hpp"
#include "menus/core/ui_element.hpp"
#include "mp3_player.hpp"
#include "fonts/bigFont.hpp"

#include "save/config.hpp"

#include <curl/curl.h>
#include "text.hpp"
#include "utils/network.hpp"

#include "state.hpp"

#include "particles/particles.hpp"
#include "particles/object_particles.hpp"
#include "particles/coin_effect.hpp"
#include "particles/collectable_effect.hpp"

#include <stdarg.h>

#include "player/player.hpp"
#include "particles/circles.hpp"
#include "particles/lightning.hpp"
#include "particles/gravity_sweep.hpp"

#include "level/main_levels.hpp"
#include "menus/main_menu.hpp"
#include "menus/level_select.hpp"
#include "menus/gameplay.hpp"
#include "menus/updater.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/loading_screen.hpp"
#include "menus/level_complete.hpp"

#include "save/saving.hpp"

#include "endwall.hpp"
#include "practice.hpp"

#include "new_best.hpp"

#include "math_helpers.hpp"
#include "profiling.hpp"

#include "utils/gfx.hpp"
#include "utils/utils.hpp"
#include "utils/precise_input.hpp"
#include "utils/server_utils.hpp"

#ifdef DEBUG_LEAKS
#include "utils/leaks_dbg.hpp"
#endif

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

namespace gd3ds
{

Arena levelArena;

u32 ctruLinearHeapSize __asm__("__ctru_linear_heap_size") = 34 << 20;

bool inExclusiveState = false;

int gameState = STATE_MENU;
bool escapeState;

bool exitingLevel;

bool playingMenuLoop = false;
char menuLoopPath[32];

int levelResult = 0;

bool cheated = false;

float globalVolume;
float musicVolume;
float soundVolume;
float dimSecondaryScreen;

int steps = 0;
int lastSteps = 0;

bool cheatsUsed[CHEAT_COUNT];

const char *cheatNames[CHEAT_COUNT] = {
    "Noclip",
    "Hitboxes"
};

float accumulator = 0.f;
bool fixedDt = true;

#define TARGET_FPS 60
#define PI_SUBSTEP_BUCKETS (STEPS_HZ / TARGET_FPS)

u32 piSubstepPresses[PI_SUBSTEP_BUCKETS];

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* topRight;
C3D_RenderTarget* bot;

ParticleSystem touchDragParticles;
ParticleSystem touchExplosionParticles;
ParticleSystem glitterParticlesBottom;
ParticleSystem slowSpeedParticlesBottom;
ParticleSystem normalSpeedParticlesBottom;
ParticleSystem fastSpeedParticlesBottom;
ParticleSystem fasterSpeedParticlesBottom;
ParticleSystem fastestSpeedParticlesBottom;
ParticleSystem endWallParticles;
ParticleSystem endWallFirework;
ParticleSystem levelCompleteEffectP1;
ParticleSystem levelCompleteEffectP2;

float slowSpeedParticlesTimer = 0.f;
float normalSpeedParticlesTimer = 0.f;
float fastSpeedParticlesTimer = 0.f;
float fasterSpeedParticlesTimer = 0.f;
float fastestSpeedParticlesTimer = 0.f;

bool altTitleScreen;

bool isN3Ds;
bool is3Dsx;
bool isNightly;

bool queuedRestart;

bool threeDWarningPassed;

char *threeDsxPath;

UIStack menuStack = {};
UIStack gameplayStack = {};

ExternalLevelFile externalFile;
ServerFile gdServerFile;
ServerFile gdpsFile;
ServerFile *currentServerFile;

void loadGdpsInfo() {
    currentServerFile = (gdps ? &gdpsFile : &gdServerFile);
    currentMainLevelPack = (gdps ? &gdpsLevels : &robtopLevels);
}

// Checks if the game is being emulated by citra/azahar
bool isCitra() {
    s64 version = 0;
    svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
    return version != 0;
}

void noDspFirmware(void) {
    consoleInit(GFX_TOP, NULL);
    printf("\x1b[01;00H///////////////////FATAL///ERROR//////////////////");
    printf("\x1b[03;00HNDSP could not be initalized!");
    printf("\x1b[05;00HThis is probably because your dspfirm is missing.");
    printf("\x1b[07;00HFor 3DS users, open the Luma menu, go to");
    printf("\x1b[08;00Hmiscellaneous options and press \"Dump DSP");
    printf("\x1b[09;00Hfirmware\".");
    printf("\x1b[11;00HFor Citra/Azahar users, open your emulator folder");
    printf("\x1b[12;00Hand create a file named \"dspfirm.cdc\" in ");
    printf("\x1b[13;00H/sdmc/3ds/. You only need the file to be there, ");
    printf("\x1b[14;00Hit can be empty for all the emulator cares.");
    printf("\x1b[16;00HFor more information, check the GitHub.");
    printf("\x1b[18;00HPress start to exit.");
    printf("\x1b[30;00H//////////////////////////////////////////////////");
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

        gfxSwapBuffers();
        gfxFlushBuffers();
    }
    
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    exit(22);
}

static aptHookCookie cookie = {0};

static void aptCallback(APT_HookType hook, void *param) {
    (void)param;

    switch (hook) {
        case APTHOOK_ONSUSPEND: // Home button
        case APTHOOK_ONSLEEP: // Lid closed
            pauseGame();
            break;

        default:
            break;
    }
}

static FILE *log_file = NULL;

static void ensureLogFile(void) {
    if (log_file) return;  // already open
    
    log_file = fopen(CONFIG_ROOT "output.txt", "w");
}

static void closeLogFile(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

int outputLog(const char *fmt, ...) {
    ensureLogFile();

    if (!log_file) return 0; // failed to open log

    va_list args1, args2;
    int ret;

    va_start(args1, fmt);
    va_copy(args2, args1);  // duplicate args for two outputs

    // Output to log file
    ensureLogFile();
    if (log_file) {
        ret = vfprintf(log_file, fmt, args1);
        fflush(log_file);
    }

    // Output to console (stdout)
    ret = vprintf(fmt, args2);
    
    va_end(args2);
    va_end(args1);

    return ret;
}

#define MAX_AUDIO_DB 9

float getVolumeSlider() {
    u8 slider = 63;
    HIDUSER_GetSoundVolume(&slider);

    return slider / 63.f;
}

float sliderToVolume(float v) {
    return expm1f(v * log1pf(MAX_AUDIO_DB)) / MAX_AUDIO_DB;
}

void setChannelVolume(int channel, float volume) {
    float mix[12] = {0};

    float gain = sliderToVolume(volume);

    mix[0] = gain;
    mix[1] = gain;

    ndspChnSetMix(channel, mix);
}

static void checkRomType() {
    u64 id;
    APT_GetProgramID(&id);
    is3Dsx = (id != GAME_TITLE_ID);
}

void applyVolumeSettings() {
    setChannelVolume(0, musicVolume);

    for (int i = 1; i <= 7; i++) {
        setChannelVolume(i, soundVolume);
    }
}

void checkSystemModel() {
    u8 model = getModel();
    isN3Ds = model == CFG_MODEL_N2DSXL || model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL || isCitra();
}

float delta = 0;
float frameTimer = 0;
unsigned int levelFrame = 0;
unsigned int frameCounter = 0;
bool songLoaded;

static int restartGame() {
    aptInit();
    Result result = 0;
	u8 param[0x300] = {};
	u8 hmac[0x20] = {};

	if (R_FAILED(result = APT_PrepareToDoApplicationJump(0, GAME_TITLE_ID, MEDIATYPE_SD))) {
		return result;
	}

	if (R_FAILED(result = APT_DoApplicationJump(param, sizeof(param), hmac))) {
		return result;
	}

	return 0;
}

// adapted from Luma3DS - sysmodules/rosalina/source/minisoc.c
void enterExclusiveState() {
    Result res = 0;
    __dmb();

    if (!inExclusiveState) {
        ndmuInit();
        res = NDMU_EnterExclusiveState(NDM_EXCLUSIVE_STATE_INFRASTRUCTURE);
        if (R_SUCCEEDED(res)) res = NDMU_LockState(); // prevents ndm from switching to StreetPass when the lid is closed
        inExclusiveState = R_SUCCEEDED(res);
        __dmb();
    }
}

void exitExclusiveState(bool force) {
    Result res = 0;
    __dmb();

    if (inExclusiveState)
    {
        if (!force) {
            res = NDMU_UnlockState();
            if (R_SUCCEEDED(res)) res = NDMU_LeaveExclusiveState();
        }
        ndmuExit();
        inExclusiveState = R_FAILED(res);
        __dmb();
    }
}


void updatePlayerEffects(float delta) {
    for (int i = 0; i < 2; i++) {
        updateParticleSystem(&dragParticles[i], delta);
        updateParticleSystem(&dragParticles2[i], delta);
        updateParticleSystem(&shipFireParticles[i], delta);
        updateParticleSystem(&shipSecondaryParticles[i], delta);
        updateParticleSystem(&secondaryParticles[i], delta);
        updateParticleSystem(&burstParticles[i], delta);
        updateParticleSystem(&robotFireParticles[i], delta);
        updateParticleSystem(&landParticles[i], delta);
        updateParticleSystem(&explosionParticles[i], delta);
    }
    updateParticleSystem(&brickDestroyParticles, delta);
    updateParticleSystem(&glitterParticles, delta);
    updateParticleSystem(&slowSpeedParticles, delta);
    updateParticleSystem(&normalSpeedParticles, delta);
    updateParticleSystem(&fastSpeedParticles, delta);
    updateParticleSystem(&fasterSpeedParticles, delta);
    updateParticleSystem(&fastestSpeedParticles, delta);
    updateParticleSystem(&coinPickupParticles, delta);
}

void allocateParticles() {
    initParticleSystem(&dragParticles[0], &dragEffect);
    initParticleSystem(&dragParticles[1], &dragEffect);

    initParticleSystem(&dragParticles2[0], &shipDragEffect);
    initParticleSystem(&dragParticles2[1], &shipDragEffect);

    initParticleSystem(&shipFireParticles[0], &dragEffect);
    initParticleSystem(&shipFireParticles[1], &dragEffect);

    initParticleSystem(&secondaryParticles[0], &dragEffect);
    initParticleSystem(&secondaryParticles[1], &dragEffect);
    
    initParticleSystem(&shipSecondaryParticles[0], &dragEffect);
    initParticleSystem(&shipSecondaryParticles[1], &dragEffect);

    initParticleSystem(&burstParticles[0], &burstEffect);
    initParticleSystem(&burstParticles[1], &burstEffect);

    initParticleSystem(&robotFireParticles[0], &burstEffect2);
    initParticleSystem(&robotFireParticles[1], &burstEffect2);

    initParticleSystem(&landParticles[0], &landEffect);
    initParticleSystem(&landParticles[1], &landEffect);
    
    initParticleSystem(&explosionParticles[0], &explodeEffect);
    initParticleSystem(&explosionParticles[1], &explodeEffect);

    initParticleSystem(&brickDestroyParticles, &glassDestroy01);
    initParticleSystem(&glitterParticles, &glitterEffect);
    initParticleSystem(&slowSpeedParticles, &speedEffectSlow);
    initParticleSystem(&normalSpeedParticles, &speedEffectNormal);
    initParticleSystem(&fastSpeedParticles, &speedEffectFast);
    initParticleSystem(&fasterSpeedParticles, &speedEffectVfast);
    initParticleSystem(&fastestSpeedParticles, &speedEffectFastest);
    initParticleSystem(&coinPickupParticles, &coinPickupEffect);

    initParticleSystem(&endWallParticles, &endEffectPortal);
    initParticleSystem(&endWallFirework, &firework);
    initParticleSystem(&levelCompleteEffectP1, &levelComplete01);
    initParticleSystem(&levelCompleteEffectP2, &levelComplete01);
}

void freeParticles() {
    for (int i = 0; i < 2; i++) {
        freeParticleData(&dragParticles[i].data);
        freeParticleData(&dragParticles2[i].data);
        freeParticleData(&shipFireParticles[i].data);
        freeParticleData(&secondaryParticles[i].data);
        freeParticleData(&shipSecondaryParticles[i].data);
        freeParticleData(&burstParticles[i].data);
        freeParticleData(&robotFireParticles[i].data);
        freeParticleData(&landParticles[i].data);
        freeParticleData(&explosionParticles[i].data);
    }

    freeParticleData(&brickDestroyParticles.data);
    freeParticleData(&glitterParticles.data);
    freeParticleData(&slowSpeedParticles.data);
    freeParticleData(&normalSpeedParticles.data);
    freeParticleData(&fastSpeedParticles.data);
    freeParticleData(&fasterSpeedParticles.data);
    freeParticleData(&fastestSpeedParticles.data);
    freeParticleData(&coinPickupParticles.data);

    freeParticleData(&endWallParticles.data);
    freeParticleData(&endWallFirework.data);
    freeParticleData(&levelCompleteEffectP1.data);
    freeParticleData(&levelCompleteEffectP2.data);
}

void initParticles(Color p1Color, Color p2Color) {    
    slowSpeedParticles.stationary = true;
    normalSpeedParticles.stationary = true;
    fastSpeedParticles.stationary = true;
    fasterSpeedParticles.stationary = true;
    fastestSpeedParticles.stationary = true;

    slowSpeedParticles.affectedByMirror = false;
    normalSpeedParticles.affectedByMirror = false;
    fastSpeedParticles.affectedByMirror = false;
    fasterSpeedParticles.affectedByMirror = false;
    fastestSpeedParticles.affectedByMirror = false;

    endWallFirework.affectedByMirror = false;

    levelCompleteEffectP1.stationary = true;
    levelCompleteEffectP2.stationary = true;

    // One shot bursts fly towards you, the trails stay glued to the player
    explosionParticles[0].depth = 1.f;
    explosionParticles[1].depth = 1.f;

    burstParticles[0].depth = 0.5f;
    burstParticles[1].depth = 0.5f;
    
    robotFireParticles[0].depth = 0.5f;
    robotFireParticles[1].depth = 0.5f;

    landParticles[0].depth = 0.35f;
    landParticles[1].depth = 0.35f;

    coinPickupParticles.depth = 0.6f;
    brickDestroyParticles.depth = 0.6f;

    // Ambience sweeps past the player instead of sitting flat behind the level
    glitterParticles.depth = 0.4f;

    slowSpeedParticles.depth = 0.6f;
    normalSpeedParticles.depth = 0.6f;
    fastSpeedParticles.depth = 0.6f;
    fasterSpeedParticles.depth = 0.6f;
    fastestSpeedParticles.depth = 0.6f;

    // Its particles spiral inwards, so they start at you and fall into the portal
    endWallParticles.depth = 0.7f;
    endWallParticles.depthInwards = true;

    endWallFirework.depth = 0.8f;
    levelCompleteEffectP1.depth = 0.8f;
    levelCompleteEffectP2.depth = 0.8f;

    Color p1NotWhite = getWhiteIfBlack(p1Color);
    Color p2NotWhite = getWhiteIfBlack(p2Color);

    dragParticles[0].cfg.startColorRed   = p1NotWhite.r / 255.f;
    dragParticles[0].cfg.startColorGreen = p1NotWhite.g / 255.f;
    dragParticles[0].cfg.startColorBlue  = p1NotWhite.b / 255.f;

    dragParticles[1].cfg.startColorRed   = p2NotWhite.r / 255.f;
    dragParticles[1].cfg.startColorGreen = p2NotWhite.g / 255.f;
    dragParticles[1].cfg.startColorBlue  = p2NotWhite.b / 255.f;

    burstParticles[0].cfg.startColorRed   = p1NotWhite.r / 255.f;
    burstParticles[0].cfg.startColorGreen = p1NotWhite.g / 255.f;
    burstParticles[0].cfg.startColorBlue  = p1NotWhite.b / 255.f;

    burstParticles[0].cfg.finishColorRed   = p1NotWhite.r / 255.f;
    burstParticles[0].cfg.finishColorGreen = p1NotWhite.g / 255.f;
    burstParticles[0].cfg.finishColorBlue  = p1NotWhite.b / 255.f;

    burstParticles[1].cfg.startColorRed   = p2NotWhite.r / 255.f;
    burstParticles[1].cfg.startColorGreen = p2NotWhite.g / 255.f;
    burstParticles[1].cfg.startColorBlue  = p2NotWhite.b / 255.f;
    
    burstParticles[1].cfg.finishColorRed   = p2NotWhite.r / 255.f;
    burstParticles[1].cfg.finishColorGreen = p2NotWhite.g / 255.f;
    burstParticles[1].cfg.finishColorBlue  = p2NotWhite.b / 255.f;

    secondaryParticles[0].cfg.startColorRed   = p2NotWhite.r / 255.f;
    secondaryParticles[0].cfg.startColorGreen = p2NotWhite.g / 255.f;
    secondaryParticles[0].cfg.startColorBlue  = p2NotWhite.b / 255.f;
    secondaryParticles[0].cfg.finishColorAlpha = 1.f;
    secondaryParticles[0].cfg.maxParticles = 15;
    secondaryParticles[0].cfg.speed = 75 - 30;

    secondaryParticles[1].cfg.startColorRed   = p1NotWhite.r / 255.f;
    secondaryParticles[1].cfg.startColorGreen = p1NotWhite.g / 255.f;
    secondaryParticles[1].cfg.startColorBlue  = p1NotWhite.b / 255.f;
    secondaryParticles[1].cfg.finishColorAlpha = 1.f;
    secondaryParticles[1].cfg.maxParticles = 15;
    secondaryParticles[1].cfg.speed = 75 - 30;

    shipSecondaryParticles[0].cfg.startColorRed   = 255.f/ 255.f;
    shipSecondaryParticles[0].cfg.startColorGreen = 70.f / 255.f;
    shipSecondaryParticles[0].cfg.startColorBlue  = 10.f / 255.f;

    shipSecondaryParticles[0].cfg.finishColorRed   = 255.f/ 255.f;
    shipSecondaryParticles[0].cfg.finishColorGreen = 20.f / 255.f;
    shipSecondaryParticles[0].cfg.finishColorBlue  = 0.f / 255.f;

    shipSecondaryParticles[1].cfg = shipSecondaryParticles[0].cfg;

    shipFireParticles[0].cfg.startColorRed   = 255.f / 255.f;
    shipFireParticles[0].cfg.startColorGreen = 170.f / 255.f;
    shipFireParticles[0].cfg.startColorBlue  = 10.f / 255.f;
    
    shipFireParticles[0].cfg.finishColorRed   = 255.f / 255.f;
    shipFireParticles[0].cfg.finishColorGreen = 65.f / 255.f;
    shipFireParticles[0].cfg.finishColorBlue  = 0.f / 255.f;

    shipFireParticles[0].cfg.speed = 75 - 30;
    shipFireParticles[0].cfg.maxRadius = 130;
    shipFireParticles[1].cfg = shipFireParticles[0].cfg;

    landParticles[0].cfg.startColorRed   = p1NotWhite.r / 255.f;
    landParticles[0].cfg.startColorGreen = p1NotWhite.g / 255.f;
    landParticles[0].cfg.startColorBlue  = p1NotWhite.b / 255.f;

    landParticles[1].cfg.startColorRed   = p2NotWhite.r / 255.f;
    landParticles[1].cfg.startColorGreen = p2NotWhite.g / 255.f;
    landParticles[1].cfg.startColorBlue  = p2NotWhite.b / 255.f;

    explosionParticles[0].cfg.startColorRed   = p1NotWhite.r / 255.f;
    explosionParticles[0].cfg.startColorGreen = p1NotWhite.g / 255.f;
    explosionParticles[0].cfg.startColorBlue  = p1NotWhite.b / 255.f;

    explosionParticles[1].cfg.startColorRed   = p2NotWhite.r / 255.f;
    explosionParticles[1].cfg.startColorGreen = p2NotWhite.g / 255.f;
    explosionParticles[1].cfg.startColorBlue  = p2NotWhite.b / 255.f;

    glitterParticles.cfg.startColorRed   = p1NotWhite.r / 255.f;
    glitterParticles.cfg.startColorGreen = p1NotWhite.g / 255.f;
    glitterParticles.cfg.startColorBlue  = p1NotWhite.b / 255.f;
    glitterParticles.cfg.startColorAlpha = 1.f;
    
    glitterParticles.cfg.finishColorRed   = p1NotWhite.r / 255.f;
    glitterParticles.cfg.finishColorGreen = p1NotWhite.g / 255.f;
    glitterParticles.cfg.finishColorBlue  = p1NotWhite.b / 255.f;

    slowSpeedParticles.cfg.startColorRed   = 255 / 255.f;
    slowSpeedParticles.cfg.startColorGreen = 255 / 255.f;
    slowSpeedParticles.cfg.startColorBlue  = 0 / 255.f;

    normalSpeedParticles.cfg.startColorRed   = 0 / 255.f;
    normalSpeedParticles.cfg.startColorGreen = 190 / 255.f;
    normalSpeedParticles.cfg.startColorBlue  = 255 / 255.f;

    fastSpeedParticles.cfg.startColorRed   = 0 / 255.f;
    fastSpeedParticles.cfg.startColorGreen = 255 / 255.f;
    fastSpeedParticles.cfg.startColorBlue  = 0 / 255.f;

    fasterSpeedParticles.cfg.startColorRed   = 230 / 255.f;
    fasterSpeedParticles.cfg.startColorGreen = 65 / 255.f;
    fasterSpeedParticles.cfg.startColorBlue  = 255 / 255.f;

    fastestSpeedParticles.cfg.startColorRed   = 255 / 255.f;
    fastestSpeedParticles.cfg.startColorGreen = 65 / 255.f;
    fastestSpeedParticles.cfg.startColorBlue  = 65 / 255.f;

    coinPickupParticles.cfg.startColorRed   = 255 / 255.f;
    coinPickupParticles.cfg.startColorGreen = state.customLevel ? 1.f : 190 / 255.f;
    coinPickupParticles.cfg.startColorBlue  = state.customLevel ? 1.f : 0.f;
    
    endWallParticles.cfg.startColorRed   = p1NotWhite.r / 255.f;
    endWallParticles.cfg.startColorGreen = p1NotWhite.g / 255.f;
    endWallParticles.cfg.startColorBlue  = p1NotWhite.b / 255.f;
    
    endWallParticles.cfg.finishColorRed   = p1NotWhite.r / 255.f;
    endWallParticles.cfg.finishColorGreen = p1NotWhite.g / 255.f;
    endWallParticles.cfg.finishColorBlue  = p1NotWhite.b / 255.f;

    endWallParticles.scale = 1.5f;

    endWallFirework.cfg.startColorRed   = p1NotWhite.r / 255.f;
    endWallFirework.cfg.startColorGreen = p1NotWhite.g / 255.f;
    endWallFirework.cfg.startColorBlue  = p1NotWhite.b / 255.f;
    
    endWallFirework.cfg.finishColorRed   = p1NotWhite.r / 255.f;
    endWallFirework.cfg.finishColorGreen = p1NotWhite.g / 255.f;
    endWallFirework.cfg.finishColorBlue  = p1NotWhite.b / 255.f;

    
    levelCompleteEffectP1.cfg.startColorRed   = p1NotWhite.r / 255.f;
    levelCompleteEffectP1.cfg.startColorGreen = p1NotWhite.g / 255.f;
    levelCompleteEffectP1.cfg.startColorBlue  = p1NotWhite.b / 255.f;

    levelCompleteEffectP1.cfg.finishColorRed   = p1NotWhite.r / 255.f;
    levelCompleteEffectP1.cfg.finishColorGreen = p1NotWhite.g / 255.f;
    levelCompleteEffectP1.cfg.finishColorBlue  = p1NotWhite.b / 255.f;
    
    levelCompleteEffectP2.cfg.startColorRed   = p2NotWhite.r / 255.f;
    levelCompleteEffectP2.cfg.startColorGreen = p2NotWhite.g / 255.f;
    levelCompleteEffectP2.cfg.startColorBlue  = p2NotWhite.b / 255.f;

    levelCompleteEffectP2.cfg.finishColorRed   = p2NotWhite.r / 255.f;
    levelCompleteEffectP2.cfg.finishColorGreen = p2NotWhite.g / 255.f;
    levelCompleteEffectP2.cfg.finishColorBlue  = p2NotWhite.b / 255.f;
}

static bool touchJumpFilter(u16 px, u16 py) {
    bool onPauseButton = px > 300-32/2 && py < 20+32/2;
    bool onPracticeUI = state.practiceMode && !settingsState.hidePracticeButtons && (px > 92 && px < 222 && py > 175 && py < 222);
    return !(onPauseButton || onPracticeUI);
}

static bool touchLeftHalf(u16 px) {
    return px < SCREEN_BOT_WIDTH / 2;
}

static bool touchJumpFilterP1(u16 px, u16 py) {
    return touchJumpFilter(px, py) && touchLeftHalf(px);
}

static bool touchJumpFilterP2(u16 px, u16 py) {
    return touchJumpFilter(px, py) && !touchLeftHalf(px);
}

static bool touchJumpFilterNone(UNUSED u16 px, UNUSED u16 py) {
    return false;
}

u32 jumpKeyMaskP1(void) {
    return (settingsState.yJump ? KEY_Y : KEY_A);
}

u32 jumpKeyMaskP2(void) {
    return KEY_UP;
}

u32 jumpKeyMask(void) {
    return jumpKeyMaskP1() | jumpKeyMaskP2();
}

static void handleGameplayInput(touchPosition touchPos, u32 kDown, u32 kHeld) {
    bool inBounds = touchJumpFilter(touchPos.px, touchPos.py);
    bool leftSide = touchLeftHalf(touchPos.px);

    bool touchPressed = inBounds && (kDown & KEY_TOUCH);
    bool touchHeld = inBounds && (kHeld & KEY_TOUCH);

    // CBF not enabled
    if (!piEnabled) {
        if (levelInfo.twoPlayerMode) {
            bool buttonPressed = (kDown & jumpKeyMaskP1()) != 0;
            bool buttonHeld = (kHeld & jumpKeyMaskP1()) != 0;

            bool buttonPressedP2 = (kDown & jumpKeyMaskP2()) != 0;
            bool buttonHeldP2 = (kHeld & jumpKeyMaskP2()) != 0;

            state.oldInput = state.input;
            
            state.input.pressedJump = (buttonPressed || (touchPressed && leftSide)) == true;
            state.input.holdJump = (state.input.pressedJump || buttonHeld || (touchHeld && leftSide)) == true;

            state.oldInputP2 = state.inputP2;

            state.inputP2.pressedJump = (buttonPressedP2 || (touchPressed && !leftSide)) == true;
            state.inputP2.holdJump = (state.inputP2.pressedJump || buttonHeldP2 || (touchHeld && !leftSide)) == true;
        } else {
            bool buttonPressed = (kDown & jumpKeyMask()) != 0;
            bool buttonHeld = (kHeld & jumpKeyMask()) != 0;
            
            state.oldInput = state.input;
            
            state.input.pressedJump = (buttonPressed || touchPressed) == true;
            state.input.holdJump = (state.input.pressedJump || buttonHeld || touchHeld) == true;
            
            state.oldInputP2 = state.input;

            state.inputP2.pressedJump = state.input.pressedJump;
            state.inputP2.holdJump = state.input.holdJump;
        }
    }
}

void syncPreciseInput(bool suppressHeld) {
    if (levelInfo.twoPlayerMode) {
        piSetJumpKeys(PI_PLAYER_1, jumpKeyMaskP1());
        piSetTouchFilter(PI_PLAYER_1, touchJumpFilterP1);
        piSetJumpKeys(PI_PLAYER_2, jumpKeyMaskP2());
        piSetTouchFilter(PI_PLAYER_2, touchJumpFilterP2);
    } else {
        piSetJumpKeys(PI_PLAYER_1, jumpKeyMask());
        piSetTouchFilter(PI_PLAYER_1, touchJumpFilter);
        piSetJumpKeys(PI_PLAYER_2, 0);
        piSetTouchFilter(PI_PLAYER_2, touchJumpFilterNone);
    }
    piReset();
    if (suppressHeld) {
        piSuppressUntilRelease();
    }
}

void uiLoop(){
    playingMenuLoop = false;
    if (settingsState.menuMusic) playMenuSong();
    else stopMP3();
    
    u64 lastTime = svcGetSystemTick();

    uiStackSetStack(&menuStack);

    while (aptMainLoop()) {
        processSaving();

        u64 now = svcGetSystemTick();
        if (fixedDt) {
            fixedDt = false;
            delta = 1/60.f;
        } else {
            delta = (now - lastTime) / (CPU_TICKS_PER_MSEC * 1000);
        }
        triggerDt = delta;
        lastTime = now;
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPos = touchPos;
        touch.interacted = false;
        touch.down = hidKeysDown();
        touch.held = hidKeysHeld();
        touch.up = hidKeysUp();
        hidCircleRead(&touch.cpad);

        uiStackUpdate(&touch);

        // autoupdater pop up stuff
        if (updaterTask.finished && !checkedForUpdate && uiStackGetFadeStatus() == FADE_STATUS_NONE && strcmp("updater", uiStackGetCurrentScreenName()) != 0 && strcmp("3d_warning_pop_up", uiStackGetCurrentScreenName()) != 0 && strcmp("info_card", uiStackGetCurrentScreenName()) != 0 && strcmp("updater_settings", uiStackGetCurrentScreenName()) != 0)
        {
            // Handle result
            if (updaterTask.result == 0)
            {
                if (updateData->isAvailable)
                {
                    checkedForUpdate = true;
                    TwoOptionPopupData *autoupdateData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
                    if (!autoupdateData)
                        return;

                    autoupdateData->text = strdup("Updates were found for the game.\nOpen the updater?");
                    autoupdateData->title = strdup("Updates Found");
                    autoupdateData->proceedText = strdup("Open");
                    autoupdateData->cancelText = strdup("Later");
                    uiStackPush(&autoupdatePopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
                    uiStackPushData(autoupdateData);
                }
                else
                    outputLog("no updates found");
            }
            updaterTask.finished = false;
        }

        // 3d first time use warning pop up stuff

        if (stereoSupported() && settingsState.stereoEnabled && uiStackGetFadeStatus() == FADE_STATUS_NONE && !threeDWarningPassed && getStereoSlider() > 0.f && strcmp("settings", uiStackGetCurrentScreenName()) != 0)
        {
            TwoOptionPopupData *threeDWarningData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
            if (!threeDWarningData)
                return;

            threeDWarningData->text = strdup("Stereoscopic 3D can significantly lower performance.\nAre you sure you want to keep it enabled?");
            threeDWarningData->title = strdup("Disclaimer");
            threeDWarningData->proceedText = strdup("Enable");
            threeDWarningData->cancelText = strdup("Cancel");
            uiStackPush(&threeDWarningPopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
            uiStackPushData(threeDWarningData);
            threeDWarningPassed = true;
        }

        // Frees a render target, so keep it out of the frame below
        updateStereoTarget();

        updateTouchEffect(DT);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        iconCacheBeginFrame();

        // Bottom screen
        C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
        C2D_SceneBegin(bot);

        uiStackDraw(SCREEN_BTM);

        if(isSaving()) {
            drawText(&bigFontFontCharset, &bigFontSheet, 0, 234, 0.5f, 0.5f, 0, false, "Saving...");
        }

        drawStackFade();

        changeBlending(true);
        drawTouchEffect();
        changeBlending(false);

        // Top screen, drawn once per eye when 3D is on
        for (int eye = 0; beginTopEye(eye); eye++) {
            beginEyeLayer(DEPTH_UI);
            uiStackDraw(SCREEN_TOP);
            drawStackFade();
            //draw_stack_debug();
            endEyeLayer();
        }
        C2D_ViewReset();
        C3D_FrameEnd(0);

        if(escapeState){
            escapeState = false;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}

#define SWAP_CENTER_SHIFT (SWAP_SIDE_OFFSET_X / SCALE)

typedef struct {
    UIElement *element;
    bool enabled;
} SwapElement;

static SwapElement swapHideTag(UIScreen *screen, const char *tag) {
    SwapElement state = { screen ? uiGetElementByTag(screen, tag) : NULL, false };

    if (state.element) {
        state.enabled = state.element->enabled;
        state.element->enabled = false;
    }

    return state;
}

static void swapRestoreElement(SwapElement state) {
    if (state.element) state.element->enabled = state.enabled;
}

static void swapDrawIdentity(UIElement *e) {
    UITransform root = { 0.f, 0.f, 1.f, 1.f };

    uiDrawTree(e, &root);
}

static void swapDrawHud(UIScreen *screen) {
    if (!screen) return;

    SwapElement pauseMenu = swapHideTag(screen, "pause_menu");
    SwapElement fps = swapHideTag(screen, "fps");

    C2D_ViewTranslate(-SWAP_SIDE_OFFSET_X, 0);
    uiScreenDraw(screen);
    C2D_ViewTranslate(SWAP_SIDE_OFFSET_X, 0);

    swapRestoreElement(fps);

    if (fps.element && fps.element->enabled) swapDrawIdentity(fps.element);

    swapRestoreElement(pauseMenu);
}

static void swapDrawPauseWindow(UIScreen *screen) {
    SwapElement bar = swapHideTag(screen, "progressalert");
    SwapElement percent = swapHideTag(screen, "percent");
    SwapElement fps = swapHideTag(screen, "fps");

    uiStackDraw(SCREEN_TOP);

    swapRestoreElement(fps);
    swapRestoreElement(percent);
    swapRestoreElement(bar);
}

void gameLoop() {
#ifdef DEBUG_LEAKS
    current_generation++;
    dump_leaks();
#endif

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    iconCacheBeginFrame();

    C2D_Fade(0);
    for (int eye = 0; beginTopEye(eye); eye++) {
        beginEyeLayer(DEPTH_UI);
        drawText(&bigFontFontCharset, &bigFontSheet, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 0.5f, 0.5f, 1.0f, true, "Loading...");
        endEyeLayer();
    }
    C3D_FrameEnd(0);

    uiStackSetStack(&gameplayStack);
    uiStackClear();
    uiStackPush(&gameplayDef, ANIM_NONE, ANIM_NONE, PUSH_ROOT);

    updatePlayerColors();

    exitingLevel = false;
        
    UIInput touch;

    if (state.onlineLevel) {
        int returned = loadOnlineLevel(currLevelString);
        levelResult = returned;
        if (returned) {
            outputLog("Failed %d\n", returned);

            uiStackPushGameState(STATE_MENU);
            uiStackUpdate(&touch);
            state.onlineLevel = false;
            exitingLevel = true;
            unloadLevel();
            return;
        }
    } else {
        const char *path;
        if (state.customLevel) {
            path = state.customLevelPath;
        } else {
            path = currentMainLevelPack->levels[currLevelID].gmdPath;
        }

        int returned = loadLevel(path);
        levelResult = returned;
        if (returned) {
            outputLog("Failed %d\n", returned);
            
            uiStackPushGameState(STATE_MENU);
            uiStackUpdate(&touch);
            exitingLevel = true;
            unloadLevel();
            return;
        }

        if (!state.customLevel) {
            snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", currentMainLevelPack->levels[currLevelID].levelName);
        }
    }

    playLevelSong(levelInfo.songOffset);

    frameTimer = 0;
    if (songLoaded) {
        pausePlaybackMP3();
    }

    bool beingFaded = true;
    float levelStartDelay = 1.f;

    firstLoadInitVariables();

    initOpSystem();

    // Particle
    allocateParticles();
    initParticles(p1Color, p2Color);
    clearPracticeMode();

    fixedDt = true;

    syncPreciseInput(false);
    memset(piSubstepPresses, 0, sizeof(piSubstepPresses));

    u64 lastTime = svcGetSystemTick();
    u64 start = svcGetSystemTick();
    bool oldWide = settingsState.wideEnabled;
    bool oldStereo = settingsState.stereoEnabled;
    
    aptHook(&cookie, aptCallback, NULL);

    // Main loop
    while (aptMainLoop()) {
        processSaving();

        start = svcGetSystemTick();
        hidScanInput();

        piPoll();
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPos = touchPos;
        touch.interacted = false;
        touch.down = hidKeysDown();
        touch.held = hidKeysHeld();
        touch.up = hidKeysUp();
        hidCircleRead(&touch.cpad);
        static u32 kHeldPaused;

        state.hitboxDisplay = 0;

        if (touch.down & KEY_X && settingsState.enableDebugBindings) {
            state.noclip ^= 1;
        }

        if (state.noclip) {
            cheated = true;
            cheatsUsed[CHEAT_NOCLIP] = true;
        }

        if ((touch.down & KEY_L) && (touch.held & KEY_B) && settingsState.enableDebugBindings)
            state.profiling ^= 1;

        if ((touch.down & KEY_R) && (touch.held & KEY_B) && settingsState.enableDebugBindings) {
            cheated = true;
            cheatsUsed[CHEAT_HITBOX_DISPLAY] = true;
            if (settingsState.hitboxesEnabled && settingsState.hitboxTrail) {
                settingsState.hitboxesEnabled = false;
                settingsState.hitboxTrail = false;
            } else if (settingsState.hitboxesEnabled && !settingsState.hitboxTrail) {
                settingsState.hitboxTrail = true;
            } else settingsState.hitboxesEnabled = true;
        }      

        if (settingsState.hitboxesEnabled || state.hitboxEnabledWhenDead) {
            state.hitboxDisplay = 1;
        }
        
        if (settingsState.hitboxTrail) {
            settingsState.hitboxesEnabled = true;
            state.hitboxDisplay = 2;
        }

        if (settingsState.hitboxesEnabled || settingsState.hitboxTrail) {
            cheated = true;
            cheatsUsed[CHEAT_HITBOX_DISPLAY] = true;
        }
        
        kHeldPaused &= ~touch.up;
        if(!gamePaused){
            touch.held &= ~kHeldPaused;
        }
        
        globalVolume = getVolumeSlider();

        snapshot.renderingMs = 0;

        handleGameplayInput(touchPos, touch.down, touch.held);
        
        for (int i = 0; i < 2; i++) {
            dragParticles[i].emitting = false;
            dragParticles2[i].stationary = true;
            dragParticles2[i].emitting = false;
            shipFireParticles[i].emitting = false;
            secondaryParticles[i].emitting = false;
            shipSecondaryParticles[i].emitting = false;
            burstParticles[i].emitting = false;
            robotFireParticles[i].emitting = false;
            landParticles[i].emitting = false;
        }

        u64 now = svcGetSystemTick();
        delta = (now - lastTime) / (CPU_TICKS_PER_MSEC * 1000);
        u64 frameWindowStart = lastTime;
        lastTime = now;

        if (state.deathTimer <= 0)  {
            float physicsDelta = delta;
            snapshot.physicsMs = 0;
            snapshot.collisions = 0;
            snapshot.collisionChecks = 0;
            snapshot.collisionMs = 0;
            snapshot.playMs = 0;
            snapshot.handlerMs = 0;
            snapshot.handlerMs = 0;
            
            brickDestroyParticles.emitting = false;
            glitterParticles.emitting = false;
            glitterParticlesBottom.emitting = false;
            slowSpeedParticlesBottom.emitting = false;
            normalSpeedParticlesBottom.emitting = false;
            fastSpeedParticlesBottom.emitting = false;
            fasterSpeedParticlesBottom.emitting = false;
            fastestSpeedParticlesBottom.emitting = false;
            
            if (physicsDelta > 0.5f) physicsDelta = STEPS_DT; // Avoid spiral of death

            if (!gamePaused) {
                if (fixedDt) {
                    physicsDelta = STEPS_DT;
                    if (!beingFaded) fixedDt = false;
                }
                accumulator += physicsDelta;
                
                if (levelStartDelay > 0.f) {
                    levelStartDelay -= delta;
                    accumulator = 0.f;
                }

                lastSteps = steps;
                steps = 0;

                // in case of merge conflicts: this needs to stay after the accumulator deposit above.
                // planned is how many substeps this frame is about to run, so if we calculate it
                // before the deposit, the accumulator only has the leftover from last frame and
                // planned will always be 1, making every input land on a single substep like normal
                // frame latching (no error, it will just silently get less precise)
                u32 planned = (u32)(accumulator / STEPS_DT);
                piBeginFrame((u32)frameWindowStart, (u32)now, planned ? planned : 1);

                state.player.gravityChangedMove = false;
                state.player2.gravityChangedMove = false;

                // Run simulation in fixed steps
                while (accumulator >= STEPS_DT) {
                    u64 startPhysics = svcGetSystemTick();
                    float playerXBeforeStep = state.player.x;
                    bool player1UpsideDownBefore = state.player.upsideDown;
                    bool player2UpsideDownBefore = state.player2.upsideDown;

                    if (piEnabled) {
                        piApplySubstep((u32)steps);

                        state.oldInput = state.input;
                        state.oldInputP2 = state.inputP2;

                        state.input.pressedJump = piPressed(PI_PLAYER_1);
                        state.input.holdJump = piHold(PI_PLAYER_1) || state.input.pressedJump;

                        if (levelInfo.twoPlayerMode) {
                            state.inputP2.pressedJump = piPressed(PI_PLAYER_2);
                            state.inputP2.holdJump = piHold(PI_PLAYER_2) || state.inputP2.pressedJump;
                        } else {
                            state.inputP2 = state.input;
                        }

                        if (piPressed(PI_PLAYER_1) || piPressed(PI_PLAYER_2)) {
                            piSubstepPresses[steps < PI_SUBSTEP_BUCKETS ? steps : PI_SUBSTEP_BUCKETS - 1]++;
                        }
                    }

                    state.pendingAutoCheckpoint = false;

                    state.currentPlayer = 0;
                    state.oldPlayer = state.player;
                    
                    currInput = state.input;
                    currOldInput = state.oldInput;

                    trail = &trailP1;
                    waveTrail = &waveTrailP1;
                    handlePlayer(&state.player);
                    state.player.gravityChangedMove |= state.player.upsideDown != player1UpsideDownBefore;
                    handleMirrorTransition();

                    state.levelProgress = (state.player.x / levelInfo.lastObjX) * 100;
                    if (state.levelProgress < 0) {
                        state.levelProgress = 0;
                    }
                    
                    if (state.levelProgress > 100) {
                        state.levelProgress = 100;
                    }

                    if (state.dead) break;

                    if (state.dual) {
                        // Run second player
                        state.oldPlayer = state.player2;
                        state.currentPlayer = 1;
                        trail = &trailP2;
                        waveTrail = &waveTrailP2;

                        currInput = state.inputP2;
                        currOldInput = state.oldInputP2;
                        handlePlayer(&state.player2);
                        state.player2.gravityChangedMove |= state.player2.upsideDown != player2UpsideDownBefore;

                        if (state.dead) break;

                        //revert state back to first player
                        state.currentPlayer = 0;
                        state.oldPlayer = state.player;
                    }

                    runPendingCheckpoint();

                    triggerDt = STEPS_DT;
                    moveLockPlayerXDelta = state.player.x - playerXBeforeStep;
                    moveLockPlayerYDelta = state.player.deltaY;
                    u64 startStepTriggers = svcGetSystemTick();
                    handleStepTriggers();
                    u64 endStepTriggers = svcGetSystemTick();
                    float stepTriggerTime = (endStepTriggers - startStepTriggers) / (CPU_TICKS_PER_MSEC);
                    snapshot.stepTriggerMs += stepTriggerTime;

                    runCamera();
                    handleBgFlash();
                    handleRespawnEffect();

                    u64 endPhysics = svcGetSystemTick();
                    float physicsTime = (endPhysics - startPhysics) / (CPU_TICKS_PER_MSEC);
                    snapshot.physicsMs += physicsTime;

                    accumulator -= STEPS_DT;
                    steps++;
                    levelFrame++;
                }
            }
        }

        // Death stops the lock triggers
        if (state.dead) {
            moveLockPlayerXDelta = 0; 
            moveLockPlayerYDelta = 0;
        }

        if (!gamePaused) {
            frameCounter++;
            frameTimer += delta;
            triggerDt = delta;

            if (state.dead && state.deathTimer <= 0.f) {
                state.deathTimer = (settingsState.quickRetry ? 0.5f : 1.f);
                bool hadNewBest = false;
                if (!cheated) {
                    LevelData *levelDataSel = &currentLevelEntry->data;
                    // Save new best
                    int progress = (int)state.levelProgress;
                    if (state.practiceMode) {
                        if (state.currentData.maxPractice < progress) {
                            state.currentData.maxPractice = progress;
                            levelDataSel->practiceProgress = progress;
                        }
                    } else {
                        if (state.currentData.maxNormal < progress) {
                            initNewBestPopup(progress);
                            hadNewBest = true;
                            state.currentData.maxNormal = progress;
                            levelDataSel->normalProgress = progress;
                        }
                    }
                }

                // New best makes the respawn timer a bit longer
                if (hadNewBest) state.deathTimer = 1.4f;

                handleDeath((state.currentPlayer == 1) ? &state.player2 : &state.player, true);
                runOnDeathTriggers();
            }

            if (state.deathTimer > 0.f) {
                state.deathTimer -= delta;

                fadeToAmplitude(0);

                if (state.player.gamemode == GAMEMODE_WAVE) {
                    if (waveTrailP1.opacity > 0) waveTrailP1.opacity -= 0.02f * 4;
            
                    if (waveTrailP1.opacity <= 0) {
                        waveTrailP1.opacity = 0;
                        waveTrailP1.nuPoints = 0;
                    }
                }

                if (state.player2.gamemode == GAMEMODE_WAVE) {
                    if (waveTrailP2.opacity > 0) waveTrailP2.opacity -= 0.02f * 4;

                    if (waveTrailP2.opacity <= 0) {
                        waveTrailP2.opacity = 0;
                        waveTrailP2.nuPoints = 0;
                    }
                }
                
                handleMirrorTransition();

                if (state.deathTimer <= 0.f) {
                    initVariables();
                    reloadLevel(); 
                    //resets UI coins on gameplay screen
                    resetCoins();

                    if (state.practiceMode) {
                        if (settingsState.autoCheckpoints && playerGamemodeIsFlying(&state.deathPlayer) && pseudoCheckpointExists) {
                            deleteLastCheckpoint();
                            pseudoCheckpointExists = false;
                        }

                        if (getCheckpointCount() > 0) {
                            restoreCheckpoint();
                        } else if (settingsState.practiceMusicSync) {
                            seekMP3(levelInfo.songOffset);
                        }
                    }

                    if (songLoaded) unpausePlaybackMP3();
                    syncPreciseInput(false);
                    fixedDt = true;
                    state.dead = false;
                    state.hitboxDisplay = 0;
                    continue;
                }
            }

            handlePracticeMode();
            handleShake(delta);

            if (state.mirroring) {
                state.mirrorTimer += delta;
                if (state.mirrorTimer > MIRROR_DURATION) {
                    state.mirroring = false;
                    state.mirrorFactor = state.intendedMirrorFactor;
                    state.mirrorSpeedFactor = 1 - 2*state.mirrorFactor;

                    // When mirror transition ends, put a wave trail point
                    if (state.player.gamemode == GAMEMODE_WAVE) {
                        waveTrailP1.positionR = (Vec2D){ state.player.x, state.player.y };
                        motionTrailResumeStroke(&waveTrailP1);
                        motionTrailAddWavePoint(&waveTrailP1);
                    }
                    if (state.dual && state.player2.gamemode == GAMEMODE_WAVE) {
                        waveTrailP2.positionR = (Vec2D){ state.player2.x, state.player2.y };
                        
                        motionTrailResumeStroke(&waveTrailP2);
                        motionTrailAddWavePoint(&waveTrailP2);
                    }
                }
            }

            u64 startTrig = svcGetSystemTick();
            if (state.deathTimer > 0.f) handleStepTriggers();
            handleColTriggers();
            handleCopyChannels();
            handleAlphaTriggers();
            handlePulseTriggers();
            calculateLbg();
            flushGroupCombinationVisualMoves();
            u64 endTrig = svcGetSystemTick();
            u64 ticksTrig = endTrig - startTrig;
            snapshot.triggersMs = ticksTrig / CPU_TICKS_PER_MSEC;

            u64 startObj = svcGetSystemTick();
            createObjects();
            u64 endObj = svcGetSystemTick();
            u64 ticksObj = endObj - startObj;
            snapshot.renderingMs = ticksObj / CPU_TICKS_PER_MSEC;

            u64 startPart = svcGetSystemTick();
            updatePlayerEffects(delta);
            updateRays(delta);

            // End wall particles
            if (levelInfo.wallY > 0) {
                updateParticleSystem(&endWallParticles, delta);
                updateParticleSystem(&endWallFirework, delta);
                updateParticleSystem(&levelCompleteEffectP1, delta);
                updateParticleSystem(&levelCompleteEffectP2, delta);

                handleLevelCompletePopup(delta);
            }

            handleNewBestPopup(delta);

            updateCollectEffect(delta);
            updateCollectableEffect(delta);

            float calc_x_speed_particles = SCREEN_WIDTH_AREA;
            float calcYSpeedParticles = (SCREEN_HEIGHT_AREA / 2);

            slowSpeedParticles.emitterX = calc_x_speed_particles;
            slowSpeedParticles.emitterY = calcYSpeedParticles;
            slowSpeedParticles.emitting = slowSpeedParticlesTimer > 0;
            if (slowSpeedParticlesTimer > 0) {
                slowSpeedParticlesTimer -= delta;
            }

            normalSpeedParticles.emitterX = calc_x_speed_particles;
            normalSpeedParticles.emitterY = calcYSpeedParticles;
            normalSpeedParticles.emitting = normalSpeedParticlesTimer > 0;
            if (normalSpeedParticlesTimer > 0) {
                normalSpeedParticlesTimer -= delta;
            }

            fastSpeedParticles.emitterX = calc_x_speed_particles;
            fastSpeedParticles.emitterY = calcYSpeedParticles;
            fastSpeedParticles.emitting = fastSpeedParticlesTimer > 0;
            if (fastSpeedParticlesTimer > 0) {
                fastSpeedParticlesTimer -= delta;
            }

            fasterSpeedParticles.emitterX = calc_x_speed_particles;
            fasterSpeedParticles.emitterY = calcYSpeedParticles;
            fasterSpeedParticles.emitting = fasterSpeedParticlesTimer > 0;
            if (fasterSpeedParticlesTimer > 0) {
                fasterSpeedParticlesTimer -= delta;
            }

            fastestSpeedParticles.emitterX = calc_x_speed_particles;
            fastestSpeedParticles.emitterY = calcYSpeedParticles;
            fastestSpeedParticles.emitting = fastestSpeedParticlesTimer > 0;
            if (fastestSpeedParticlesTimer > 0) {
                fastestSpeedParticlesTimer -= delta;
            }
            
            endWallParticles.emitterX = levelInfo.wallX - 15;
            endWallParticles.emitterY = levelInfo.wallY;
            endWallParticles.emitting = levelInfo.wallY > 0;

            updateUseEffects(delta, getUseEffectArrayPtr(GFX_TOP));
            updateUseEffects(delta, getUseEffectArrayPtr(GFX_TOP_BUT_ABOVE_LEVEL));
            updateObjectParticles(delta);
            u64 endPart = svcGetSystemTick();
            u64 ticksPart = endPart - startPart;
            snapshot.particlesMs = ticksPart / CPU_TICKS_PER_MSEC;

            // Update trails
            motionTrailUpdate(&trailP1, delta);
            motionTrailUpdateWaveTrail(&waveTrailP1, delta);
            updateP1Trail(&state.player, 0);

            motionTrailUpdate(&trailP2, delta);
            motionTrailUpdateWaveTrail(&waveTrailP2, delta);
            updateP1Trail(&state.player2, 1);
            
            // If not dual, fade the p2 wave trial
            if (!state.dual) {
                if (waveTrailP2.opacity > 0) waveTrailP2.opacity -= (0.02f * STEPS_HZ) * delta;
                
                if (waveTrailP2.opacity <= 0) {
                    waveTrailP2.opacity = 0;
                    waveTrailP2.nuPoints = 0;
                }
            }

        } else{
            kHeldPaused = touch.held;
        }

        uiStackUpdate(&touch);

        if (stereoSupported() && settingsState.stereoEnabled && !threeDWarningPassed && getStereoSlider() > 0.f && strcmp("3d_warning_pop_up", uiStackGetCurrentScreenName()) != 0 && uiStackGetFadeStatus() == FADE_STATUS_NONE) {
            if (!gamePaused) pauseGame();
            if (!gamePaused) return; 
            TwoOptionPopupData *threeDWarningData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
            if (!threeDWarningData) return;

            threeDWarningData->text = strdup("3D can significantly lower performance.\nAre you sure you want to keep it on?");
            threeDWarningData->title = strdup("Disclaimer");
            threeDWarningData->proceedText = strdup("Enable");
            threeDWarningData->cancelText = strdup("Disable");
            uiStackPush(&threeDWarningPopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
            uiStackPushData(threeDWarningData);
            threeDWarningPassed = true;
        }

        // If the wide or 3D settings have been changed, reinitialize screens
        if (settingsState.wideEnabled != oldWide || settingsState.stereoEnabled != oldStereo) {
            gspWaitForVBlank();
            applyScreenModes();
            gspWaitForVBlank();
            reinitializeScreens();
            oldWide = settingsState.wideEnabled;
            oldStereo = settingsState.stereoEnabled;
        }

        // Frees a render target, so keep it out of the frame below
        updateStereoTarget();

        // Handle level being completed
        if (levelInfo.completing) {
            int status = handleWallCutscene(delta, &touch);
            // Exiting
            if (status == 1) {
                exitingLevel = true;
                uiStackPushGameState(STATE_MENU);
                // Restarting level
            } else if (status == 2) {
                levelInfo.completing = false;
                levelCompleteDestroy();
                continue;
            }
        }

        u64 end = svcGetSystemTick();
        u64 ticks = end - start;

        // Render the scene
        updateBottomParticles(delta);
        updateTouchEffect(delta);

        bool swapScreens = settingsState.swapGameplayScreens && !inLevelComplete;
        C3D_RenderTarget *playScreen = swapScreens ? bot : top;
        C3D_RenderTarget *sideScreen = swapScreens ? top : bot;
        UIScreen *swap_gameplay_screen = swapScreens ? uiStackGetScreen("gameplay", SCREEN_BTM) : NULL;
        UIScreen *swap_gameplay_top = swapScreens ? uiStackGetScreen("gameplay", SCREEN_TOP) : NULL;
        int world_width = swapScreens ? SCREEN_BOT_WIDTH : SCREEN_WIDTH;

        if(C3D_FrameBegin(settingsState.vsync ? C3D_FRAME_SYNCDRAW : C3D_FRAME_NONBLOCK)){
            iconCacheBeginFrame();
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);

            // Top screen, drawn once per eye when 3D is on
            for (int eye = 0; beginTopEyeTarget(eye, playScreen); eye++) {
                beginEyeLayer(DEPTH_BACKGROUND);
                drawBackground(state.backgroundX / 8, -(state.cameraY / 8) + 200);
                endEyeLayer();

                C2D_ViewScale(SCALE, SCALE);
                C2D_ViewTranslate(0, CAM_Y_MTX_OFFSET);

                // The level rides in front of the screen, the background stays way back
                beginEyeLayer(DEPTH_LEVEL);

                drawObjects();

                drawEndWall(delta);

                drawAttemptText();

                drawGround(state.groundX, state.cameraY, 0, false, world_width);

                if (state.groundYGfx > 2) {
                    if (state.cameraY - LEVEL_Y_OFFSET + state.groundYGfx > 0) drawGround(state.groundX, state.cameraY, state.cameraY + state.groundYGfx - LEVEL_Y_OFFSET, false, world_width);
                    drawGround(state.groundX, state.cameraY, state.cameraY - LEVEL_Y_OFFSET + SCREEN_HEIGHT_AREA - state.groundYGfx, true, world_width);
                }

                changeBlending(true);
                drawGravitySweeps();

                drawUseEffects(getUseEffectArrayPtr(GFX_TOP_BUT_ABOVE_LEVEL));

                if (levelInfo.wallY > 0) {
                    drawParticleSystem(&endWallFirework, 0, 0, 1);
                    drawParticleSystem(&levelCompleteEffectP1, 0, 0, 1);
                    drawParticleSystem(&levelCompleteEffectP2, 0, 0, 1);
                }

                endEyeLayer();

                changeBlending(false);

                if (swapScreens) C2D_ViewTranslate(-SWAP_CENTER_SHIFT, 0);

                if (levelInfo.wallY > 0) {
                    beginEyeLayer(DEPTH_POPUP);
                    drawLevelCompletePopup();
                    endEyeLayer();
                }

                beginEyeLayer(DEPTH_POPUP);
                drawNewBestPopup();
                endEyeLayer();

                if (swapScreens) C2D_ViewTranslate(SWAP_CENTER_SHIFT, 0);

                C2D_ViewTranslate(0, -CAM_Y_MTX_OFFSET);
                C2D_ViewScale(1/SCALE, 1/SCALE);

                beginEyeLayer(DEPTH_POPUP);
                //gameplay_screen_top_loop();
                if (swapScreens) {
                    swapDrawHud(swap_gameplay_top);

                    if (gamePaused) {
                        uiStackDraw(SCREEN_BTM);
                    } else if (swap_gameplay_screen) {
                        uiRunFuncOnTag(swap_gameplay_screen, "pause_btn", swapDrawIdentity);
                        uiRunFuncOnTag(swap_gameplay_screen, "practice_buttons", swapDrawIdentity);
                    }

                    changeBlending(true);
                    drawTouchEffect();
                    changeBlending(false);
                } else {
                    uiStackDraw(SCREEN_TOP);
                }
                drawLevelCompleteTop();
                drawStackFade();
                //draw_stack_debug();
                endEyeLayer();
            }

            // Bottom screen
            for (int eye = 0; beginTopEyeTarget(eye, sideScreen); eye++) {
                float dim = dimSecondaryScreen;
                float side_width = swapScreens ? SCREEN_WIDTH : SCREEN_BOT_WIDTH;

                if (dim < 1.f) {
                    drawBackground((state.backgroundX / 8) + 40, 200);

                    C2D_ViewScale(SCALE, SCALE);
                    C2D_ViewTranslate(0, CAM_Y_MTX_OFFSET);

                    changeBlending(true);
                    drawBottomParticles();
                    changeBlending(false);

                    drawGround(state.groundX + 52.5f, 0.f, -71.f, false, side_width);
                    drawGround(state.groundX + 52.5f, 0.f, 210.f, true, side_width);

                    C2D_ViewTranslate(0, -CAM_Y_MTX_OFFSET);
                    C2D_ViewScale(1/SCALE, 1/SCALE);
                }

                if (dim > 0.f) {
                    changeBlending(false);

                    if (dim < 1.f) C2D_DrawRectSolid(0.f, 0.f, 0.f, side_width, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, (int)(dim * 255.f)));
                }

                //gameplay_screen_bot_loop();
                if (swapScreens) {
                    if (gamePaused) {
                        swapDrawPauseWindow(swap_gameplay_top);
                    } else {
                        SwapElement pauseBtn = swapHideTag(swap_gameplay_screen, "pause_btn");
                        SwapElement addCheck = swapHideTag(swap_gameplay_screen, "add_checkpoint");
                        SwapElement removeCheck = swapHideTag(swap_gameplay_screen, "remove_checkpoint");

                        C2D_ViewTranslate(SWAP_SIDE_OFFSET_X, 0);
                        uiStackDraw(SCREEN_BTM);
                        C2D_ViewTranslate(-SWAP_SIDE_OFFSET_X, 0);

                        swapRestoreElement(removeCheck);
                        swapRestoreElement(addCheck);
                        swapRestoreElement(pauseBtn);
                    }
                } else {
                    uiStackDraw(SCREEN_BTM);
                }

                drawLevelComplete();

                if (!swapScreens) {
                    changeBlending(true);
                    drawTouchEffect();
                    changeBlending(false);
                }

                if (state.profiling && eye == EYE_LEFT) {
                    float processingTime = ticks / CPU_TICKS_PER_MSEC;
                    ProfilerUpdateData data = {
                        .touchPos = touchPos,
                        .kDown = touch.down,
                        .steps = steps,
                        .processingTime = processingTime
                    };

                    profilerUpdate(data);
                    if (swapScreens) C2D_ViewTranslate(SWAP_SIDE_OFFSET_X, 0);
                    profilerDraw();
                    if (swapScreens) C2D_ViewTranslate(-SWAP_SIDE_OFFSET_X, 0);
                }

                drawStackFade();
            }

            if (state.noclip) {
                drawText(&bigFontFontCharset, &bigFontSheet, 0, 234, 0.5f, 0.5f, 0, true, "Noclip Activated");
            }
            C2D_ViewReset();

            C3D_FrameEnd(0);
        }

        if (beingFaded && levelStartDelay <= 0.f) {
            if (songLoaded) unpausePlaybackMP3();
            beingFaded = false;
        }

        if (escapeState) {
            escapeState = false;
            gamePaused = false;
            inLevelComplete = false;
            break;
        }
    }

    if (!aptMainLoop()) return;
    
    aptUnhook(&cookie);

    LevelData *levelDataSel = &currentLevelEntry->data;

    levelDataSel->attempts += state.currentData.attempts;
    levelDataSel->jumps += state.currentData.jumps;
    levelDataSel->normalProgress = state.currentData.maxNormal;
    levelDataSel->practiceProgress = state.currentData.maxPractice;

    totalAttempts += state.currentData.attempts;
    totalJumps += state.currentData.jumps;

    if (state.onlineLevel) {
        saveCurrentSaveFile(LEVEL_LIST_ONLINE);
    } else if (state.customLevel) {
        saveCurrentSaveFile(LEVEL_LIST_EXTERNAL);
    } else {
        saveCurrentSaveFile(LEVEL_LIST_MAIN_LEVELS);
    }

    cfgSave();

    freeParticles();

    unloadLevel();

    clearPracticeMode();

    clearRespawnEffect();

    levelCompleteDestroy();

    if (songLoaded) unpausePlaybackMP3();

    if (state.onlineLevel) {
        state.onlineLevel = false;
    }
}

void gameAssetsInit() {
    // Load graphics
    spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    if (!spriteSheet) svcBreak(USERBREAK_PANIC);
    
    spriteSheet2 = C2D_SpriteSheetLoad("romfs:/gfx/portals.t3x");
    if (!spriteSheet2) svcBreak(USERBREAK_PANIC);

    spriteSheet3 = C2D_SpriteSheetLoad("romfs:/gfx/sprites_2p0.t3x");
    if (!spriteSheet3) svcBreak(USERBREAK_PANIC);

    spriteSheet4 = C2D_SpriteSheetLoad("romfs:/gfx/sprites_2p1.t3x");
    if (!spriteSheet4) svcBreak(USERBREAK_PANIC);

    animatedSheet = C2D_SpriteSheetLoad("romfs:/gfx/animated.t3x");
    if (!animatedSheet) svcBreak(USERBREAK_PANIC);

    playerEffectsSheet = C2D_SpriteSheetLoad("romfs:/gfx/playerEffects.t3x");
    if (!playerEffectsSheet) svcBreak(USERBREAK_PANIC);
    
    glowSheet = C2D_SpriteSheetLoad("romfs:/gfx/glow.t3x");
    if (!glowSheet) svcBreak(USERBREAK_PANIC);
    
    iconCacheInit();

    trailSheet = C2D_SpriteSheetLoad("romfs:/gfx/trails.t3x");
    if (!trailSheet) svcBreak(USERBREAK_PANIC);

    particleSheet = C2D_SpriteSheetLoad("romfs:/gfx/particles.t3x");
    if (!particleSheet) svcBreak(USERBREAK_PANIC);
    
    gravLineSheet = C2D_SpriteSheetLoad("romfs:/gfx/effects.t3x");
    if (!gravLineSheet) svcBreak(USERBREAK_PANIC);

    initParticleSystem(&touchDragParticles, &touchDragEffect);
    touchDragParticles.relativeStationary = true;

    initParticleSystem(&touchExplosionParticles, &touchExplosionEffect);
    touchExplosionParticles.relativeStationary = true;

    initParticleSystem(&glitterParticlesBottom, &glitterEffect);
    glitterParticlesBottom.cfg.startColorAlpha = 1.f;
    glitterParticlesBottom.dontApplyCamY = true;

    initParticleSystem(&slowSpeedParticlesBottom, &speedEffectSlow);
    slowSpeedParticlesBottom.relativeStationary = true;

    initParticleSystem(&normalSpeedParticlesBottom, &speedEffectNormal);
    normalSpeedParticlesBottom.relativeStationary = true;

    initParticleSystem(&fastSpeedParticlesBottom, &speedEffectFast);
    fastSpeedParticlesBottom.relativeStationary = true;

    initParticleSystem(&fasterSpeedParticlesBottom, &speedEffectVfast);
    fasterSpeedParticlesBottom.relativeStationary = true;

    initParticleSystem(&fastestSpeedParticlesBottom, &speedEffectFastest);
    fastestSpeedParticlesBottom.relativeStationary = true;
}

static void initLevelArena() {
    struct mallinfo mi = mallinfo();
    size_t free = envGetHeapSize() - mi.uordblks;

    size_t capacity = (size_t) (isN3Ds ? free * 0.8f : free * 0.68f);

    bool could = arenaInit(&levelArena, capacity);
    if (!could) outputLog("Couldn't allocate level arena\n");
}

}

int main(int argc, char* argv[]) {
    using namespace gd3ds;

    // Init libs
    initTrigTable();

    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 4);
    C2D_Init(MAX_SPRITES);
    C2D_Prepare();
    osSetSpeedupEnable(1);
    socInit();
    checkRomType();
    // output_log("argc: %d\n", argc);
    if (argc > 0) { 
        threeDsxPath = argv[0];
        // output_log("3dsx path: %s\n", _3dsx_path);
    }
    
#ifndef IS_RELEASE
    isNightly = true;
    consoleDebugInit(debugDevice_SVC);
#endif

    cfgInit();
    cfguInit();
    checkSystemModel();

    // Sort out wide / 3D before making the targets, they need different sizes
    applyScreenModes();
    reinitializeScreens();

    srand(time(NULL));
    altTitleScreen = randomFloat(0, 1) < (1.f / 256);
    queuedRestart = false;
    
    C2D_SetTintMode(C2D_TintMult);
    
    if(ndspInit()) {
        noDspFirmware();
    }

    requiredLoadingScreenAssetsInit();

    loadingScreenInit();

    //short fade in to loading screen
    int fade = 255;
    while(aptMainLoop()){
        loadingScreenUpdate(0, fade);

        fade -= FADE_SPEED * (1 / 60.f);

        if(fade < 0){
            break;
        }
    }

    uiAssetsInit();
    gameAssetsInit();
    loadingScreenUpdate(10, 0);

    loadSaveFile(SAVE_ROBTOP_SERVER_FILE, &gdServerFile);
    
    loadingScreenUpdate(20, 0);

    loadSaveFile(SAVE_1P9_SERVER_FILE, &gdpsFile);

    loadingScreenUpdate(30, 0);

    loadExternalFile(SAVE_EXTERNAL_LEVELS_FILE, &externalFile);

    loadGdpsInfo();

    migrateOldData();
    
    loadingScreenUpdate(40, 0);

    calculateStats();

    cacheAllSprites();

    loadingScreenUpdate(55, 0);
    
    initDefaultUseEffectPools();
    updatePlayerColors();

    loadingScreenUpdate(75, 0);

    loadSfx();

    initLevelArena();

    memset(&levelInfo, 0, sizeof(LoadedLevelInfo));

    loadingScreenUpdate(100, 0);

    // Unload loading screen
    uiUnloadScreen(&defaultScreen);
    uiUnloadScreen(&defaultScreenTop);

    // Set to known value
    changeBlending(false);

    uiStackSetStack(&menuStack);
    uiStackPushRootInstant(&mainMenuDef);

    bool exit = false;
    
    while (aptMainLoop() && !exit) {
        slowSpeedParticlesBottom.cfg.startColorRed = 255 / 255.f;
        slowSpeedParticlesBottom.cfg.startColorGreen = 255 / 255.f;
        slowSpeedParticlesBottom.cfg.startColorBlue = 0 / 255.f;

        normalSpeedParticlesBottom.cfg.startColorRed = 0 / 255.f;
        normalSpeedParticlesBottom.cfg.startColorGreen = 190 / 255.f;
        normalSpeedParticlesBottom.cfg.startColorBlue = 255 / 255.f;

        fastSpeedParticlesBottom.cfg.startColorRed = 0 / 255.f;
        fastSpeedParticlesBottom.cfg.startColorGreen = 255 / 255.f;
        fastSpeedParticlesBottom.cfg.startColorBlue = 0 / 255.f;

        fasterSpeedParticlesBottom.cfg.startColorRed = 230 / 255.f;
        fasterSpeedParticlesBottom.cfg.startColorGreen = 65 / 255.f;
        fasterSpeedParticlesBottom.cfg.startColorBlue = 255 / 255.f;

        fastestSpeedParticlesBottom.cfg.startColorRed = 255 / 255.f;
        fastestSpeedParticlesBottom.cfg.startColorGreen = 65 / 255.f;
        fastestSpeedParticlesBottom.cfg.startColorBlue = 65 / 255.f;

        switch (gameState) {
            case STATE_MENU:
                uiLoop();
                break;
            case STATE_GAME:
                gameLoop();
                break;
            case STATE_EXIT:
                exit = true;
                break;
        }
    }
    
    // Wait for the game to save
    waitForSaving();

    uiStackSetStack(&menuStack);
    uiStackClear();

    stopMP3();

    closeLogFile();

    freeCachedSprites();
    iconCacheShutdown();

    lightningBolts.clear();
    lightningBolts.shrink_to_fit();

    // Delete graphics
    C2D_SpriteSheetFree(spriteSheet);
    C2D_SpriteSheetFree(spriteSheet2);
    C2D_SpriteSheetFree(spriteSheet3);
    C2D_SpriteSheetFree(spriteSheet4);
    C2D_SpriteSheetFree(animatedSheet);
    C2D_SpriteSheetFree(playerEffectsSheet);
    C2D_SpriteSheetFree(glowSheet);
    C2D_SpriteSheetFree(bgSheet);
    C2D_SpriteSheetFree(trailSheet);
    C2D_SpriteSheetFree(particleSheet);
    C2D_SpriteSheetFree(uiSheet);
    C2D_SpriteSheetFree(ui2Sheet);
    C2D_SpriteSheetFree(groundSheet);
    C2D_SpriteSheetFree(bigFontSheet);
    C2D_SpriteSheetFree(chatFontSheet);
    C2D_SpriteSheetFree(goldFontSheet);
    C2D_SpriteSheetFree(windowSheet);
    C2D_SpriteSheetFree(barSheet);

    cfgFini();

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    ndspExit();

    // restart game if a restart was queued
    if (queuedRestart) {
        restartGame();
    }

    return 0;
}
