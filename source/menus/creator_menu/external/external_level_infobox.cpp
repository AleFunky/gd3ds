#include <3ds.h>
#include "level_loading.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_label.hpp"
#include "save/saving.hpp"

static void externalLevelInfoboxInit(UIScreen *s) {
    UILabel *name = (UILabel *) uiGetElementByTag(s, "levelname");

    uiLabelSetText(name, levelInfo.levelName);

    LevelData *data = &currentLevelEntry->data;

    char attempts[256];
    snprintf(attempts, sizeof(attempts), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumps[256];
    snprintf(jumps, sizeof(jumps), "<#60abef>Total Jumps</>: %d", data->jumps);
    
    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normalProgress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practiceProgress);

    uiLabelSetText((UILabel *) uiGetElementByTag(s, "totalattempts"), attempts);
    uiLabelSetText((UILabel *) uiGetElementByTag(s, "totaljumps"), jumps);
    uiLabelSetText((UILabel *) uiGetElementByTag(s, "normalprogressvalue"), normal);
    uiLabelSetText((UILabel *) uiGetElementByTag(s, "practiceprogressvalue"), practice);
}

extern const UIScreenDefPair externalInfoboxDef = {
    .name = "external_infobox",
    .btm = {
        .path = "romfs:/menus/creator_menu/external/level_info_pop_up.txt",
        .init = externalLevelInfoboxInit
    }
};