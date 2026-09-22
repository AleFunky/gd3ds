#include <3ds.h>
#include <citro2d.h>

#include "save/saving.h"
#include "utils/server_utils.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/creator_menu/online/online_menu.h"
#include "menus/creator_menu/search_menu.h"

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

void populate_online_info() {
    SearchEntry *curr_entry = &search_entries[curr_search_id];  

    char buffer[256];

    snprintf(buffer, sizeof(buffer), "<#ffff00>%s</>", curr_entry->name);
    ui_label_set_text(level_name, buffer);

    snprintf(buffer, sizeof(buffer), "By: <#ffff00>%s</>", creator_entries[curr_entry->creatorIndex].creatorName);
    ui_label_set_text(level_creator, buffer);
    
    snprintf(
        buffer, sizeof(buffer), 
        gdps ? "Uploaded: <#ffff00>%s</>" : "Uploaded: <#ffff00>%s ago</>", 
        level_entry->uploadDate
    );
    ui_label_set_text(uploaded_ago, buffer);
    
    snprintf(buffer, sizeof(buffer), "Updated: <#ffff00>%s ago</>", level_entry->updateDate);
    ui_label_set_text(updated_ago, buffer);

    snprintf(buffer, sizeof(buffer), "Stars Requested: <#ffff00>%d</>", curr_entry->reqStars);
    ui_label_set_text(requested_stars, buffer);
    
    snprintf(buffer, sizeof(buffer), "Game Version: <#ffff00>%.1f</>", derive_gj_version(curr_entry->gameVersion));
    ui_label_set_text(game_ver, buffer);
}

static void online_level_infobox_init_top(UIScreen *screen_top) {
    level_name = (UILabel *) ui_get_element_by_tag(screen_top, "name");
    level_creator = (UILabel *) ui_get_element_by_tag(screen_top, "creator");
    uploaded_ago = (UILabel *) ui_get_element_by_tag(screen_top, "uploaded");
    updated_ago = (UILabel *) ui_get_element_by_tag(screen_top, "lastupdated");
    requested_stars = (UILabel *) ui_get_element_by_tag(screen_top, "requestedstars");
    game_ver = (UILabel *) ui_get_element_by_tag(screen_top, "gdversion");
}

static void online_level_infobox_init (UIScreen *screen) {
    attempts = (UILabel *) ui_get_element_by_tag(screen, "totalattempts");
    jumps = (UILabel *) ui_get_element_by_tag(screen, "totaljumps");
    normal_percent = (UILabel *) ui_get_element_by_tag(screen, "normalprogressvalue");
    practice_percent = (UILabel *) ui_get_element_by_tag(screen, "practiceprogressvalue");

    if (level_entry) populate_online_info();
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
