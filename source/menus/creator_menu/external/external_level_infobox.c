#include <3ds.h>
#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"

static void external_level_infobox_init(UIScreen *s) {
    UILabel *name = (UILabel *) ui_get_element_by_tag(s, "levelname");

    ui_label_set_text(name, level_info.level_name);

    LevelData *data = &level_data;

    char attempts[256];
    snprintf(attempts, sizeof(attempts), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumps[256];
    snprintf(jumps, sizeof(jumps), "<#60abef>Total Jumps</>: %d", data->jumps);
    
    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normal_progress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practice_progress);

    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "totalattempts"), attempts);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "totaljumps"), jumps);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "normalprogressvalue"), normal);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "practiceprogressvalue"), practice);
}

const UIScreenDefPair external_infobox_def = {
    .name = "external_infobox",
    .btm = {
        .path = "romfs:/menus/creator_menu/external/level_info_pop_up.txt",
        .init = external_level_infobox_init
    }
};