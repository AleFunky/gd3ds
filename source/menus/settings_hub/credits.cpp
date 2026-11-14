#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_label.hpp"
#include "fonts/bigFont.hpp"
#include "main.hpp"
#include "menus/settings_hub/credits.hpp"

namespace gd3ds
{

struct CreditsEntries {
    const char *contributor;
    const char *contribution;
    const float nameScale;
};

static const CreditsEntries credits[] = {
    {"<i400s0>", "Geometry Dash", 0.7f}, // Robert
    {"AleFunky", "Lead Developer", 0.5f},
    {"novex", "General Developer", 0.5f},
    {"Cloud54", "General Developer", 0.5f},
    {"advexed", "UI and Online", 0.5f},
    {"DiegoWearden", "240 Hz Input", 0.5f},
    {"orionconstel", "Concepts, Menus", 0.48f},
    {"StarryDawn72", "Decompilation", 0.5f},
    {"iliasHDZ", "Trigger Optimization", 0.5f},
    {"zylonity", "3D Support", 0.5f},
    {"camila314", "Pathfinder (Physics)", 0.5f},
    {"Crafty Jumper", "UI Assets", 0.5f},
    {"nittynatty", "VFX Tweaks", 0.5f},
    {"AleYoshi", "Tweaks", 0.5f},
};

void creditsInit(UIScreen *s) {
    UIList *list = (UIList *) uiGetElementByTag(s, "list");

    if (list) {
        float listWidth = list->base.w * 0.5f;

        for (size_t i = 0; i < ARRAY_LEN(credits); i++) {
            const char *contributor = credits[i].contributor;
            const char *contribution = credits[i].contribution;
            const float contributorScale = credits[i].nameScale;

            UIElement *card = (UIElement *) uiCreateRectangle(s);

            if (card) {
                uiRectangleSetColor((UIRectangle *) card, C2D_Color32(0,34,65,0));
                uiElementSetSize(card, 0, 17);

                // Contibutor name
                UILabel *name = uiCreateLabel(s);
                if (name) {
                    name->base.w = list->base.w - 12;
                    uiLabelSetText(name, contributor);
                    uiElementSetPosition((UIElement *) name, -listWidth + 3, 0);
                    uiElementSetScale((UIElement *) name, contributorScale);
                    
                    // name->font = 2;

                    uiElementAddChild(card, (UIElement *) name);
                }

                // Contribution name
                UILabel *description = uiCreateLabel(s);
                if (description) {
                    char text[256];
                    snprintf(text, sizeof(text) - 1, "- %s", contribution);
                    uiLabelSetText(description, text);
                    uiElementSetPosition((UIElement *) description, -listWidth + 8 + getTextLength(&bigFontFontCharset, contributorScale, true, contributor), 0);
                    uiElementSetScale((UIElement *) description, 0.7f);
                    
                    description->font = 1;

                    uiElementAddChild(card, (UIElement *) description);
                }

                uiListAdd(list, card);
            }
        }
    }
}

extern const UIScreenDefPair creditsDef = {
    .name = "credits",
    .top = {
        0
    },
    .btm = {
        .path = "romfs:/menus/settings_hub/credits.txt",
        .init = creditsInit
    }
};

}