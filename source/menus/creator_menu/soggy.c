#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"

#include "save/config.h"

bool gotSogged = false;

static void soggy_menu_init(UIScreen *s) {
    gotSogged = true;
    cfg_save(); // You got sogged

    stop_mp3();
    playing_menu_loop = false;
    play_mp3("romfs:/songs/SogLoop.mp3", true, 0);
}

const UIScreenDefPair soggy_def = {
    .name = "soggy",
    .top = {
        .path = "romfs:/menus/creator_menu/soggy_top.txt"
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/soggy.txt",
        .init = soggy_menu_init
    },
};