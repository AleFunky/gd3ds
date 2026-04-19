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
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "state.h"

#include "generic_disclaimer.h"
#include "search_menu.h"

#include "gameplay.h"

#include "save/config.h"
#include "utils/folders.h"
#include "level_loading.h"

static bool in_disclaimer = false;
static bool exit_flag = false;

static UIScreen screen;
static UIScreen screen_top;
static UIElement *bg_gradient;
static UIElement *bg_gradient_top;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_disclaimer(UIElement* e) {
    in_disclaimer = true;
    disclaimer_init();
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"disclaimer", action_open_disclaimer }
};

void search_menu_loop() {

    exit_flag = false;

    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu.txt");
    bg_gradient = ui_get_element_by_tag(&screen, "gradient");
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu_top.txt");
    bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));


    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_B) {
            action_exit(NULL);
        }

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        if (!in_disclaimer) ui_screen_update(&screen, &touch);
        
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
            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}
