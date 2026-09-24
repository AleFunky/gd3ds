#include <3ds.h>
#include <citro2d.h>

#include "save/saving.h"
#include "utils/server_utils.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/creator_menu/online/online_menu.h"
#include "menus/creator_menu/search_menu.h"
#include "online_level_menu.h"

static UILabel *level_name;
static UILabel *level_creator;
static UILabel *uploaded_ago;
static UILabel *updated_ago;
static UILabel *requested_stars;
static UILabel *game_ver;

static UILabel *attempts;
static UILabel *jumps;
static UILabel *normal_percent;
static UILabel *practice_percent;

static void populate_online_info() {
    SearchEntry *curr_entry;
    CreatorEntry *creator_entry;
    LevelEntry *lvl_entry;
    SavedLevelDataEntry *data = get_saved_level_data(online_menu_level_id);
    if (data) {
        curr_entry = &data->search_entry;
        creator_entry = &data->creator_entry;
        lvl_entry = &data->level_entry;
    } else {
        curr_entry = &search_entries[curr_search_id];
        creator_entry = &creator_entries[curr_entry->creatorIndex];
        lvl_entry = level_entry;
    }

    char buffer[256];

    snprintf(buffer, sizeof(buffer), "<#ffff00>%s</>", curr_entry->name);
    ui_label_set_text(level_name, buffer);

    snprintf(buffer, sizeof(buffer), "By: <#ffff00>%s</>", creator_entry->creatorName);
    ui_label_set_text(level_creator, buffer);
    
    if (lvl_entry) {
        snprintf(
            buffer, sizeof(buffer), 
            gdps ? "Uploaded: <#ffff00>%s</>" : "Uploaded: <#ffff00>%s ago</>", 
            lvl_entry->uploadDate
        );
        ui_label_set_text(uploaded_ago, buffer);
        
        snprintf(buffer, sizeof(buffer), "Updated: <#ffff00>%s ago</>", lvl_entry->updateDate);
        ui_label_set_text(updated_ago, buffer);
    } else {
        ui_label_set_text(uploaded_ago, "Uploaded: <#ffff00>N/A</>");
        ui_label_set_text(updated_ago, "Updated: <#ffff00>N/A</>");
    }

    snprintf(buffer, sizeof(buffer), "Stars Requested: <#ffff00>%d</>", curr_entry->reqStars);
    ui_label_set_text(requested_stars, buffer);
    
    snprintf(buffer, sizeof(buffer), "Game Version: <#ffff00>%.1f</>", derive_gj_version(curr_entry->gameVersion));
    ui_label_set_text(game_ver, buffer);
}

static void populate_stats() {
    LevelData *data = &current_level_entry->data;

    char attempts_count[256];
    snprintf(attempts_count, sizeof(attempts_count), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumps_count[256];
    snprintf(jumps_count, sizeof(jumps_count), "<#60abef>Total Jumps</>: %d", data->jumps);

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normal_progress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practice_progress);

    ui_label_set_text(attempts, attempts_count);
    ui_label_set_text(jumps, jumps_count);
    ui_label_set_text(normal_percent, normal);
    ui_label_set_text(practice_percent, practice);
}

static void online_level_infobox_init_top(UIScreen *screen_top) {
    level_name = (UILabel *) ui_get_element_by_tag(screen_top, "name");
    level_creator = (UILabel *) ui_get_element_by_tag(screen_top, "creator");
    uploaded_ago = (UILabel *) ui_get_element_by_tag(screen_top, "uploaded");
    updated_ago = (UILabel *) ui_get_element_by_tag(screen_top, "lastupdated");
    requested_stars = (UILabel *) ui_get_element_by_tag(screen_top, "requestedstars");
    game_ver = (UILabel *) ui_get_element_by_tag(screen_top, "gdversion");
    
    populate_online_info();
}

static void online_level_infobox_init (UIScreen *screen) {
    attempts = (UILabel *) ui_get_element_by_tag(screen, "totalattempts");
    jumps = (UILabel *) ui_get_element_by_tag(screen, "totaljumps");
    normal_percent = (UILabel *) ui_get_element_by_tag(screen, "normalprogressvalue");
    practice_percent = (UILabel *) ui_get_element_by_tag(screen, "practiceprogressvalue");

    populate_stats();
}

const UIScreenDefPair online_infobox_def = {
    .name = "online_infobox",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_level_info_pop_up_top.txt",
        .init = online_level_infobox_init_top
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_info_pop_up.txt",
        .init = online_level_infobox_init,
    }
};
