#include <3ds.h>
#include <stdlib.h>
#include <citro2d.h>
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"
#include "state.h"
#include "utils/folders.h"
#include "utils/server_utils.h"
#include "utils/string_helpers.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_window_button.h"
#include "menus/creator_menu/external/external_popup.h"
#include "menus/creator_menu/saved_levels.h"

#include "fonts/goldFont.h"

static UILabel *list_title;
static UILabel *top_title;

static UILabel *error_label;

static UIList *list;

static void saved_levels_init(UIScreen *s) {
    list_title = (UILabel *) ui_get_element_by_tag(s, "listtitle");
    ui_label_set_text(list_title, "Saved Levels");
    top_title = (UILabel *) ui_get_element_by_tag(&s->pair->screens[SCREEN_TOP], "toptitle");
    ui_label_set_text(top_title, "Browse your saved user levels!");

    list = (UIList *) ui_get_element_by_tag(s, "list");
    error_label = (UILabel *)ui_get_element_by_tag(s, "errorLabel");

    play_menu_song();
}

const UIScreenDefPair saved_levels_def = {
    .name = "saved_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_levels_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_levels.txt",
        .init = saved_levels_init,
    }
};