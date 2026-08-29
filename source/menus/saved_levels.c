#include <3ds.h>
#include <stdlib.h>
#include <citro2d.h>
#include "menus/components/ui_window_button.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_rectangle.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"
#include "utils/folders.h"
#include "external_popup.h"
#include "utils/server_utils.h"
#include "utils/string_helpers.h"
#include "fonts/goldFont.h"

static bool exit_flag = false;

static int new_state;

static UILabel *list_title;
static UILabel *top_title;

static UILabel *error_label;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UIList *list;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_level_menu(UIElement* e) {
    new_state = STATE_ONLINE_LEVEL;
    set_fade_status(FADE_STATUS_OUT);
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"open_level_menu", action_open_level_menu },
};

void saved_levels_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(bot);
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    // Nothing to draw up there, just clear every eye
    for (int eye = 0; begin_top_eye(eye); eye++) { }
    C3D_FrameEnd(0);

    new_state = 0;
    exit_flag = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_levels.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_levels_top.txt");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    list_title = (UILabel *) ui_get_element_by_tag(&default_screen, "listtitle");
    ui_label_set_text(list_title, "Saved Levels");
    top_title = (UILabel *) ui_get_element_by_tag(&default_screen_top, "toptitle");
    ui_label_set_text(top_title, "Browse your saved user levels!");

    list = (UIList *) ui_get_element_by_tag(&default_screen, "list");
    error_label = (UILabel *)ui_get_element_by_tag(&default_screen, "errorLabel");

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3(menu_loop_path, true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        ui_screen_update(&default_screen, &touch);
        
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

        if (new_state) {
            game_state = new_state;
            break;
        }

        if (exit_flag) {
            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
