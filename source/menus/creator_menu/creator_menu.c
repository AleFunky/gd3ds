#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_button.h"
#include "menus/creator_menu/soggy.h"
#include "state.h"

void creator_menu_init(UIScreen *s){
    if (gotSogged) ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "create_button"), 20, 1);
    play_menu_song();
}

const UIScreenDefPair creator_menu_def = {
    .name = "creator_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/creator_menu_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/creator_menu.txt",
        .init = creator_menu_init,
    }
};