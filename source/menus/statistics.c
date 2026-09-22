#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "statistics.h"
#include "menus/components/ui_darken.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_label.h"

#include "save/saving.h"

static UIList *list;

typedef struct StatisticEntries {
    char *name;
    int *value;
} StatisticEntries;

static const StatisticEntries stats[] = {
    { "Total Jumps", &total_jumps },
    { "Total Attempts", &total_attempts },
    { "Collected Stars", &total_stars }, 
    { "Completed Levels", &completed_main_levels },
    { "Completed Ext. Levels", &completed_external_levels },
    { "Completed Demon Levels", &total_demons },
    { "Collected Secret Coins", &total_coins },
    { "Players Destroyed", &players_destroyed }
};

void statistics_init(UIScreen *s) {
    list = (UIList *) ui_get_element_by_tag(s, "list");

    if (list) {
        float list_width = list->base.w * 0.5f;

        for (int i = 0; i < ARRAY_LEN(stats); i++) {
            char *name = stats[i].name;
            int value = *stats[i].value;

            UIElement *card = (UIElement *) ui_create_rectangle(s);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
                ui_element_set_size(card, 0, 28);

                // Stat name
                UILabel *stat = ui_create_label(s);
                if (stat) {
                    ui_label_set_text(stat, name);
                    ui_element_set_position((UIElement *) stat, -list_width + 6, 1);
                    ui_element_set_scale((UIElement *) stat, 0.54f);
                    
                    stat->font = 2;

                    ui_element_add_child(card, (UIElement *) stat);
                }

                // Value name
                UILabel *stat_value = ui_create_label(s);
                if (stat_value) {
                    char tmp_value[16];

                    snprintf(tmp_value, sizeof(tmp_value), "%d", value);

                    ui_label_set_text(stat_value, tmp_value);
                    ui_element_set_position((UIElement *) stat_value, list_width - 6, 1);
                    ui_element_set_scale((UIElement *) stat_value, 0.54f);

                    stat_value->font = 2;
                    stat_value->alignment = 1;
                    
                    ui_element_add_child(card, (UIElement *) stat_value);
                }

                ui_list_add(list, card);
            }
        }
    }
}

const UIScreenDefPair statistics_def = {
    .name = "statistics",
    .btm = {
        .path = "romfs:/menus/statistics.txt",
        .init = statistics_init,
    }
};
