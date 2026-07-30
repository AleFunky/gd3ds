#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "save/saving.h"
#include "save/config.h"
#include "search_filters.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

const char *length_tags[] = {
    "tiny",
    "short",
    "medium",
    "long",
    "xl",
};

void exit_length_filter(UIElement* e) {
    yes_exit = true;
}

void length_filter(UIElement *e) {
    lengthFilter = ((UICheckBox *)e)->checked;

    ui_run_func_on_tag(&screen, "button", lengthFilter ? ui_enable_element : ui_disable_element);
}

static UIAction actions[] = {
    { "length", length_filter},
    { "exit", exit_length_filter },
};

void length_filter_init() {

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/length_filter_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    
    ui_run_func_on_tag(&screen, "button", lengthFilter ? ui_enable_element : ui_disable_element);

    if (!lengthFilter){
        for (int i = 0; i < ARRAY_LEN(length_tags); i++)
        {
            ui_run_func_on_tag(&screen, length_tags[i], ui_disable_element);
        }
    }

    UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, "chk_length");
    if (checkbox) {
            checkbox->checked = lengthFilter;
            ui_set_checkbox_checked(checkbox, checkbox->checked);
        }

    yes_exit = false;
}

int length_filter_loop() {
    if (yes_exit) {
        cfg_save();
        ui_unload_screen(&screen);

        return true;
    };

    
    if (!lengthFilter) {
        ui_run_func_on_tag(&screen, "button", ui_disable_element);
        for (int i = 0; i < ARRAY_LEN(length_tags); i++)
        {
            ui_run_func_on_tag(&screen, length_tags[i], ui_disable_element);
        }
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

void length_filter_draw() {
    ui_screen_draw(&screen);
}