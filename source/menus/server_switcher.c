#include <3ds.h>
#include <citro2d.h>
#include "main.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "search_menu.h"
#include "utils/server_utils.h"
#include "mp3_player.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_window_button.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

static void darken_text(UIElement* e){
    UILabel *l = (UILabel *)e;

    if(ui_prop_int(&e->parent->custom_properties, "server", 0) != gdps) snprintf(l->text, sizeof(l->text), "<127, 127, 127>%s</>", ui_prop_string(&e->custom_properties, "basetext", "Fuck you"));
    else snprintf(l->text, sizeof(l->text), "<255, 255, 255>%s</>", ui_prop_string(&e->custom_properties, "basetext", "Fuck you"));
}

static void darken_image(UIElement* e){
    UIImage *i = (UIImage *)e;

    if(ui_prop_int(&e->parent->custom_properties, "server", 0) != gdps) ui_image_set_tint(i, C2D_Color32(127, 127, 127, 255));
    else ui_image_set_tint(i, C2D_Color32(255, 255, 255, 255));
}

static void darken_button(UIElement* e){
    UIWindowButton *b = (UIWindowButton *)e;

    if(ui_prop_int(&e->custom_properties, "server", 0) != gdps) ui_window_button_set_tint(b, C2D_Color32(127, 127, 127, 255));
    else ui_window_button_set_tint(b, C2D_Color32(255, 255, 255, 255));
}

static void update_server_buttons(){
    ui_run_func_on_tag(&screen, "server", darken_button);
    ui_run_func_on_tag(&screen, "serverimage", darken_image);
    ui_run_func_on_tag(&screen, "servertext", darken_text);
}

static void action_switch_server(UIElement* e) {
    int target = ui_prop_int(&e->custom_properties, "server", 0);
    bool gdps_before = gdps;
    gdps = (target == 1);

    if(gdps != gdps_before){
        stop_mp3();
        strcpy(menu_loop_path, gdps ? "romfs:/songs/menuLoopGDPS.mp3" : "romfs:/songs/menuLoop.mp3");
        size_t out_size;
        void *buf = read_file(menu_loop_path, &out_size);
        if (buf) {
            play_mp3_buf(buf, out_size, true, 0);
        } else {
            playing_menu_loop = false;
        }

        if(gdps){
            disable_demons();
        } else if(!gdps && filters.isDemon){
            enable_demons();
        }
    }

    filters.super = filters.super && gdps;

    update_server_buttons();
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

    update_server_buttons();

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