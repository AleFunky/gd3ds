#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_image.h"
#include "main.h"
#include "graphics.h"

#include "generic_disclaimer.h"
#include "search_menu.h"
// #include "server_switcher.h"
// #include "filters_menu.h"

static bool in_disclaimer = false;
static bool in_server_switcher = false;
static bool in_filters = false;
static bool exit_flag = false;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

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
    // in_filters = true;
    // filters_init();
}

void action_clear_filters(UIElement* e) {
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"disclaimer", action_open_disclaimer },
    {"serverswitcher", action_open_server_switcher },
    {"openfilters", action_open_filters },
    {"clearfilters", action_clear_filters },
};

void search_menu_loop() {

    exit_flag = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu_top.txt");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

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

        if (!in_disclaimer) ui_screen_update(&default_screen, &touch);
        
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

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();

            ui_screen_draw(&default_screen_top);
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
