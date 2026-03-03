#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"

#include "level/main_levels.h"

UIScreen screen_top;
UIScreen screen;

bool start_level = false;
bool exit_flag = false;

int curr_level_id = 0;

s8 scroll_dir = 0;

float scrolled = 0;

float anim_time = 0;

UIElement *bg_gradient = NULL;
UIElement *bg_gradient_top = NULL;
UIElement *level_card_window = NULL;

UIElement *level_card_title = NULL;
UIElement *level_card_stars = NULL;
UIElement *level_card_face = NULL;

UIElement *level_card_2_window = NULL;

UIElement *level_card_2_title = NULL;
UIElement *level_card_2_stars = NULL;
UIElement *level_card_2_face = NULL;

#define ANIM_DURATION 0.8f

#define C2D_Color32Const(r, g, b, a) (r | (g << (u32)8) | (b << (u32)16) | (a << (u32)24))

#define NUM_COLORS 9

const u32 default_lvl_colors[] = {
    C2D_Color32Const(0, 0, 232, 255),
    C2D_Color32Const(227, 0, 229, 255), 
    C2D_Color32Const(233, 0, 115, 255),
    C2D_Color32Const(233, 0, 0, 255),
    C2D_Color32Const(231, 112, 0, 255),
    C2D_Color32Const(233, 232, 0, 255),
    C2D_Color32Const(0, 231, 0, 255),
    C2D_Color32Const(0, 227, 228, 255),
    C2D_Color32Const(0, 112, 229, 255),
};

void update_level_name(int level, int card);
void update_level_stars(int level, int card);

void disable_card_2(UIElement *e) {
	e->enabled = false;
}

void enable_card_2(UIElement *e) {
	e->enabled = true;
}

void level_card_move_right(UIElement *e) {
	e->x += 320;
}

void level_card_move_left(UIElement *e) {
	e->x -= 320;
}

void update_level_name(int level, int card) {
	if (level < 0) level = MAIN_LEVELS_NUM-1;
	if (level >= MAIN_LEVELS_NUM) level = 0;

	UIElement *e = (card) ? level_card_2_title : level_card_title;
    float length = get_text_length(bigFont_fontCharset, 1 / 0.85f, main_levels[level].level_name);

    float txt_scale;
    if (level_card_window->w < length) {
        txt_scale = (level_card_window->w / length);
    } else {
        txt_scale = 0.85f;
    }

	e->label.scale = txt_scale;

	strncpy(e->label.text, main_levels[level].level_name, 255);
}

void update_level_stars(int level, int card) {
	if (level < 0) level = MAIN_LEVELS_NUM-1;
	if (level >= MAIN_LEVELS_NUM) level = 0;

	UIElement *e = (card) ? level_card_2_stars : level_card_stars;
	char stars[10] = { 0 };
	snprintf(stars, 9, "%d", main_levels[level].stars);
	strncpy(e->label.text, stars, 255);
}

void update_level_face(int level) {
	if (level < 0) level = MAIN_LEVELS_NUM-1;
	if (level >= MAIN_LEVELS_NUM) level = 0;

	ui_image_set_image(level_card_face, 239 + main_levels[level].difficulty);
}

void action_open_level(void* data) { 
	set_fade_status(FADE_STATUS_OUT);
	start_level = true; 
};

void handle_card_movement() {
	if (scroll_dir != 0) {
		if (anim_time > ANIM_DURATION) {
			update_level_name(curr_level_id, 0);
			update_level_stars(curr_level_id, 0);

			ui_run_func_on_tag(&screen, "level_card_2", disable_card_2);
			ui_set_pos_on_tag(&screen, 160, 90, "level_card");
			ui_set_pos_on_tag(&screen, 160, 90, "level_card_2");
			scroll_dir = 0;
			return;
		}

		float fade_value = easeValue(ELASTIC_OUT, 0, 320, anim_time, ANIM_DURATION, 0.6f);
		float value = 160 + (fade_value) * scroll_dir;
		anim_time += 0.016666f;

		ui_set_pos_on_tag(&screen, value, 90, "level_card");
		ui_set_pos_on_tag(&screen, value - 320 * scroll_dir, 90, "level_card_2");
	}
}

void action_move_right(void* data) { 
	curr_level_id++;
	scroll_dir = -1;
	scrolled = 0;
	anim_time = 0;
	
	if (curr_level_id >= MAIN_LEVELS_NUM) curr_level_id = 0;
	
	ui_set_pos_on_tag(&screen, 160, 90, "level_card");
	ui_run_func_on_tag(&screen, "level_card_2", enable_card_2);
	ui_run_func_on_tag(&screen, "level_card_2", level_card_move_right);
	
	upload_color_to_buffer(0, default_lvl_colors[curr_level_id % NUM_COLORS], ANIM_DURATION);

	update_level_name(curr_level_id - 1, 0);
	update_level_stars(curr_level_id - 1, 0);
	
	update_level_face(curr_level_id);

	update_level_name(curr_level_id, 1);
	update_level_stars(curr_level_id, 1);
};

void action_move_left(void* data) { 
	curr_level_id--;
	scroll_dir = 1;
	scrolled = 0;
	anim_time = 0;

	if (curr_level_id < 0) curr_level_id = MAIN_LEVELS_NUM-1;

	ui_set_pos_on_tag(&screen, 160, 90, "level_card");
	ui_run_func_on_tag(&screen, "level_card_2", enable_card_2);
	ui_run_func_on_tag(&screen, "level_card_2", level_card_move_left);

	upload_color_to_buffer(0, default_lvl_colors[curr_level_id % NUM_COLORS], ANIM_DURATION);

	update_level_name(curr_level_id + 1, 0);
	update_level_stars(curr_level_id + 1, 0);
	
	update_level_face(curr_level_id);

	update_level_name(curr_level_id, 1);
	update_level_stars(curr_level_id, 1);
};

void action_exit(void* data) {
	exit_flag = true;
}

UIAction actions[] = {
    {"open_level", action_open_level},
    {"exit", action_exit},
    {"move_right", action_move_right},
    {"move_left", action_move_left}
};

UIAction actions_top[] = {

};

void level_select_loop() {
	start_level = false;
	ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_select.txt");
	ui_load_screen(&screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/level_select_top.txt");

	// Set window color
	level_card_window = ui_get_element_by_tag(&screen, "card_window");
	ui_window_set_tint(level_card_window, C2D_Color32(0, 0, 0, 127));

	level_card_2_window = ui_get_element_by_tag(&screen, "card_window_2");
	ui_window_set_tint(level_card_2_window, C2D_Color32(0, 0, 0, 127));
	
	ui_window_set_tint(ui_get_element_by_tag(&screen_top, "face_card"), C2D_Color32(0, 0, 0, 127));

	// Get level card components
	level_card_title = ui_get_element_by_tag(&screen, "level_title");
	level_card_stars = ui_get_element_by_tag(&screen, "level_stars");
	level_card_face = ui_get_element_by_tag(&screen_top, "level_face");

	level_card_2_title = ui_get_element_by_tag(&screen, "level_title_2");
	level_card_2_stars = ui_get_element_by_tag(&screen, "level_stars_2");

	update_level_name(curr_level_id, 0);
	update_level_stars(curr_level_id, 0);
	update_level_face(curr_level_id);
	
	ui_run_func_on_tag(&screen, "level_card_2", disable_card_2);

	u32 color = default_lvl_colors[curr_level_id % NUM_COLORS];

	channels[0].color.r = GET_R(color);
    channels[0].color.g = GET_G(color);
    channels[0].color.b = GET_B(color);

	scroll_dir = 0;

	set_fade_status(FADE_STATUS_IN);

		
	// Set bg color
	bg_gradient = ui_get_element_by_tag(&screen, "gradient");
	bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient");

	play_mp3("romfs:/songs/menuLoop.mp3", true);

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_LEFT) {
			action_move_left(NULL);
		}
		if (kDown & KEY_RIGHT) {
			action_move_right(NULL);
		}
		if (kDown & (KEY_START | KEY_A)) {
			action_open_level(NULL);
		}

		if (exit_flag) {
			game_state = STATE_EXIT;
			break; // break in order to return to hbmenu
		}

		UIInput touch;
		touchPosition touchPos;
		hidTouchRead(&touchPos);
		touch.touchPosition = touchPos;
		touch.did_something = false;
		touch.interacted = false;

		handle_col_channel(0);

		ColorChannel channel = channels[0];

		ui_image_set_tint(bg_gradient, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
		ui_image_set_tint(bg_gradient_top, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));

		handle_card_movement();

		ui_screen_update(&screen, &touch);
		ui_screen_update(&screen_top, &touch);
		do {
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
			C2D_SceneBegin(bot);

			ui_screen_draw(&screen);
			draw_fade();

			C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
			C2D_SceneBegin(top);

			ui_screen_draw(&screen_top);
			draw_fade();
			
			C3D_FrameEnd(0);
		} while (handle_fading());

		if (start_level) {
			game_state = STATE_GAME;
			break;
		}
	}
	stop_mp3();
	C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}