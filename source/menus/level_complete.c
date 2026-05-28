#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "info_card.h"
#include "state.h"

#define ANIM_DURATION 1.f
#define RESTART_ANIM_DURATION 0.5f

static bool yes_exit = false;
static bool restart = false;
static bool init = false;

static bool animating_down = false;
static bool animating_up = false;

static float anim_time = 0;

static float up_y_start = 0;
static float window_y_pos = 0;

static UIScreen screen_top;
static UIScreen screen;

static UIElement *attempt_text;
static UIElement *jumps_text;
static UIElement *time_text;

static void exit_level_complete(UIElement* e) {
    play_sfx(&quit_sound, 1);
    yes_exit = true;
}

static void restart_level(UIElement* e) {
    if (!animating_up) {
        play_sfx(&play_sound, 1);
        animating_up = true;
        animating_down = false;
        anim_time = 0;
        pause_playback_mp3();
    }
}

static UIAction actions[] = {
    { "restart", restart_level },
    { "exit", exit_level_complete },
};

// This runs the initial animation of the menu coming off screen
static void run_start_animation(float delta) {
    float fade_value = easeValue(BOUNCE_OUT, 0, 240, anim_time, ANIM_DURATION, 1.f);
    window_y_pos = -120 + fade_value;
    up_y_start = fade_value;

    ui_set_pos_on_tag(&screen, SCREEN_BOT_WIDTH / 2, window_y_pos, "window");
    ui_set_pos_on_tag(&screen_top, SCREEN_WIDTH / 2, window_y_pos, "window");

    // Animation end
    if (anim_time >= ANIM_DURATION) {
        animating_down = false;
        anim_time = 0;
    }
    anim_time += delta;
}

// This runs the animation that happens when you press "restart"
static void run_restart_animation(float delta) {
    float fade_value = easeValue(QUAD_IN, up_y_start, 0, anim_time, RESTART_ANIM_DURATION, 1.f);
    window_y_pos = -120 + fade_value;

    ui_set_pos_on_tag(&screen, SCREEN_BOT_WIDTH / 2, window_y_pos, "window");
    ui_set_pos_on_tag(&screen_top, SCREEN_WIDTH / 2, window_y_pos, "window");
    

    // Animation end
    if (anim_time >= RESTART_ANIM_DURATION) {
        animating_up = false;
        restart = true;
        anim_time = 0;
    }
    anim_time += delta;
}


void level_complete_init() {
    init = true;
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete_top.txt");
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete.txt");

    char attempts[64];
    char jumps[64];
    char time[64];

    snprintf(attempts, sizeof(attempts), "Attempts: %d", state.current_data.attempts);
    snprintf(jumps, sizeof(jumps), "Jumps: %d", 0);
    snprintf(time, sizeof(time), "Time: %02d:%02d", 0, 0);

    attempt_text = ui_get_element_by_tag(&screen_top, "attempts");
    if (attempt_text) ui_label_set_text(attempt_text, attempts);

    jumps_text = ui_get_element_by_tag(&screen_top, "jumps");
    if (jumps_text) ui_label_set_text(jumps_text, jumps);

    time_text = ui_get_element_by_tag(&screen_top, "time");
    if (time_text) ui_label_set_text(time_text, time);

    yes_exit = false;
    restart = false;
    animating_down = true;
    animating_up = false;
    anim_time = 0;
    window_y_pos = 0;
}

int level_complete_loop(float delta) {
    if (!init) return 0;

    u32 kDown = hidKeysDown();

    if (animating_down) run_start_animation(delta);
    if (animating_up) run_restart_animation(delta);

    if (yes_exit || (kDown & KEY_B)) {
        return 1;
    }

    if (restart) {
        return 2;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_update(&screen_top, &touch);

    return 0;
}

void level_complete_destroy() {
    init = false;
}

void draw_level_complete() {
    if (init) {
        ui_screen_draw(&screen);
    }
}
void draw_level_complete_top() {
    if (init) {
        ui_screen_draw(&screen_top);
    }
}