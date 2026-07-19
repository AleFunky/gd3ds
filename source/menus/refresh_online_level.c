#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};
static UIScreen screen_top = {
};

void exit_refresh_level(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_refresh_level },
};

void refresh_level_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/refresh_level.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    yes_exit = false;
}

int refresh_level_loop() {
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
    ui_screen_update(&screen_top, &touch);

    return false;
}

void refresh_level_draw_bot() {
    ui_screen_draw(&screen);
}