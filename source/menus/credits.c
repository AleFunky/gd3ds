#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_label.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "credits.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true,
};

static UIList *list;

typedef struct CreditsEntries {
    char *contributor;
    char *contribution;
    float name_scale;
} CreditsEntries;

static const CreditsEntries credits[] = {
    { "<i400s0>", "Geometry Dash", 0.7f }, // Robert
    { "AleFunky", "Lead Developer", 0.5f },
    { "camila314", "Pathfinder (Physics)", 0.5f },
    { "advexed", "Menus and VFX", 0.5f },
    { "nittynatty", "VFX", 0.5f }, 
    { "orionconstel", "Concepts, Menus", 0.5f },
    { "Cloud54", "Menus", 0.5f },
    { "novex", "Optimization", 0.5f },
    { "DiegoWearden", "240hz input", 0.5f },
    { "zylonity", "3D Support", 0.5f },
};

void exit_credits(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_credits },
};

void credits_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/credits.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    yes_exit = false;

    list = (UIList *) ui_get_element_by_tag(&screen, "list");

    if (list) {
        float list_width = list->base.w * 0.5f;

        for (int i = 0; i < ARRAY_LEN(credits); i++) {
            char *contributor = credits[i].contributor;
            char *contribution = credits[i].contribution;
            float contributor_scale = credits[i].name_scale;

            UIElement *card = (UIElement *) ui_create_rectangle(&screen.ctx);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, C2D_Color32(0,34,65,0));
                ui_element_set_size(card, 0, 17);

                // Contibutor name
                UILabel *name = ui_create_label(&screen.ctx);
                if (name) {
                    name->base.w = list->base.w - 12;
                    ui_label_set_text(name, contributor);
                    ui_element_set_position((UIElement *) name, -list_width + 3, 0);
                    ui_element_set_scale((UIElement *) name, contributor_scale);
                    
                    // name->font = 2;

                    ui_element_add_child(card, (UIElement *) name);
                }

                // Contribution name
                UILabel *description = ui_create_label(&screen.ctx);
                if (description) {
                    char text[256];
                    snprintf(text, sizeof(text) - 1, "- %s", contribution);
                    ui_label_set_text(description, text);
                    ui_element_set_position((UIElement *) description, -list_width + 8 + get_text_length(&bigFont_fontCharset, contributor_scale, true, contributor), -1);
                    ui_element_set_scale((UIElement *) description, 0.7f);
                    
                    description->font = 1;

                    ui_element_add_child(card, (UIElement *) description);
                }

                ui_list_add(list, card);
            }
        }
    }
}

int credits_loop() {
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

    ui_screen_draw(&screen);

    return false;
}