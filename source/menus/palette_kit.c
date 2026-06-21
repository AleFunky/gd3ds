#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_color_button.h"
#include "menus/components/ui_window_button.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "palette_kit.h"
#include "icons.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true,
    .open_anim = ANIM_SLIDE_RIGHT
};

const u32 colors[] = {
    // Reds
    ABGR8(253, 212, 206, 255),
    ABGR8(255, 125, 125, 255),
    ABGR8(255, 58, 58, 255),
    ABGR8(255, 0, 0, 255),

    ABGR8(150, 0, 0, 255),
    ABGR8(112, 0, 0, 255),
    ABGR8(82, 2, 0, 255),
    ABGR8(56, 1, 6, 255),

    ABGR8(175, 0, 75, 255),
    ABGR8(128, 79, 79, 255),
    ABGR8(122, 53, 53, 255),
    ABGR8(81, 36, 36, 255),

    // Oranges
    ABGR8(255, 185, 114, 255),
    ABGR8(255, 160, 64, 255),
    ABGR8(255, 125, 0, 255),
    ABGR8(255, 75, 0, 255),

    ABGR8(175, 75, 0, 255),
    ABGR8(163, 98, 70, 255),
    ABGR8(117, 73, 54, 255),
    ABGR8(86, 53, 40, 255),

    ABGR8(150, 50, 0, 255),
    ABGR8(102, 49, 30, 255),
    ABGR8(91, 39, 0, 255),
    ABGR8(71, 32, 0, 255),

    // Yellows
    ABGR8(255, 255, 192, 255),
    ABGR8(255, 250, 127, 255),
    ABGR8(255, 255, 0, 255),
    ABGR8(125, 125, 0, 255),

    ABGR8(253, 224, 160, 255),
    ABGR8(255, 185, 0, 255),
    ABGR8(150, 100, 0, 255),
    ABGR8(80, 50, 14, 255),

    ABGR8(205, 165, 118, 255),
    ABGR8(167, 123, 77, 255),
    ABGR8(109, 83, 57, 255),
    ABGR8(81, 62, 42, 255),

    // Greens
    ABGR8(192, 255, 160, 255),
    ABGR8(177, 255, 109, 255),
    ABGR8(125, 255, 0, 255),
    ABGR8(0, 255, 0, 255), // Default p1

    ABGR8(210, 255, 50, 255),
    ABGR8(75, 175, 0, 255),
    ABGR8(100, 150, 0, 255),
    ABGR8(0, 192, 55, 255),

    ABGR8(0, 255, 125, 255),
    ABGR8(0, 150, 0, 255),
    ABGR8(0, 96, 0, 255),
    ABGR8(0, 64, 0, 255),

    // Cyans
    ABGR8(192, 255, 224, 255),
    ABGR8(148, 255, 228, 255),
    ABGR8(0, 255, 192, 255),
    ABGR8(0, 255, 255, 255), // Default P2, glow

    ABGR8(125, 255, 175, 255),
    ABGR8(67, 161, 138, 255),
    ABGR8(49, 109, 95, 255),
    ABGR8(38, 84, 73, 255),
    
    ABGR8(0, 150, 100, 255),
    ABGR8(0, 125, 125, 255),
    ABGR8(0, 96, 96, 255),
    ABGR8(0, 64, 64, 255),

    // Blues
    ABGR8(160, 255, 255, 255),
    ABGR8(0, 200, 255, 255),
    ABGR8(0, 125, 255, 255),
    ABGR8(0, 0, 255, 255),

    ABGR8(0, 75, 175, 255),
    ABGR8(0, 0, 150, 255),
    ABGR8(1, 7, 112, 255),
    ABGR8(0, 10, 76, 255),

    ABGR8(0, 100, 150, 255),
    ABGR8(0, 73, 109, 255),
    ABGR8(0, 50, 76, 255),
    ABGR8(0, 38, 56, 255),

    ABGR8(118, 189, 255, 255),
    ABGR8(80, 128, 173, 255),
    ABGR8(51, 83, 117, 255),
    ABGR8(35, 60, 86, 255),

    // Purples
    ABGR8(190, 181, 255, 255),
    ABGR8(125, 125, 255, 255),
    ABGR8(125, 0, 255, 255),
    ABGR8(100, 0, 150, 255),

    ABGR8(182, 128, 255, 255),
    ABGR8(75, 0, 175, 255),
    ABGR8(61, 6, 140, 255),
    ABGR8(55, 8, 96, 255),

    ABGR8(77, 77, 143, 255),
    ABGR8(111, 73, 164, 255),
    ABGR8(84, 54, 127, 255),
    ABGR8(66, 42, 99, 255),

    // Magentas
    ABGR8(252, 181, 255, 255),
    ABGR8(255, 0, 125, 255),
    ABGR8(150, 0, 100, 255),
    ABGR8(102, 3, 62, 255),

    ABGR8(255, 0, 255, 255),
    ABGR8(185, 0, 255, 255),
    ABGR8(125, 0, 125, 255),
    ABGR8(71, 1, 52, 255),

    ABGR8(250, 127, 255, 255),
    ABGR8(175, 87, 175, 255),
    ABGR8(130, 67, 130, 255),
    ABGR8(94, 49, 94, 255),

    // Gray scale
    ABGR8(255, 255, 255, 255),
    ABGR8(224, 224, 224, 255),
    ABGR8(175, 175, 175, 255),
    ABGR8(128, 128, 128, 255),
    ABGR8(90, 90, 90, 255),
    ABGR8(64, 64, 64, 255),
    ABGR8(0, 0, 0, 255)
};

static int color_counter = 0;

static int color_page = 0;

static void disable_glow_setting(UIElement *e) {
    e->enabled = false;
}

static void enable_glow_setting(UIElement *e) {
    e->enabled = true;
}

static void set_color_index(UIElement *e) {
    ui_color_button_set_index(e, color_page, color_counter);
    color_counter++;
}

static void action_color_selected(UIElement *e) {
    *current_colors[color_page] = e->color.color_index;
    update_player_colors();
    color_counter = 0;
    ui_run_func_on_tag(&screen, "color", set_color_index);
}

void exit_palette_kit(UIElement* e) {
    yes_exit = true;
}

static void disable_all_color_buttons(UIElement *e) {
    ui_window_button_set_style(e, 4);
}

static void set_p1_page(UIElement *e) {    
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style(e, 5);
    color_page = 0;
    color_counter = 0;
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
    ui_run_func_on_tag(&screen, "color", set_color_index);
}

static void set_p2_page(UIElement *e) {
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style(e, 5);
    color_page = 1;
    color_counter = 0;
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
    ui_run_func_on_tag(&screen, "color", set_color_index);
}

static void set_glow_page(UIElement *e) {
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style(e, 5);
    color_page = 2;
    color_counter = 0;
    ui_run_func_on_tag(&screen, "glow_option", enable_glow_setting);
    ui_run_func_on_tag(&screen, "color", set_color_index);
}

void player_glow_settings(UIElement* e) {
    player_glow_enabled = e->checkbox.checked;
}

static UIAction actions[] = {
    {"exit", exit_palette_kit},
    {"color_selected", action_color_selected},
    {"action_p1", set_p1_page},
    {"action_p2", set_p2_page},
    {"action_glow", set_glow_page},
    {"glow", player_glow_settings}
};


void palette_kit_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/palette_kit.txt");
    yes_exit = false;
    ui_window_set_tint(ui_get_element_by_tag(&screen, "bg_window"), C2D_Color32(0, 0, 0, 64));
    color_counter = 0;
    color_page = 0;
    ui_run_func_on_tag(&screen, "color", set_color_index);
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
    ui_get_element_by_tag(&screen, "chk_glow")->checkbox.checked = player_glow_enabled;
}

int palette_kit_loop() {
    if (yes_exit) {  
        ui_unload_screen(&screen);
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_draw(&screen);

    return false;
}