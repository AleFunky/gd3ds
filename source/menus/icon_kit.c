#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_icon.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "icon_kit.h"
#include "level/main_levels.h"

#include "palette_kit.h"

#include "save/config.h"

static UIScreen screen_top;
static UIScreen screen;

static UIElement *bg_gradient = NULL;
static UIElement *bg_gradient_top = NULL;

static UIElement *bg_window = NULL;

static bool exit_flag = false;

static int gamemode_page = 0;

// Page handling 

static int current_cube_page = 0;
static int current_ship_page = 0;
static int current_ball_page = 0;
static int current_ufo_page  = 0;
static int current_wave_page = 0;

int selected_cube = 1;
int selected_ship = 1;
int selected_ball = 1;
int selected_ufo  = 1;
int selected_wave = 1;

int selected_p1 = 0;
int selected_p2 = 0;
int selected_glow = 0;

bool player_glow_enabled = false;

static const int gamemode_icon_count[GAMEMODE_COUNT] = {
    ICON_COUNT_PLAYER,
    ICON_COUNT_SHIP,
    ICON_COUNT_PLAYER_BALL,
    ICON_COUNT_BIRD,
    ICON_COUNT_DART
};

static int *current_pages[GAMEMODE_COUNT] = {
    &current_cube_page,
    &current_ship_page,
    &current_ball_page,
    &current_ufo_page,
    &current_wave_page
};

int *current_icons[GAMEMODE_COUNT] = {
    &selected_cube,
    &selected_ship,
    &selected_ball,
    &selected_ufo,
    &selected_wave
};

int *current_colors[3] = {
    &selected_p1,
    &selected_p2,
    &selected_glow
};


static const int button_images[5] = {
    341,
    351,
    325,
    327,
    329
};
static int icon_counter = 1;

static bool in_palette_kit = false;

static void set_icon_index(UIElement *e) {
    int new_index = (*current_pages[gamemode_page] * ICONS_PER_PAGE) + icon_counter;
    if (new_index < gamemode_icon_count[gamemode_page]) {
        e->enabled = true;
        ui_icon_set_gamemode_index(e, gamemode_page, new_index);
        icon_counter++;
    } else {
        e->enabled = false;
    }
}

static void disable_all_icon_buttons() {
    icon_counter = 1;
    ui_button_set_image(ui_get_element_by_tag(&screen, "cube"), button_images[0], 0);
    ui_button_set_image(ui_get_element_by_tag(&screen, "ship"), button_images[1], 0);
    ui_button_set_image(ui_get_element_by_tag(&screen, "ball"), button_images[2], 0);
    ui_button_set_image(ui_get_element_by_tag(&screen, "ufo"),  button_images[3], 0);
    ui_button_set_image(ui_get_element_by_tag(&screen, "dart"), button_images[4], 0);
}

static void set_cube_page(UIElement *e) {
    disable_all_icon_buttons();
    ui_button_set_image(e, button_images[0] + 1, 0);
    gamemode_page = 0;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void set_ship_page(UIElement *e) {
    disable_all_icon_buttons();
    ui_button_set_image(e, button_images[1] + 1, 0);
    gamemode_page = 1;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void set_ball_page(UIElement *e) {
    disable_all_icon_buttons();
    ui_button_set_image(e, button_images[2] + 1, 0);
    gamemode_page = 2;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void set_ufo_page(UIElement *e) {
    disable_all_icon_buttons();
    ui_button_set_image(e, button_images[3] + 1, 0);
    gamemode_page = 3;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void set_wave_page(UIElement *e) {
    disable_all_icon_buttons();
    ui_button_set_image(e, button_images[4] + 1, 0);
    gamemode_page = 4;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void action_exit(UIElement* e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void move_index_left(UIElement* e) {
    *current_pages[gamemode_page] -= 1;
    if (*current_pages[gamemode_page] < 0) {
        *current_pages[gamemode_page] = (gamemode_icon_count[gamemode_page] - 2) / ICONS_PER_PAGE;
    }
    icon_counter = 1;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void move_index_right(UIElement* e) {
    *current_pages[gamemode_page] += 1;
    if ((*current_pages[gamemode_page] * ICONS_PER_PAGE) + 1 >= gamemode_icon_count[gamemode_page]) {
        *current_pages[gamemode_page] = 0;
    }
    icon_counter = 1;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void action_icon_selected(UIElement *e) {
    *current_icons[e->icon.gamemode] = e->icon.index;
    icon_counter = 1;
    ui_run_func_on_tag(&screen, "icon", set_icon_index); 
}

static void action_open_palette_kit(UIElement* e) {
    in_palette_kit = true;
    palette_kit_init();
}

static UIAction actions[] = {
    {"exit", action_exit},
    {"action_cube", set_cube_page },
    {"action_ship", set_ship_page },
    {"action_ball", set_ball_page },
    {"action_ufo",  set_ufo_page },
    {"action_dart", set_wave_page },
    {"icons_left", move_index_left },
    {"icons_right", move_index_right },
    {"icon_selected", action_icon_selected },
    {"palette", action_open_palette_kit }
};

static UIAction actions_top[] = {

};

void icon_kit_loop() {
    exit_flag = false;
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/icon_kit.txt");
    ui_load_screen(&screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/icon_kit_top.txt");
    
    // Set bg color
    bg_gradient = ui_get_element_by_tag(&screen, "gradient");
    bg_window = ui_get_element_by_tag(&screen, "bg_window");

    bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient");
    
    ui_image_set_tint(bg_gradient, C2D_Color32(167, 167, 167, 255));
    ui_window_set_tint(bg_window, C2D_Color32(0, 0, 0, 64));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(167, 167, 167, 255));

    set_cube_page(ui_get_element_by_tag(&screen, "cube"));

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if ((kDown & KEY_B) && !in_palette_kit) {
            action_exit(NULL);
        }

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        if (!in_palette_kit) ui_screen_update(&screen, &touch);
        ui_screen_update(&screen_top, &touch);
        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&screen);
            if (in_palette_kit) {
                int returned = palette_kit_loop();
                if (returned) {
                    in_palette_kit = false;
                }
            }

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);

            ui_screen_draw(&screen_top);
            spawn_icon_at(
                gamemode_page, *current_icons[gamemode_page], player_glow_enabled, 200, 120, 0, 0, 0, 2.f,
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );

            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            cfg_save();
            game_state = STATE_MAIN_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}