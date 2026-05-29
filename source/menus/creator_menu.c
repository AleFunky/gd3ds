#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_external_level_card.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "state.h"

#include "settings.h"
#include "saved_levels.h"
#include "external_levels.h"
#include "search_menu.h"
#include "creator_menu.h"
#include "generic_disclaimer.h"
#include "soggy.h"

#include "gameplay.h"

#include "save/config.h"
#include "utils/folders.h"
#include "level_loading.h"

static int new_state = 0;
static bool exit_flag = false;
static bool in_disclaimer = false;

static UIScreen screen;
static UIScreen screen_top;
static UIElement *bg_gradient;
static UIElement *bg_gradient_top;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_external_menu(UIElement *e) {
    new_state = STATE_EXTERNAL_LEVELS;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_saved_menu(UIElement *e) {
    //disabled for release build
    // new_state = STATE_SAVED_LEVELS;
    // set_fade_status(FADE_STATUS_OUT);
    in_disclaimer = true;
    disclaimer_init();
}

static void action_open_search_menu(UIElement *e) {
    // disabled for release build
    // new_state = STATE_SEARCH_MENU;
    // set_fade_status(FADE_STATUS_OUT);
    in_disclaimer = true;
    disclaimer_init();
}

static void action_open_soggy_menu(UIElement *e) {
    new_state = STATE_SOGGY;
    set_fade_status(FADE_STATUS_OUT);
    playing_menu_loop = false;
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"external", action_open_external_menu},
    {"saved", action_open_saved_menu},
    {"search", action_open_search_menu},
    {"create", action_open_soggy_menu},
};

void creator_menu_loop() {
    exit_flag = false;
    new_state = STATE_CREATOR_MENU;

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/creator_menu.txt");
    bg_gradient = ui_get_element_by_tag(&screen, "gradient");
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/creator_menu_top.txt");
    bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    if (gotSogged) ui_button_set_image(ui_get_element_by_tag(&screen, "create_button"), 20, 1);

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;
        if (!in_disclaimer) {
            ui_screen_update(&screen, &touch);
            if (kDown & KEY_B) {
                action_exit(NULL);
            }
        }

        

        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&screen);

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

            ui_screen_draw(&screen_top);
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_MAIN_MENU;
            break;
        }

        if (new_state != STATE_CREATOR_MENU) {
            game_state = new_state;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}
