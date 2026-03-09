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
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"

static bool exit = false;

static UIScreen screen;

void exit_settings(UIElement* e) {
    exit = true;
}

void wide_settings(UIElement* e) {
    wideEnabled = e->checkbox.checked;
}


void aa_settings(UIElement* e) {
    aaEnabled = e->checkbox.checked;
}

static UIAction actions[] = {
    { "exit", exit_settings },
    { "wide", wide_settings },
    { "aa", aa_settings },
};


void settings_init() {
	ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/settings.txt");
    exit = false;
    ui_get_element_by_tag(&screen, "chk_wide")->checkbox.checked = wideEnabled;
    ui_get_element_by_tag(&screen, "chk_aa")->checkbox.checked = aaEnabled;
}

int settings_loop() {
    u32 kDown = hidKeysDown();

    if (exit || (kDown & KEY_B)) {
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