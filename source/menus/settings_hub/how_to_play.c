#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_button.h"

#include "menus/main_menu.h"

static int current_page = 0;

static UIWindowButton *next_button;

const char *how_to_play_pages[] = {
    "page1",
    "page2",
    "page3",
    "page4",
    "page5",
    "page6",
    "page7",
    "page8",
};


void switch_page2(int page, UIScreen *s) {
    for (int i = 0; i < ARRAY_LEN(how_to_play_pages); i++) {
        if (i == page) {
            ui_run_func_on_tag(s, how_to_play_pages[page], ui_enable_element);
            ui_run_func_on_tag(&s->pair->screens[SCREEN_TOP], how_to_play_pages[page], ui_enable_element);

        } else {
            ui_run_func_on_tag(s, how_to_play_pages[i], ui_disable_element);
            ui_run_func_on_tag(&s->pair->screens[SCREEN_TOP], how_to_play_pages[i], ui_disable_element);
        }
    }
}

void action_go_next(UIElement *e, const UIPropertyList *args) {
    current_page++;
    if (current_page >= ARRAY_LEN(how_to_play_pages)) {
        ui_stack_pop();
    }

    switch_page2(current_page, e->screen);
}


static UIActionDef how_to_play_actions[] = {
    { "next", action_go_next}
};

void how_to_play_init(UIScreen *s) {
    next_button = (UIWindowButton*) ui_get_element_by_tag(s, "nextbutton");
    current_page = 0;

    switch_page2(current_page, s);
}

static void how_to_play_update() {
    if (current_page == 7) ui_button_set_text((UIButton *)next_button, "Exit");
}

const UIScreenDefPair how_to_play_def = {
    .name = "how_to_play",
    .top = {
        .path = "romfs:/menus/settings_hub/how_to_play_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/settings_hub/how_to_play.txt",
        .init = how_to_play_init,
        .update = how_to_play_update,
        .action_list = {
            .action_count = ARRAY_LEN(how_to_play_actions),
            .actions = how_to_play_actions
        }
    }
};