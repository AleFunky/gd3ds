#include "menus/creator_menu/search/server_switcher.hpp"
#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "mp3_player.hpp"

#include "utils/server_utils.hpp"

#include "menus/core/ui_stack.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_window_button.hpp"
#include "state.hpp"
#include "menus/creator_menu/search_menu.hpp"

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

static void update_server_buttons(UIScreen *s){
    ui_run_func_on_tag(s, "server", darken_button);
    ui_run_func_on_tag(s, "serverimage", darken_image);
    ui_run_func_on_tag(s, "servertext", darken_text);
}

static void action_switch_server(UIElement* e, const UIPropertyList *args) {
    int target = ui_prop_int(&e->custom_properties, "server", 0);
    bool gdps_before = gdps;
    gdps = (target == 1);

    if(gdps != gdps_before){
        stop_mp3();
        strcpy(menu_loop_path, gdps ? "romfs:/songs/menuLoopGDPS.mp3" : "romfs:/songs/menuLoop.mp3");
        playing_menu_loop = false;
        play_menu_song();

        if (gdps) {
            filters.difficultyFilters = filters.isDemon ? 0 : filters.difficultyFilters;
        }

        if (ui_stack_check_loaded_root(&search_menu_def)) {
            UIScreenPair *screenPair = ui_stack_get_loaded_screen(&search_menu_def);
            UIScreen *screen = &screenPair->screens[SCREEN_BTM];
            if(gdps){
                disable_demons(screen);
            } else if(!gdps && filters.isDemon){
                enable_demons(screen);
            }

            update_difficulty_tints(screen);
        }

        filters.super = filters.super && gdps;
        filters.mainSong = 0;
        filters.songFilter = false;
    }

    update_server_buttons(e->screen);

    load_gdps_info();
}

static UIActionDef server_switcher_actions[] = {
    { "change_server", action_switch_server}
};

void server_switcher_init(UIScreen *s) {
    update_server_buttons(s);
}

extern const UIScreenDefPair server_switcher_def = {
    .name = "server_switcher",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/server_switcher_pop_up.txt",
        .init = server_switcher_init,
        .action_list = {
            .actions = server_switcher_actions,
            .action_count = ARRAY_LEN(server_switcher_actions)
        }
    }
};