#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_statistic_card.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "statistics.h"
#include "menus/components/ui_darken.h"

#include "save/saving.h"

static bool yes_exit = false;
static bool exiting = false;

static UIScreen screen = {
    .isBottom = true
};

static UIElement *list;

typedef struct StatisticEntries {
    char *name;
    int *value;
} StatisticEntries;

static const StatisticEntries stats[] = {
    { "Total Attempts", &total_attempts },
    { "Total Jumps", &total_jumps },
    { "Collected Stars", &total_stars }, 
    { "Completed Levels", &completed_main_levels },
    { "Completed Ext. Levels", &completed_external_levels },
    { "Completed Demon Levels", &total_demons },
    { "Collected Secret Coins", &total_coins },
    { "Players Destroyed", &players_destroyed }

};

#define NUM_STATS_ENTRIES (sizeof(stats) / sizeof(StatisticEntries))

UIElement entries[NUM_STATS_ENTRIES];

void exit_statistics(UIElement* e) {
    //start exit animation
    screen.open_anim_done = false;
    screen.open_anim_time = 0.5f - screen.open_anim_time;
    exiting = true;
    screen.disable_element_update = true;
}

static UIAction actions[] = {
    { "exit", exit_statistics }
};

void statistics_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/statistics.txt");

    list = ui_get_element_by_tag(&screen, "list");

    if (list) {
        for (int i = 0; i < NUM_STATS_ENTRIES; i++) {
            char *name = stats[i].name;
            int value = *stats[i].value;

            entries[i] = ui_create_statistic_card(0, 0, i & 1, name, value, NULL);
            ui_list_add(list, &entries[i]);
        }
    }

    exiting = false;
    yes_exit = false;
}

int statistics_loop() {
    if(exiting){
        UIElement *darken = ui_get_element_by_tag(&screen, "darken");
        darken->opacity = ((0.5f - screen.open_anim_time) * 2.f) * darken->darken.targetOpacity;
        ui_darken_reset_opacity(darken);
        if(screen.open_anim_done){
            yes_exit = true;
        }
    }

    if (yes_exit) {
        ui_unload_screen(&screen);
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;

    ui_screen_update(&screen, &touch);

    run_animation_slide(&screen, exiting);

    ui_screen_draw(&screen);

    return false;
}
