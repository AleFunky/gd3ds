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
#include "utils/server_utils.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

static void update_length_tint(UIElement *e){
    UILabel *l = (UILabel *)e;
    int opacity = (filters.lengthFilters & (ui_prop_int(&e->custom_properties, "lengthval", 0))) > 0 ? 255 : 127;

    int length_bit = ui_prop_int(&e->custom_properties, "lengthval", 0);
    int length = __builtin_ctz(length_bit);

    char *length_str = "Unkn.";
    if (IN_BOUNDS(length, level_lengths)) {
        length_str = (char *) level_lengths[length];
    }

    snprintf(l->text, sizeof(l->text), "<%d,%d,%d>%s</>", opacity, opacity, opacity, length_str);
}

static void update_length_tints(){
    ui_run_func_on_tag(&screen, "lengthbtn", update_length_tint);
}

void action_set_length(UIElement* e) {
    filters.lengthFilters ^= ui_prop_int(&e->custom_properties, "lengthval", 0);
    update_length_tints();
}

void exit_length_filter(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "length", action_set_length},
    { "exit", exit_length_filter }
};

void length_filter_init() {

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/length_filter_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    update_length_tints();

    yes_exit = false;
}

int length_filter_loop() {
    if (yes_exit) {
        cfg_save();
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

void length_filter_draw() {
    ui_screen_draw(&screen);
}