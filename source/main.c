#include <citro2d.h>

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "objects.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "level/main_levels.h"
#include "fonts/bigFont.h"

#include "save/config.h"

#include <curl/curl.h>

#include "menus/main_menu.h"
#include "menus/level_select.h"
#include "menus/icon_kit.h"
#include "menus/components/ui_screen.h"

#include "player/collision.h"
#include "state.h"

#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

int game_state = STATE_MAIN_MENU;

bool playing_menu_loop = false;

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* bot;

bool is_citra() {
    s64 version = 0;
    svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
    return version != 0;
}

void no_dsp_firmware(void) {
    consoleInit(GFX_TOP, NULL);
    printf("\x1b[01;00H/////////////FATAL///ERROR////////////////////////");
    printf("\x1b[03;00HNDSP could not be initalized.");
    printf("\x1b[05;00HThis is probably because your dspfirm is missing.");
    printf("\x1b[07;00HPut the ndsp firmwared called dspfirm.cdc");
    printf("\x1b[09;00Hon your sd card in sdmc:/3ds");
    printf("\x1b[11;00HCitra/Azahar users only need the file to be there,");
    printf("\x1b[13;00Hit can be empty.");
    printf("\x1b[15;00HPress start to exit.");
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

void game_loop() {
    int returned = load_level(main_levels[curr_level_id].gmd_path);
    if (returned) printf("\x1b[9;1HFailed %d", returned);

    returned = play_mp3(main_levels[curr_level_id].song_path, false);
    toggle_playback_mp3();

    printf("\x1b[8;1HUse dpad to move camera");
    state.camera_x = 0;
    state.camera_y = 0;
    current_fading_effect = FADE_NONE;

    set_fade_status(FADE_STATUS_IN);

    bool being_faded = true;
    bool exiting = false;

    init_variables();

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            exiting = true;
            set_fade_status(FADE_STATUS_OUT);
        }
            
        if (kDown & KEY_SELECT)
            seek_mp3(0);
            
        if (kDown & KEY_X)
            toggle_playback_mp3();

        u32 kHeld = hidKeysHeld();

        state.input.pressedJump = (kDown & KEY_A) == true;
        state.input.holdJump = (kHeld & KEY_A) == true;

        for (size_t i = 0; i < 4; i++) {
            state.old_player = state.player;
            handle_player(&state.player);
            run_camera();
        }

        handle_triggers();
        handle_col_triggers();
        calculate_lbg();

        // Render the scene
        do {
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
            
            // Top screen
            C2D_SceneBegin(top);
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            
            scale_view();
            
            draw_background(state.camera_x / 8, -(state.camera_y / 8) + 200);

            C2D_ViewScale(SCALE, SCALE);

            draw_objects();

            draw_ground(state.camera_x, state.camera_y, 0, false, SCREEN_WIDTH);
            draw_fade();
            C2D_ViewScale(1/SCALE, 1/SCALE);

            // Bottom screen
            C2D_SceneBegin(bot);
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 6, 0.5f, 0, "CPU: %6.2f%%", C3D_GetProcessingTime() * 6.25f);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 18, 0.5f, 0, "GPU: %6.2f%%", C3D_GetDrawingTime() * 6.25f);
            draw_text(bigFont_fontCharset, bigFont_sheet, 0, 30, 0.5f, 0, "Usage: %6.2f%%", (C3D_GetProcessingTime() + C3D_GetDrawingTime()) * 6.25f);
            draw_fade();
            C2D_ViewReset();

            C3D_FrameEnd(0);
        } while (handle_fading());

        if (being_faded) {
            toggle_playback_mp3();
            being_faded = false;
        }

        if (exiting) {
            break;
        }
    }

    unload_level();

    game_state = STATE_LEVEL_SELECT;
}

void game_assets_init() {
    bgSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg.t3x");
    if (!bgSheet) svcBreak(USERBREAK_PANIC);
    
    groundSheet = C2D_SpriteSheetLoad("romfs:/gfx/grounds.t3x");
    if (!groundSheet) svcBreak(USERBREAK_PANIC);
    
    // Load graphics
    spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    if (!spriteSheet) svcBreak(USERBREAK_PANIC);
    
    spriteSheet2 = C2D_SpriteSheetLoad("romfs:/gfx/portals.t3x");
    if (!spriteSheet2) svcBreak(USERBREAK_PANIC);
    
    glowSheet = C2D_SpriteSheetLoad("romfs:/gfx/glow.t3x");
    if (!glowSheet) svcBreak(USERBREAK_PANIC);
    
    iconSheet = C2D_SpriteSheetLoad("romfs:/gfx/icons.t3x");
    if (!iconSheet) svcBreak(USERBREAK_PANIC);
}


int main(int argc, char* argv[]) {
    // Init libs
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(MAX_SPRITES);
    C2D_Prepare();
    osSetSpeedupEnable(1);

    cfg_init();
    
    C2D_SetTintMode(C2D_TintMult);
    
    if(ndspInit()) {
        no_dsp_firmware();
    }

    ui_assets_init();
    game_assets_init();

    cache_all_sprites();
    update_player_colors();

    srand(time(NULL));

    top = C2D_CreateScreenTargetExt(GFX_TOP, GFX_LEFT, aaEnabled);
    bot = C2D_CreateScreenTargetExt(GFX_BOTTOM, GFX_LEFT, aaEnabled);

    bool exit = false;
    while (aptMainLoop() && !exit) {
        switch (game_state) {
            case STATE_MAIN_MENU:
                main_menu_loop();
                break;
            case STATE_LEVEL_SELECT:
                level_select_loop();
                break;
            case STATE_ICON_KIT:
                icon_kit_loop();
                break;
            case STATE_GAME:
                game_loop();
                break;
            case STATE_EXIT:
                exit = true;
                break;
        }
    }

    free_cached_sprites();

    // Delete graphics
    C2D_SpriteSheetFree(spriteSheet);
    C2D_SpriteSheetFree(spriteSheet2);
    C2D_SpriteSheetFree(glowSheet);
    C2D_SpriteSheetFree(bgSheet);
    C2D_SpriteSheetFree(iconSheet);
    C2D_SpriteSheetFree(ui_sheet);
    C2D_SpriteSheetFree(ui_2_sheet);
    C2D_SpriteSheetFree(groundSheet);
    C2D_SpriteSheetFree(bigFont_sheet);
    C2D_SpriteSheetFree(window_sheet);

    cfg_fini();

    // Deinit libs
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    ndspExit();
    return 0;
}
