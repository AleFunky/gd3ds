#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "search_menu.h"
#include "utils/server_utils.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};
static UICheckBox *robtop_checkbox;
static UICheckBox *gdps_checkbox;

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

static void action_switch_server(UIElement* e) {
    int target = ui_prop_int(&e->custom_properties, "server", 0);
    gdps = (target == 1);
    ui_set_checkbox_checked(robtop_checkbox, !gdps);
    ui_set_checkbox_checked(gdps_checkbox, gdps);
}

void exit_server_switcher(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {

    { "change_server", action_switch_server},
    { "exit", exit_server_switcher }
};

void server_switcher_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/server_switcher_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    gdps_checkbox = (UICheckBox *) ui_get_element_by_tag(&screen, "chk_gdps");
    robtop_checkbox = (UICheckBox *) ui_get_element_by_tag(&screen, "chk_robtop");

    ui_set_checkbox_checked(robtop_checkbox, !gdps);
    ui_set_checkbox_checked(gdps_checkbox, gdps);

    // update_length_tints();
    
    yes_exit = false;
}

int server_switcher_loop() {
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

void server_switcher_draw() {
    ui_screen_draw(&screen);
}