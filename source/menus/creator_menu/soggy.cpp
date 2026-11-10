#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "mp3_player.hpp"
#include "save/config.hpp"

bool gotSogged = false;

static void soggy_menu_init(UIScreen *s) {
    gotSogged = true;
    cfg_save(); // You got sogged

    stop_mp3();
    playing_menu_loop = false;
    play_mp3("romfs:/songs/SogLoop.mp3", true, 0);
}

extern const UIScreenDefPair soggy_def = {
    .name = "soggy",
    .top = {
        .path = "romfs:/menus/creator_menu/soggy_top.txt"
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/soggy.txt",
        .init = soggy_menu_init
    },
};