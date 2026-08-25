#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"
#include "utils/server_utils.h"
#include "online_menu.h"

static bool cancel = false;
static bool proceed = false;

static UIScreen screen = {
    .isBottom = true
};

static UILabel *title_label;
static UILabel *content_label;


static void action_cancel(UIElement* e) {
    cancel = true;
}
static void action_proceed(UIElement* e) {
    proceed = true;
}

static UIAction actions[] = {
    { "cancel", action_cancel },
    { "proceed", action_proceed },
};

void online_level_warningbox_init(char *title, char *content) {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_warning.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    title_label = (UILabel *) ui_get_element_by_tag(&screen, "title");
    content_label = (UILabel *) ui_get_element_by_tag(&screen, "content");

    ui_label_set_text(title_label, title);
    ui_label_set_text(content_label, content);

    cancel = false;
    proceed = false;
}

int online_level_warningbox_loop() {
    if (cancel) {
        ui_unload_screen(&screen);
        return 1;
    }

    if (proceed) {
        ui_unload_screen(&screen);
        return 2;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);

    return 0;
}

void online_level_warningbox_draw_bot() {
    ui_screen_draw(&screen);
}