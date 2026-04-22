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

static bool yes_exit = false;

static UIScreen screen;
static UIElement *content;
static UIElement *content1;
static UIElement *content2;

void exit_info_card(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_info_card},
};

void set_info_content(char *text, char *text2, bool splitText) {
    content = ui_get_element_by_tag(&screen, "content");
    content1 = ui_get_element_by_tag(&screen, "content1");
    content2 = ui_get_element_by_tag(&screen, "content2");
    if (splitText){
        strncpy(content1->label.text, text, 255);
        strncpy(content2->label.text, text2, 255);
    } else {
        strncpy(content->label.text, text, 255);
    }
}

void info_card_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/info_card.txt");
    yes_exit = false;
}

int info_card_loop() {
    u32 kDown = hidKeysDown();

    if (yes_exit || (kDown & KEY_B)) {
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
