#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "online_level_errorbox.h"
#include "menus/components/ui_label.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

static UILabel *error_label;
static UILabel *title;

void exit_online_errorbox(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_online_errorbox },
};

void online_errorbox_init(char error[64]) {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/info_card.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    error_label = (UILabel *) ui_get_element_by_tag(&screen, "content");
    title = (UILabel *) ui_get_element_by_tag(&screen, "title");
    ui_label_set_text(error_label, error);
    ui_label_set_text(title, "Error");
    yes_exit = false;
}

int online_errorbox_loop() {
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

void online_errorbox_draw_bot() {
    ui_screen_draw(&screen);
}