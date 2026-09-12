#include "screen_definitions.h"

#include "menus/creator_menu/search/clear_search_filters.h"
#include "menus/creator_menu/creator_menu.h"
#include "menus/creator_menu/online/online_menu.h"
#include "menus/settings_hub/credits.h"
#include "menus/creator_menu/online/two_option_pop_up.h"
#include "menus/creator_menu/external/external_level_infobox.h"
#include "menus/creator_menu/external/external_levels.h"
#include "menus/creator_menu/external/external_popup.h"
#include "menus/first_boot_disclaimer.h"
#include "menus/gameplay.h"
#include "menus/generic_disclaimer.h"
#include "menus/settings_hub/how_to_play.h"
#include "menus/icon_kit.h"
#include "menus/settings_hub/info_card.h"
#include "menus/creator_menu/search/length_filter.h"
#include "menus/level_complete.h"
#include "menus/level_select.h"
#include "menus/loading_screen.h"
#include "menus/main_menu.h"
#include "menus/creator_menu/online/online_level_comments.h"
#include "menus/creator_menu/online/online_level_infobox.h"
#include "menus/creator_menu/online/online_level_menu.h"
#include "menus/palette_kit.h"
#include "menus/creator_menu/saved_levels.h"
#include "menus/creator_menu/search/search_filters.h"
#include "menus/creator_menu/search/server_switcher.h"
#include "menus/creator_menu/search_menu.h"
#include "menus/settings_hub/settings_hub.h"
#include "menus/settings_hub/settings.h"
#include "menus/creator_menu/soggy.h"
#include "menus/creator_menu/search/song_filter.h"
#include "menus/settings_hub/songs.h"
#include "menus/statistics.h"

const UIScreenDefPair *defs[] = {
    &main_menu_def,
    &icon_kit_def,
    &level_select_def,
    &creator_menu_def,
    &palette_kit_def,
    &credits_def,
    &soggy_def,
    &search_menu_def,
    &external_def,
    &settings_hub_def,
    &songs_def,
    &statistics_def,
    &how_to_play_def,
    &settings_def,
    &info_card_def,
    &online_def,
    &saved_levels_def,
    &song_filter_def,
    &length_filter_def,
    &search_filters_def,
    &server_switcher_def,
    &two_option_pop_up_def,
    &delete_pop_up_def,
    &warning_pop_up_def,
    &clear_filters_def,
    &online_infobox_def,
    &online_level_menu_def,
    &online_level_comments_def,
    &external_popup_def,
    &external_infobox_def,
};

const UIScreenDefPair *ui_get_screen_def(const char* name){
    for(size_t i = 0; i < ARRAY_LEN(defs); i++){
        if(strcmp(name, defs[i]->name) == 0){
            return defs[i];
        }
    }
    return &soggy_def;
}