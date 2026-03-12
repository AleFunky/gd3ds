#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_button.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "icon_kit.h"

#include "icons.h"

#include "level/main_levels.h"

static UIScreen screen_top;
static UIScreen screen;

static UIElement *bg_gradient = NULL;
static UIElement *bg_gradient_top = NULL;

static UIElement *bg_window = NULL;

static bool exit_flag = false;

static int gamemode_page = 0;

static int icon_selected = 0;

static const int button_images[5] = {
	341,
	351,
	325,
	327,
	329
};

static void disable_all_icon_buttons() {
	ui_button_set_image(ui_get_element_by_tag(&screen, "cube"), button_images[0], 0);
	ui_button_set_image(ui_get_element_by_tag(&screen, "ship"), button_images[1], 0);
	ui_button_set_image(ui_get_element_by_tag(&screen, "ball"), button_images[2], 0);
	ui_button_set_image(ui_get_element_by_tag(&screen, "ufo"),  button_images[3], 0);
	ui_button_set_image(ui_get_element_by_tag(&screen, "dart"), button_images[4], 0);
}

static void set_cube_page(UIElement *e) {
	disable_all_icon_buttons();
	ui_button_set_image(e, button_images[0] + 1, 0);
	gamemode_page = 0;
}

static void set_ship_page(UIElement *e) {
	disable_all_icon_buttons();
	ui_button_set_image(e, button_images[1] + 1, 0);
	gamemode_page = 1;
}

static void set_ball_page(UIElement *e) {
	disable_all_icon_buttons();
	ui_button_set_image(e, button_images[2] + 1, 0);
	gamemode_page = 2;
}

static void set_ufo_page(UIElement *e) {
	disable_all_icon_buttons();
	ui_button_set_image(e, button_images[3] + 1, 0);
	gamemode_page = 3;
}

static void set_wave_page(UIElement *e) {
	disable_all_icon_buttons();
	ui_button_set_image(e, button_images[4] + 1, 0);
	gamemode_page = 4;
}

static void action_exit(UIElement* e) {
	exit_flag = true;
	set_fade_status(FADE_STATUS_OUT);
}

static void move_index_left(UIElement* e) {
	icon_selected -= 1;
	if (icon_selected < 0) {
		icon_selected = ICON_COUNT_PLAYER - 1;
	}
}

static void move_index_right(UIElement* e) {
	icon_selected += 1;
	if (icon_selected >= ICON_COUNT_PLAYER) {
		icon_selected = 0;
	}
}

static UIAction actions[] = {
    {"exit", action_exit},
	{"action_cube", set_cube_page },
	{"action_ship", set_ship_page },
	{"action_ball", set_ball_page },
	{"action_ufo",  set_ufo_page },
	{"action_dart", set_wave_page },
	{"icons_left", move_index_left },
	{"icons_right", move_index_right },
};

static UIAction actions_top[] = {

};

void icon_kit_loop() {
	exit_flag = false;
	ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/icon_kit.txt");
	ui_load_screen(&screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/icon_kit_top.txt");
	
	// Set bg color
	bg_gradient = ui_get_element_by_tag(&screen, "gradient");
	bg_window = ui_get_element_by_tag(&screen, "bg_window");

	bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient");
	
	ui_image_set_tint(bg_gradient, C2D_Color32(167, 167, 167, 255));
	ui_window_set_tint(bg_window, C2D_Color32(0, 0, 0, 64));
	ui_image_set_tint(bg_gradient_top, C2D_Color32(167, 167, 167, 255));

	set_cube_page(ui_get_element_by_tag(&screen, "cube"));

	set_fade_status(FADE_STATUS_IN);

	if (!playing_menu_loop) {
		play_mp3("romfs:/songs/menuLoop.mp3", true);
		playing_menu_loop = true;
	}

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_B) {
			action_exit(NULL);
		}

		UIInput touch;
		touchPosition touchPos;
		hidTouchRead(&touchPos);
		touch.touchPosition = touchPos;
		touch.did_something = false;
		touch.interacted = false;

		ui_screen_update(&screen, &touch);
		ui_screen_update(&screen_top, &touch);
		do {
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			
			// Bottom screen
			C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
			C2D_SceneBegin(bot);
			scale_view();
			draw_fade();

			ui_screen_draw(&screen);

			// Top screen
			C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
			C2D_SceneBegin(top);

			ui_screen_draw(&screen_top);
			spawn_icon_at(gamemode_page, icon_selected, false, 200, 120, 0, 0, 0, 2.f);

			C2D_ViewReset();
			C3D_FrameEnd(0);
		} while (handle_fading());

		if (exit_flag) {
			game_state = STATE_MAIN_MENU;
			break;
		}
	}
	C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}