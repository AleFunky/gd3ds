#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_image.h"
#include "main.h"
#include "graphics.h"

#include "generic_disclaimer.h"
#include "search_menu.h"
// #include "server_switcher.h"
#include "search_filters.h"
#include "clear_search_filters.h"
#include "song_filter.h"
#include "menus/components/ui_label.h"
static int new_state = 0;

static bool in_disclaimer = false;
static bool in_server_switcher = false;
static bool in_filters = false;
static bool in_clear_search_filters = false;
static bool exit_flag = false;
bool search_needs_refresh = true;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;
static UITextbox *search_input;

SearchFilters search_filters;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_disclaimer(UIElement* e) {
    in_disclaimer = true;
    disclaimer_init();
}

void action_open_server_switcher(UIElement* e) {
    // in_server_switcher = true;
    // server_switcher_init();
}

void action_open_filters(UIElement* e) {
    in_filters = true;
    search_filters_init();
}

void action_clear_filters(UIElement* e) {
    in_clear_search_filters = true;
    clear_search_filters_init();
}

void action_search(UIElement* e) {
    search_filters.searchType = ui_prop_int(&e->custom_properties, "type", 0);
    strncpy(search_filters.searchQuery, search_input->text, sizeof(search_filters.searchQuery) - 1);
    search_filters.currentPage = 0;
    search_needs_refresh = true;
    new_state = STATE_ONLINE;
    set_fade_status(FADE_STATUS_OUT);
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"disclaimer", action_open_disclaimer },
    {"serverswitcher", action_open_server_switcher },
    {"openfilters", action_open_filters },
    {"clearfilters", action_clear_filters },
    {"search", action_search }
};

void search_menu_loop() {

    exit_flag = false;
    new_state = STATE_SEARCH_MENU;
    
    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu_top.txt");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");
    search_input = (UITextbox *) ui_get_element_by_tag(&default_screen, "searchbox");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));


    set_fade_status(FADE_STATUS_IN);

    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        if (!in_disclaimer && !in_server_switcher && !in_clear_search_filters && !in_filters) ui_screen_update(&default_screen, &touch);

        if (in_filters) {
            int returned = search_filters_loop();
            if (returned) {
                in_filters = false;
            }
        }
        

        // Frees a render target, so keep it out of the frame below
        update_stereo_target();

        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&default_screen);
            
            if (in_disclaimer) {
                int returned = disclaimer_loop();
                if (returned) {
                    in_disclaimer = false;
                }
            }

            if (in_clear_search_filters) {
                int returned = clear_search_filters_loop();
                if (returned) {
                    in_clear_search_filters = false;
                }
            }
            
            if (in_filters) search_filters_draw();

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            // Top screen, drawn once per eye when 3D is on
            for (int eye = 0; begin_top_eye(eye); eye++) {
                draw_fade();

                begin_eye_layer(DEPTH_UI);
                ui_screen_draw(&default_screen_top);
                end_eye_layer();
            }
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_CREATOR_MENU;
            break;
        }

        if (new_state != STATE_SEARCH_MENU) {
            game_state = new_state;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
