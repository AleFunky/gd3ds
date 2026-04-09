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
#include "menus/components/ui_label_aller.h"
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
#include "external_levels.h"

#include "gameplay.h"

#include "save/config.h"
#include "utils/folders.h"
#include "level_loading.h"

static bool exit_flag = false;
static bool start_level = false;

static bool first_time_loaded = true;

static UIScreen screen;
static UIScreen screen_top;
static UIElement *bg_gradient;
static UIElement *bg_gradient_top;
static UIElement *list;
static UIElement *path_label;

UIElement texts[UI_LIST_MAX_ITEMS];

char current_path[256] = { 0 };
char last_path[256] = { 1 };

static void open_folder(UIElement *e);

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void open_level(UIElement *e) {
    play_sfx(&play_sound, 1);

    state.custom_level = true;
    strncpy(state.custom_level_path, e->external_level_card.path, 256);

    set_fade_status(FADE_STATUS_OUT);
    start_level = true; 
}

void load_level_folder(char *folder) {
    if (strncmp(last_path, current_path, 256) == 0) return;
    ui_list_reset(list);
    path_label = ui_get_element_by_tag(&screen, "path");
    char path[256];
    strncpy(path, current_path, 256);
    truncate_filename(path, 30);
    strncpy(path_label->label_aller.text, path, 256);
    int count = 0;
    FileOrFolder *entries = load_folder(folder, &count);
    char level_name[256];
    if (entries && list) {
        for (int i = 0; i < count && i < UI_LIST_MAX_ITEMS; i++) {
            FileOrFolder *entry = &entries[i];
            if (entry->is_dir) {
                // Folder
                char *name = strip_filename(entry->name);
                truncate_filename(name, 16);
                texts[i] = ui_create_external_level_card(0, 0, 320, 0, name, entry->name, open_folder, NULL);
                ui_list_add(list, &texts[i]);
            } else {
                strncpy(level_name, entry->name, 255);
                strip_extension(level_name);
                char *name = strip_filename(level_name);
                truncate_filename(name, 16);
                texts[i] = ui_create_external_level_card(0, 0, 420, 0, name, entry->name, open_level, NULL);
                ui_list_add(list, &texts[i]);
            }
        }
    } else {
        texts[0] = ui_create_label(0, 0, 0.5f, "No levels", 0.5f, NULL);
        ui_list_add(list, &texts[0]);
    }

    strncpy(last_path, current_path, 256);
}

static void action_go_back(UIElement *e) {
    if (strlen(current_path) > 0) {
        go_back_directory(current_path);
        load_level_folder(current_path);
    }
}

static void open_folder(UIElement *e) {
    strncpy(current_path, e->external_level_card.path, 256);
    load_level_folder(current_path);

}

static UIAction actions[] = {
    {"exit", action_exit },
    {"go_back", action_go_back },
};

void external_levels_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(bot);
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    draw_text(bigFont_fontCharset, bigFont_sheet, 310, SCREEN_HEIGHT - 10, 0.5f, 1.0f, "Loading...");
    C3D_FrameEnd(0);

    start_level = false;
    exit_flag = false;
    if (first_time_loaded) {
        ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_levels.txt");
        bg_gradient = ui_get_element_by_tag(&screen, "gradient");
        ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_levels_top.txt");
        bg_gradient_top = ui_get_element_by_tag(&screen_top, "gradient_top");
        list = ui_get_element_by_tag(&screen, "list");
        first_time_loaded = false;
    }

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    load_level_folder(current_path);

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

        ui_screen_update(&screen, &touch);
        
        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&screen);

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

        if (start_level) {
            stop_mp3();
            game_state = STATE_GAME;
            playing_menu_loop = false;
            break;
        }

        if (exit_flag) {
            game_state = STATE_MAIN_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}