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
#include "first_boot_disclaimer.h"

#include "save/config.h"

static bool yes_exit = false;
bool initialDisclaimerAccepted = false;

static UIScreen screen;

void exit_first_boot_disclaimer(UIElement* e) {
    initialDisclaimerAccepted = true;
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_first_boot_disclaimer },
};

void first_boot_disclaimer_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/first_boot_disclaimer.txt");
    yes_exit = false;
}

int first_boot_disclaimer_loop() {
    u32 kDown = hidKeysDown();

    if (yes_exit || (kDown & KEY_B)) {
        cfg_save();
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
