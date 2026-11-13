#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_button.hpp"
#include "menus/creator_menu/soggy.hpp"
#include "state.hpp"

namespace gd3ds
{

void creatorMenuInit(UIScreen *s){
    if (gotSogged) uiButtonSetImage((UIButton *) uiGetElementByTag(s, "create_button"), 20, 1);
    playMenuSong();
}

extern const UIScreenDefPair creatorMenuDef = {
    .name = "creator_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/creator_menu_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/creator_menu.txt",
        .init = creatorMenuInit,
    }
};

}