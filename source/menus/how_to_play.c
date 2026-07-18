#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "main_menu.h"
#include "menus/components/ui_button.h"

static bool yes_exit = false;

static int current_page = 0;

static UIScreen screen_top = {
};

static UIScreen screen = {
    .isBottom = true
};

static UIWindowButton *next_button;

const char *pages_tags2[] = {
    "page1",
    "page2",
    "page3",
    "page4",
    "page5",
    "page6",
    "page7",
    "page8",
};


void switch_page2(int page) {
    for (int i = 0; i < ARRAY_LEN(pages_tags2); i++) {
        if (i == page) {
            ui_run_func_on_tag(&screen, pages_tags2[page], ui_enable_element);
            ui_run_func_on_tag(&screen_top, pages_tags2[page], ui_enable_element);

        } else {
            ui_run_func_on_tag(&screen, pages_tags2[i], ui_disable_element);
            ui_run_func_on_tag(&screen_top, pages_tags2[i], ui_disable_element);
        }
    }
}

void exit_how_to_play(UIElement* e) {
    yes_exit = true;
}

void action_go_next(UIElement *e) {
    current_page++;
    if (current_page >= ARRAY_LEN(pages_tags2)) {
        yes_exit = true;
    }

    switch_page2(current_page);
}


static UIAction actions[] = {
    { "exit", exit_how_to_play },
    { "next", action_go_next},
};

void how_to_play_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/how_to_play.txt");
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/how_to_play_top.txt");

    ui_screen_open(&screen, ANIM_ZOOM);
    ui_screen_open(&screen_top, ANIM_ZOOM);

    next_button = (UIWindowButton*) ui_get_element_by_tag(&screen, "nextbutton");

    yes_exit = false;

    current_page = 0;

    switch_page2(0);
}

int how_to_play_loop() {
    if (yes_exit) {  
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
        return true;
    }

    if (current_page == 7) ui_button_set_text((UIButton *)next_button, "Exit");

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

void draw_how_to_play_top(){
    ui_screen_draw(&screen_top);
}