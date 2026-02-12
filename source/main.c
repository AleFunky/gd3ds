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

float cam_x = 0;
float cam_y = 0;

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
	
	gfxExit();
	romfsExit();
	exit(22);
}

int main(int argc, char* argv[]) {
	// Init libs
	romfsInit();
	gfxInitDefault();
	
	if(ndspInit()) {
		no_dsp_firmware();
	}

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(MAX_SPRITES);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
	osSetSpeedupEnable(1);
	

	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTargetClear(top, C3D_CLEAR_ALL, 0, 0);
	
	bgSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg.t3x");
	if (!bgSheet) svcBreak(USERBREAK_PANIC);
	
	groundSheet = C2D_SpriteSheetLoad("romfs:/gfx/grounds.t3x");
	if (!groundSheet) svcBreak(USERBREAK_PANIC);
	
	// Load graphics
	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);
	
	spriteSheet2 = C2D_SpriteSheetLoad("romfs:/gfx/portals.t3x");
	if (!spriteSheet2) svcBreak(USERBREAK_PANIC);

	C2D_SetTintMode(C2D_TintMult);

	int returned = load_level("romfs:/Electroman.gmd");
	if (returned) printf("\x1b[9;1HFailed %d", returned);

	returned = play_mp3("romfs:/songs/Electroman.mp3");

	printf("\x1b[8;1HUse dpad to move camera");
	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
			
		if (kDown & KEY_SELECT)
			seek_mp3(0);
			
		if (kDown & KEY_X)
			toggle_playback_mp3();

		u32 kHeld = hidKeysHeld();
		
		if (kHeld & KEY_UP) {
			cam_y += CAM_SPEED;
		}
		
		if (kHeld & KEY_DOWN) {
			cam_y -= CAM_SPEED;
		}

		if (kHeld & KEY_RIGHT) {
			cam_x += CAM_SPEED;
		}
		
		if (kHeld & KEY_LEFT) {
			cam_x -= CAM_SPEED;
		}

		handle_triggers();
		handle_col_triggers();
		calculate_lbg();
		
		printf("\x1b[1;1HSpriteCount: %d\x1b[K", sprite_count);
		printf("\x1b[2;1HCPU:        %6.2f%%\x1b[K", C3D_GetProcessingTime()*6.0f);
		printf("\x1b[3;1HGPU:        %6.2f%%\x1b[K", C3D_GetDrawingTime()*6.0f);
		printf("\x1b[4;1HCmdBuf:     %6.2f%%\x1b[K", C3D_GetCmdBufUsage()*100.0f);
		printf("\x1b[5;1HLinear:     %zu\x1b[K", (int)(linearSpaceFree()));
		printf("\x1b[6;1HTotalHeap:  %lu\x1b[K", envGetHeapSize());
		printf("\x1b[7;1HCamera:     %.2f %.2f\x1b[K", cam_x, cam_y);

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_RenderTargetClear(top, C3D_CLEAR_ALL, 0, 0);
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO);
		C2D_SceneBegin(top);
		
		draw_background(cam_x / 8, -(cam_y / 8) + 200);

		C2D_ViewScale(SCALE, SCALE);

		draw_objects();

		draw_ground(0, false);
		C2D_ViewReset();

		C3D_FrameEnd(0);
	}

	unload_level();

	// Delete graphics
	C2D_SpriteSheetFree(spriteSheet);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	ndspExit();
	return 0;
}
