#pragma once

#include "../types.hpp"
#include <3ds.h>
#include <c2d/base.h>
#include "../save/saving.hpp"
#include "../level/main_levels.hpp"
#include <stdarg.h>
#include "../utils/utils.hpp"
#include "../utils/json_config.hpp"
#include "game_manager.hpp"

namespace gd3ds::platform
{

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11
#define GAME_TITLE_ID 0x000400000BB41C00

inline ExternalLevelFile externalFile;
inline ServerFile gdServerFile;
inline ServerFile gdpsFile;
inline ServerFile *currentServerFile;

// adapted from Luma3DS - sysmodules/rosalina/source/minisoc.c
inline void enterExclusiveState() {
    Result res = 0;
    __dmb();

    auto& ps = GameManager::get()->platformState;

    if (!ps.inExclusiveState) {
        ndmuInit();
        res = NDMU_EnterExclusiveState(NDM_EXCLUSIVE_STATE_INFRASTRUCTURE);
        if (R_SUCCEEDED(res)) res = NDMU_LockState(); // prevents ndm from switching to StreetPass when the lid is closed
        ps.inExclusiveState = R_SUCCEEDED(res);
        __dmb();
    }
}

inline void exitExclusiveState(bool force) {
    Result res = 0;
    __dmb();

    auto& ps = GameManager::get()->platformState;

    if (ps.inExclusiveState)
    {
        if (!force) {
            res = NDMU_UnlockState();
            if (R_SUCCEEDED(res)) res = NDMU_LeaveExclusiveState();
        }
        ndmuExit();
        ps.inExclusiveState = R_FAILED(res);
        __dmb();
    }
}

inline void loadGdpsInfo() {
    currentServerFile = (GameManager::get()->platformState.isGDPS ? &gdpsFile : &gdServerFile);
    currentMainLevelPack = (GameManager::get()->platformState.isGDPS ? &gdpsLevels : &robtopLevels);
}

// Checks if the game is being emulated by citra/azahar
inline bool isCitra() {
    s64 version = 0;
    svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
    return version != 0;
}

inline FILE *log_file = NULL;

inline void ensureLogFile(void) {
    if (log_file) return;  // already open
    
    log_file = fopen(CONFIG_ROOT "output.txt", "w");
}

inline void closeLogFile(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

inline int outputLog(const char *fmt, ...) {
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

inline void checkSystemModel() {
    u8 model = getModel();
    GameManager::get()->platformState.isN3Ds = model == CFG_MODEL_N2DSXL || model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL || isCitra();
}

inline void checkRomType() {
    u64 id;
    APT_GetProgramID(&id);
    GameManager::get()->platformState.is3Dsx = (id != GAME_TITLE_ID);
}

inline void noDspFirmware(void) {
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

}