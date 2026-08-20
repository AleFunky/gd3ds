#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"
#include "utils/server_utils.h"
#include "online_menu.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};
static UIScreen screen_top = {
};

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

void exit_online_level_infobox(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_online_level_infobox },
};

void populate_online_info() {
    SearchEntry *curr_entry = &search_entries[curr_search_id];  
    ui_label_set_text(level_name, curr_entry->name);

    char tmp_creator[48] = "";
    snprintf(tmp_creator, 48, "<#ffff00>By: %s</>", creator_entries[curr_entry[curr_search_id].creatorIndex].creatorName);
    ui_label_set_text(level_creator, tmp_creator);

    char tmp_upload[150] = "";
    snprintf(tmp_upload, 150, "Uploaded: %s ago", level_entry->uploadDate);
    ui_label_set_text(uploaded_ago, tmp_upload);

    char tmp_update[150] = "";
    snprintf(tmp_update, 150, "Updated: %s ago", level_entry->updateDate);
    ui_label_set_text(updated_ago, tmp_update);

    char tmp_reqstars[24] = "";
    snprintf(tmp_reqstars, 24, "Stars Requested: %d", curr_entry->reqStars);
    ui_label_set_text(requested_stars, tmp_reqstars);

    char tmp_gjver[24] = "";
    snprintf(tmp_gjver, 24, "Game Version: %.1f", derive_gj_version(curr_entry->gameVersion));
    ui_label_set_text(game_ver, tmp_gjver);

    // ui_label_set_text(attempts, );
    // ui_label_set_text(jumps, );
    // ui_label_set_text(normal_percent, );
    // ui_label_set_text(practice_percent, );

};

void online_level_infobox_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_info_pop_up.txt");
    ui_screen_open(&screen, ANIM_ZOOM);

    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_info_pop_up_top.txt");
    ui_screen_open(&screen_top, ANIM_ZOOM);

    level_name = (UILabel *) ui_get_element_by_tag(&screen_top, "name");
    level_creator = (UILabel *) ui_get_element_by_tag(&screen_top, "creator");
    uploaded_ago = (UILabel *) ui_get_element_by_tag(&screen_top, "uploaded");
    updated_ago = (UILabel *) ui_get_element_by_tag(&screen_top, "lastupdated");
    requested_stars = (UILabel *) ui_get_element_by_tag(&screen_top, "requestedstars");
    game_ver = (UILabel *) ui_get_element_by_tag(&screen_top, "gdversion");

    attempts = (UILabel *) ui_get_element_by_tag(&screen, "totalattempts");
    jumps = (UILabel *) ui_get_element_by_tag(&screen, "totaljumps");
    normal_percent = (UILabel *) ui_get_element_by_tag(&screen, "normalprogressvalue");
    practice_percent = (UILabel *) ui_get_element_by_tag(&screen, "practiceprogressvalue");

    populate_online_info();

    yes_exit = false;
}

int online_level_infobox_loop() {
    if (yes_exit) {
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_update(&screen_top, &touch);

    return false;
}

void online_level_infobox_draw_top() {
    ui_screen_draw(&screen_top);
}

void online_level_infobox_draw_bot() {
    ui_screen_draw(&screen);
}