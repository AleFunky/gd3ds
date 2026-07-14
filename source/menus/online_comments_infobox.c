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

void exit_online_comments_infobox(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_online_comments_infobox },
};

void online_comments_infobox_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_info.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    yes_exit = false;
}

int online_comments_infobox_loop() {
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

    return false;
}

void online_comments_infobox_draw() {
    ui_screen_draw(&screen);
}
