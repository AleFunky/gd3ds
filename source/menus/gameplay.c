#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_progress_bar.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "state.h"

#include "settings.h"
#include "generic_disclaimer.h"
#include "first_boot_disclaimer.h"

#include "gameplay.h"

#include "save/config.h"

bool game_paused = false;
static bool in_disclaimer = false;
static bool in_first_boot_disclaimer = false;
static bool in_settings = false;

static UIScreen screen;
static UIScreen screen_top;
static UIElement *bg_gradient;
static UIElement *progress_bar;
static UIElement *level_name;

void pause_game() {
    game_paused = true;
    if (song_loaded) pause_playback_mp3();
    ui_run_func_on_tag(&screen_top, "pause_menu", ui_enable_element);
    ui_run_func_on_tag(&screen, "paused", ui_enable_element);
    ui_run_func_on_tag(&screen, "not_paused", ui_disable_element);
    in_settings = false;
}

void unpause_game() {
    game_paused = false;
    if (state.death_timer <= 0 && song_loaded) {
        unpause_playback_mp3();
    }
    ui_run_func_on_tag(&screen_top, "pause_menu", ui_disable_element);
    ui_run_func_on_tag(&screen, "paused", ui_disable_element);
    ui_run_func_on_tag(&screen, "not_paused", ui_enable_element);
    in_settings = false;
}

void exit_level() {
    play_sfx(&quit_sound, 1);
    exiting_level = true;
    set_fade_status(FADE_STATUS_OUT);
}

void restart_level() {
    init_variables();
    reload_level(); 
    if (song_loaded) seek_mp3(level_info.song_offset);
    unpause_game();
}

void open_disclaimer() {
    in_disclaimer = true;
    disclaimer_init();
}

void open_first_boot_disclaimer() {
    in_first_boot_disclaimer = true;
    first_boot_disclaimer_init();
}

void open_settings() {
    in_settings = true;
    settings_init();
}

static void action_pause(UIElement *e) {
    pause_game();
}

static void action_unpause(UIElement *e) {
    unpause_game();
}

static void action_exit(UIElement *e) {
    exit_level();
}

static void action_restart(UIElement *e) {
    restart_level();
}

static void action_open_settings(UIElement *e) {
    open_settings();
}

static void action_open_disclaimer(UIElement *e) {
    open_disclaimer();
}

static UIAction actions[] = {
    {"pause", action_pause },
    {"unpause", action_unpause },
    {"exit", action_exit },
    {"restart", action_restart },
    {"settings", action_open_settings },
    {"disclaimer", action_open_disclaimer },
};

void gameplay_screen_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/gameplay.txt");
    bg_gradient = ui_get_element_by_tag(&screen, "gradient");

    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/gameplay_top.txt");;
    progress_bar = ui_get_element_by_tag(&screen_top, "progressalert");
    level_name = ui_get_element_by_tag(&screen_top, "level_title");

    Color color = get_white_if_black(p1_color);

    ui_progress_bar_set_tint(progress_bar, C2D_Color32(color.r, color.g, color.b, 255));
    
    ui_window_set_tint(ui_get_element_by_tag(&screen_top, "bgwindow"), C2D_Color32(0, 0, 0, 127));

    strncpy(level_name->label.text, level_info.level_name, 255);
    
    // Hide pause menu
    ui_run_func_on_tag(&screen_top, "pause_menu", ui_disable_element);
    ui_run_func_on_tag(&screen, "paused", ui_disable_element);
}

int gameplay_screen_top_loop() { 
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);

    progress_bar->progress_bar.value = state.level_progress;

    ui_screen_update(&screen_top, &touch);
    ui_screen_draw(&screen_top);

    return false;
}

int gameplay_screen_bot_loop() {
    u32 kDown = hidKeysDown();

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);

    ColorChannel channel = channels[CHANNEL_BG];
    Color color = channel.color;

    ui_image_set_tint(bg_gradient, C2D_Color32(color.r, color.g, color.b, 255));

    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_settings && !in_disclaimer) {
        ui_screen_update(&screen, &touch);
        
        if ((kDown & KEY_B) && !exiting_level && game_paused) {
            exit_level();
        }
    }

    ui_screen_draw(&screen);

    if (in_settings) {
        int returned = settings_loop();
        if (returned) {
            in_settings = false;
        }
    }

    if (in_disclaimer) {
        int returned = disclaimer_loop();
        if (returned) {
            in_disclaimer = false;
        }
    }

    return false;
}
