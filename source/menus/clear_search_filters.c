#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"
#include "search_filters.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};
static UIScreen screen_top = {
};

void exit_clear_search_filters(UIElement* e) {
    yes_exit = true;
}

void action_clear_search_filters(UIElement* e) {
    reset_search_filters();
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_clear_search_filters },
    { "clear", action_clear_search_filters },
};

void clear_search_filters_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/clear_filters.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    
    yes_exit = false;
}

int clear_search_filters_loop() {
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

    ui_screen_draw(&screen);

    return false;
}