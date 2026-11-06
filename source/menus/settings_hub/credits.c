#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_label.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "menus/settings_hub/credits.h"

typedef struct CreditsEntries {
    char *contributor;
    char *contribution;
    float name_scale;
} CreditsEntries;

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

void credits_init(UIScreen *s) {
    UIList *list = (UIList *) ui_get_element_by_tag(s, "list");

    if (list) {
        float list_width = list->base.w * 0.5f;

        for (int i = 0; i < ARRAY_LEN(credits); i++) {
            char *contributor = credits[i].contributor;
            char *contribution = credits[i].contribution;
            float contributor_scale = credits[i].name_scale;

            UIElement *card = (UIElement *) ui_create_rectangle(s);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, C2D_Color32(0,34,65,0));
                ui_element_set_size(card, 0, 17);

                // Contibutor name
                UILabel *name = ui_create_label(s);
                if (name) {
                    name->base.w = list->base.w - 12;
                    ui_label_set_text(name, contributor);
                    ui_element_set_position((UIElement *) name, -list_width + 3, 0);
                    ui_element_set_scale((UIElement *) name, contributor_scale);
                    
                    // name->font = 2;

                    ui_element_add_child(card, (UIElement *) name);
                }

                // Contribution name
                UILabel *description = ui_create_label(s);
                if (description) {
                    char text[256];
                    snprintf(text, sizeof(text) - 1, "- %s", contribution);
                    ui_label_set_text(description, text);
                    ui_element_set_position((UIElement *) description, -list_width + 8 + get_text_length(&bigFont_fontCharset, contributor_scale, true, contributor), 0);
                    ui_element_set_scale((UIElement *) description, 0.7f);
                    
                    description->font = 1;

                    ui_element_add_child(card, (UIElement *) description);
                }

                ui_list_add(list, card);
            }
        }
    }
}

const UIScreenDefPair credits_def = {
    .name = "credits",
    .top = {
        0
    },
    .btm = {
        .path = "romfs:/menus/settings_hub/credits.txt",
        .init = credits_init
    }
};
