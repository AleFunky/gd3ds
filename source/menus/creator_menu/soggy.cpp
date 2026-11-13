#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "mp3_player.hpp"
#include "save/config.hpp"

namespace gd3ds
{

bool gotSogged = false;

static void soggyMenuInit(UIScreen *s) {
    gotSogged = true;
    cfgSave(); // You got sogged

    stopMP3();
    playingMenuLoop = false;
    playMP3("romfs:/songs/SogLoop.mp3", true, 0);
}

extern const UIScreenDefPair soggyDef = {
    .name = "soggy",
    .top = {
        .path = "romfs:/menus/creator_menu/soggy_top.txt"
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/soggy.txt",
        .init = soggyMenuInit
    },
};

}