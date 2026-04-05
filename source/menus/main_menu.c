#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"

#include "main_menu.h"
#include "level_select.h"
#include "settings.h"
#include "external_levels.h"

static UIScreen screen_top;
static UIScreen screen;

static int main_menu_color_index = 0;

static int new_state = 0;
static bool exit_flag = false;

static bool in_settings = false;

static float bg_scroll = 0;

void action_open_level_select(UIElement* e) {
    new_state = STATE_LEVEL_SELECT;
    set_fade_status(FADE_STATUS_OUT);
}
void action_open_extra_menu(UIElement* e) {
    new_state = STATE_EXTERNAL_LEVELS;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_icon_kit(UIElement* e) {
    new_state = STATE_ICON_KIT;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_settings(UIElement* e) {
    in_settings = true;
    settings_init();
}

static UIAction actions[] = {
    { "level_select", action_open_level_select },
    { "extra_menu", action_open_extra_menu },
    { "settings", action_open_settings },
    { "icon_kit", action_open_icon_kit },
};

static UIAction actions_top[] = {

};


void main_menu_loop() {
    exit_flag = false;
    new_state = 0;
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/main_menu.txt");
    ui_load_screen(&screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/main_menu_top.txt");
    
    main_menu_color_index = 0;
    u32 color = default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS];

    Color col;
    col.r = GET_R(color);
    col.g = GET_G(color);
    col.b = GET_B(color);

    channels[CHANNEL_BG].color = col;
    channels[CHANNEL_GROUND].color = col;
    channels[CHANNEL_LINE].color = white;

    UIElement *title = ui_get_element_by_tag(&screen_top, "title");

    if (title && alt_title_screen) {
        ui_image_set_image(title, 3, 1);
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    get_buffer(CHANNEL_BG)->active = false;
    get_buffer(CHANNEL_GROUND)->active = false;
    get_buffer(CHANNEL_LINE)->active = false;

    bool old_wide = wideEnabled;
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_SELECT) {
            game_state = STATE_EXIT;
            stop_mp3();
            break; // break in order to return to hbmenu
        }

        if (kDown & KEY_START) {
            action_open_level_select(NULL);
        }

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        handle_col_channel(CHANNEL_BG);
        handle_col_channel(CHANNEL_GROUND);

        ColTriggerBuffer *trig = get_buffer(CHANNEL_BG);
        if (!trig->active) {
            upload_color_to_buffer(CHANNEL_BG, default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS], 4.f);
            upload_color_to_buffer(CHANNEL_GROUND, default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS], 4.f);
            main_menu_color_index++;
        }

        if (wideEnabled != old_wide) {        
            gspWaitForVBlank();
            set_wide(wideEnabled);
            gspWaitForVBlank();
            reinitialize_screens();
            old_wide = wideEnabled;
        }

        if (!in_settings) ui_screen_update(&screen, &touch);
        ui_screen_update(&screen_top, &touch);
        do {
            update_touch_effect(DT);
            bg_scroll += 5.19300155f;
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();

            draw_background(-40 + (bg_scroll / 8), 0);
            ui_screen_draw(&screen_top);

            // Bottom Screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);

            draw_background(bg_scroll / 8, SCREEN_HEIGHT);
            C2D_ViewScale(SCALE, SCALE);
            draw_ground(bg_scroll, 0, 0, false, 320);

            C2D_ViewScale(1/SCALE, 1/SCALE);

            ui_screen_draw(&screen);
            if (in_settings) {
                int returned = settings_loop();
                if (returned) {
                    in_settings = false;
                }
            }
            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            C2D_ViewReset();

            C3D_FrameEnd(0);
        } while (handle_fading());

        if (new_state) {
            game_state = new_state;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}