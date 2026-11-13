#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "statistics.hpp"
#include "menus/components/ui_darken.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_label.hpp"

#include "save/saving.hpp"
#include "menus/creator_menu/search_menu.hpp"

static UIList *list;

typedef struct StatisticEntries {
    const char *name;
    const int *value;
} StatisticEntries;

static const StatisticEntries stats[] = {
    { "Total Jumps", &totalJumps },
    { "Total Attempts", &totalAttempts },
    { "Collected Stars", &totalStars }, 
    { "Completed Levels", &completedMainLevels },
    { "Completed Ext. Levels", &completedExternalLevels },
    { "Completed Demon Levels", &totalDemons },
    { "Collected Secret Coins", &totalCoins },
    { "Collected User Coins", &totalUserCoins },
    { "Players Destroyed", &playersDestroyed }
};

void statisticsInit(UIScreen *s) {
    list = (UIList *) uiGetElementByTag(s, "list");

    if (list) {
        float listWidth = list->base.w * 0.5f;

        for (size_t i = 0; i < ARRAY_LEN(stats); i++) {
            if (stats[i].value == &totalUserCoins && !userCoinsCounterVisible()) continue;

            const char *name = stats[i].name;
            const int value = *stats[i].value;

            UIElement *card = (UIElement *) uiCreateRectangle(s);

            if (card) {
                uiRectangleSetColor((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
                uiElementSetSize(card, 0, 28);

                // Stat name
                UILabel *stat = uiCreateLabel(s);
                if (stat) {
                    uiLabelSetText(stat, name);
                    uiElementSetPosition((UIElement *) stat, -listWidth + 6, 1);
                    uiElementSetScale((UIElement *) stat, 0.54f);
                    
                    stat->font = 2;

                    uiElementAddChild(card, (UIElement *) stat);
                }

                // Value name
                UILabel *statValue = uiCreateLabel(s);
                if (statValue) {
                    char tmpValue[16];

                    snprintf(tmpValue, sizeof(tmpValue), "%d", value);

                    uiLabelSetText(statValue, tmpValue);
                    uiElementSetPosition((UIElement *) statValue, listWidth - 6, 1);
                    uiElementSetScale((UIElement *) statValue, 0.54f);

                    statValue->font = 2;
                    statValue->alignment = 1;
                    
                    uiElementAddChild(card, (UIElement *) statValue);
                }

                uiListAdd(list, card);
            }
        }
    }
}

extern const UIScreenDefPair statisticsDef = {
    .name = "statistics",
    .btm = {
        .path = "romfs:/menus/statistics.txt",
        .init = statisticsInit,
    }
};
