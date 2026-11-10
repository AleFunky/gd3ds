#include "screen_definitions.hpp"

#include "menus/creator_menu/search/clear_search_filters.hpp"
#include "menus/creator_menu/creator_menu.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/settings_hub/credits.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/creator_menu/external/external_level_infobox.hpp"
#include "menus/creator_menu/external/external_levels.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "menus/gameplay.hpp"
#include "menus/settings_hub/how_to_play.hpp"
#include "menus/icon_kit.hpp"
#include "menus/settings_hub/info_card.hpp"
#include "menus/creator_menu/search/length_filter.hpp"
#include "menus/level_complete.hpp"
#include "menus/level_select.hpp"
#include "menus/loading_screen.hpp"
#include "menus/main_menu.hpp"
#include "menus/creator_menu/online/online_level_comments.hpp"
#include "menus/creator_menu/online/online_level_infobox.hpp"
#include "menus/creator_menu/online/online_level_menu.hpp"
#include "menus/palette_kit.hpp"
#include "menus/creator_menu/saved_levels.hpp"
#include "menus/creator_menu/search/search_filters.hpp"
#include "menus/creator_menu/search/server_switcher.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/settings_hub/settings_hub.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/creator_menu/soggy.hpp"
#include "menus/creator_menu/search/song_filter.hpp"
#include "menus/settings_hub/songs.hpp"
#include "menus/statistics.hpp"
#include "menus/updater.hpp"
#include "menus/updater_settings.hpp"
#include "menus/updater_pop_up.hpp"

const UIScreenDefPair *defs[] = {
    &main_menu_def,
    &icon_kit_def,
    &level_select_def,
    &gameplay_def,
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
    &updater_def,
    &updater_settings_def,
    &updater_pop_up_def,
    &quit_pop_up_def,
    &autoupdate_pop_up_def,
};

extern const UIScreenDefPair *ui_get_screen_def(const char* name){
    for(size_t i = 0; i < ARRAY_LEN(defs); i++){
        if(strcmp(name, defs[i]->name) == 0){
            return defs[i];
        }
    }
    return &soggy_def;
}