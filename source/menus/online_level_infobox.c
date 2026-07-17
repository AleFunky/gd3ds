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

void exit_online_level_infobox(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_online_level_infobox },
};

void online_level_infobox_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_info_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_info_pop_up_top.txt");
    ui_screen_open(&screen_top, ANIM_ZOOM);

    yes_exit = false;
}

int online_level_infobox_loop() {
    if (yes_exit) {
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
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

void online_level_infobox_draw_top() {
    ui_screen_draw(&screen_top);
}

void online_level_infobox_draw_bot() {
    ui_screen_draw(&screen);
}