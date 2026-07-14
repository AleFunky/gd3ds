#include <3ds.h>
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
#include "menus/external_level_infobox.h"
#include "menus/online_level_comments.h"

static bool exit_flag = false;
static bool in_info_box = false;
static bool in_comments = false;


static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_info(UIElement *e) {
    // in_info_box = true;
    // external_level_infobox_init();
}

static void action_open_comments(UIElement *e) {
    in_comments = true;
    online_comments_init();
}


static UIAction actions[] = {
    {"exit", action_exit },
    {"info", action_open_info },
    {"comments", action_open_comments },
};

void online_menu_loop() {
    exit_flag = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu.txt");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu_top.txt");

    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
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

        if (!in_info_box && !in_comments) ui_screen_update(&default_screen, &touch);
        
        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&default_screen);
            if (in_comments) online_comments_draw_bot();

            change_blending(true);
            draw_touch_effect();
            change_blending(false);
            
            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();
            
            ui_screen_draw(&default_screen_top);
            if (in_comments) online_comments_draw_top();

            C2D_ViewReset();
            C3D_FrameEnd(0);


        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_SAVED_LEVELS;
            break;
        }

        if (in_info_box)
        {
            int returned = external_level_infobox_loop();
            if (returned)
            {
                in_info_box = false;
            }
        }

        if (in_comments)
        {
            int returned = online_comments_loop();
            if (returned)
            {
                in_comments = false;
            }
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
