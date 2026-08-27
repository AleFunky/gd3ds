#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_button.h"
#include "menus/core/common_setters.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_rectangle.h"
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

#include "menus/external_popup.h"
#include "menus/info_card.h"

const char *error_strings[] = {
    "Invalid gmd.",
    "Invalid level data.",
    "Level string missing sections.",
    "Out of memory.",
    "Couldn't parse objects."
};

static bool exit_flag = false;
bool external_start_level = false;

static bool first_time_loaded = true;

static bool in_external_popup = false;

static bool reload_pending;
static char reload_path[320];

static UIScreen screen = {
    .isBottom = true
};
static UIScreen screen_top;
static UIImage *bg_gradient;
static UIImage *bg_gradient_top;
static UIList *list;
static UILabel *path_label;

char current_path[320] = { 0 };
char last_path[320] = { 1 };

typedef struct {
    char path[256];
    bool is_dir;
} LevelCardData;

static void open_folder(UIElement *e);

static void action_exit(UIElement *e) {
    exit_flag = true;
    current_path[0] = '\0'; // Reset it
    set_fade_status(FADE_STATUS_OUT);
}

static void open_external_popup(UIElement *e) {
    LevelCardData *entry = e->userdata;
    strcpy(state.custom_level_path, entry->path);
    external_popup_init();
    in_external_popup = true;
}

void load_level_folder(char *folder) {
    if (strncmp(last_path, current_path, sizeof(last_path)) == 0) return;
    path_label = (UILabel *) ui_get_element_by_tag(&screen, "path");
    ui_run_func_on_tag(&screen, "no_levels", ui_disable_element);

    char path[320+5];
    sprintf(path, "Root/%s", current_path);
    truncate_filename_start(path, 27, sizeof(path));
    
    ui_label_set_text(path_label,path);

    int count = 0;
    FileOrFolder *entries = load_folder(folder, &count);
    char level_name[256];
    
    ui_list_reset(list);
    
    if (entries && list) {
        for (int i = 0; i < count; i++) {
            FileOrFolder *entry = &entries[i];
            strncpy(level_name, entry->name, sizeof(level_name) - 1);

            UIElement *card = NULL;

            u32 color = (i & 1 ? C2D_Color32(50,50,50,255) :  C2D_Color32(75,75,75,255));
            char *name = NULL;
            if (entry->is_dir) {
                // Folder
                name = strip_filename(level_name);
                truncate_filename(name, 16);
            } else {
                // File
                strip_extension(level_name);
                name = strip_filename(level_name);
                truncate_filename(name, 16);
            }

            float list_width = list->base.w * 0.5f;

            card = (UIElement *) ui_create_rectangle(&screen.ctx);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, color);
                ui_element_set_size(card, 0, 28);
                
                UIButton *button = ui_create_button(&screen.ctx);
                if (button) {
                    // Store in the user data
                    LevelCardData *data = malloc(sizeof(*data));

                    strcpy(data->path, entry->name);
                    data->is_dir = entry->is_dir;

                    ui_element_set_userdata((UIElement *) button, data);

                    ui_button_set_image(button, (entry->is_dir ? 7 : 6), 0);
                    ui_element_set_position((UIElement *) button, list_width - 15, 0);
                    ui_element_set_scale_xy((UIElement *) button, -0.5f, 0.5f);
                    ui_element_set_action((UIElement *) button, (entry->is_dir ? open_folder : open_external_popup));

                    ui_element_add_child(card, (UIElement *) button);
                }

                UIImage *icon = ui_create_image(&screen.ctx);
                if (icon) {
                    ui_image_set_image(icon, (entry->is_dir ? 320 : 420), 0);
                    ui_element_set_position((UIElement *) icon, -list_width + 15, 0);
                    ui_element_set_scale((UIElement *) icon, 0.58f);

                    ui_element_add_child(card, (UIElement *) icon);
                }

                // Name
                UILabel *label = ui_create_label(&screen.ctx);
                if (label) {
                    ui_label_set_text(label, name);
                    ui_element_set_position((UIElement *) label, -list_width + 29, 1);
                    ui_element_set_scale((UIElement *) label, 0.54f);
                    
                    ui_element_add_child(card, (UIElement *) label);
                }

                ui_list_add(list, card);
            }
        }
        
        if (count == 0) {
            ui_run_func_on_tag(&screen, "no_levels", ui_enable_element);
        }
    } else {
        ui_run_func_on_tag(&screen, "no_levels", ui_enable_element);
    }

    strncpy(last_path, current_path, sizeof(last_path));
}

static void action_go_back(UIElement *e) {
    if (strlen(current_path) > 0) {
        go_back_directory(current_path);
        load_level_folder(current_path);
    }
}

// Go my warning suppresion gadget
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

static void open_folder(UIElement *e) {
    LevelCardData *entry = e->userdata;
    char tmp[320];

    if (current_path[0] == '\0') {
        // First level: no leading slash
        snprintf(tmp, sizeof(tmp), "%s", entry->path);
    } else {
        snprintf(tmp, sizeof(tmp), "%s/%s", current_path, entry->path);
    }

    strncpy(current_path, tmp, sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';

    reload_pending = true;
    strcpy(reload_path, current_path);
}

#pragma GCC diagnostic pop

static UIAction actions[] = {
    {"exit", action_exit },
    {"go_back", action_go_back },
};

static void show_error_message() {
    // Level gave error
    char tmp[512];

    int message_id = level_result - 1;
    char *message = "Ultra unknown error.";
    if (IN_BOUNDS(message_id, error_strings)) {
        message = (char *) error_strings[message_id]; 
    }

    snprintf(tmp, sizeof(tmp), "<red>ERROR</>:\n%s", message);

    info_card_init();
    set_info_content(tmp);

    in_info_card = true;
    level_result = 0;
}

void external_levels_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(bot);
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    for (int eye = 0; begin_top_eye(eye); eye++) {
        begin_eye_layer(DEPTH_UI);
        draw_text(&bigFont_fontCharset, &bigFont_sheet, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 0.5f, 0.5f, 1.0f, true, "Loading...");
        end_eye_layer();
    }
    C3D_FrameEnd(0);

    external_start_level = false;
    exit_flag = false;
    if (first_time_loaded) {
        ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_levels.txt");
        bg_gradient = (UIImage *) ui_get_element_by_tag(&screen, "gradient");
        ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_levels_top.txt");
        bg_gradient_top = (UIImage *) ui_get_element_by_tag(&screen_top, "gradient_top");
        list = (UIList *) ui_get_element_by_tag(&screen, "list");
        first_time_loaded = false;
    }

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    load_level_folder(current_path);

    if (level_result) {
        show_error_message();
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();
        /*
        if ((kDown & KEY_B) && !in_external_popup) {
            action_exit(NULL);
        }
        */

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        if (reload_pending) {
            load_level_folder(reload_path);
            reload_pending = false;
        }

        if (!in_external_popup) ui_screen_update(&screen, &touch);

        if (in_external_popup) {
            int returned = external_popup_loop();
            if (returned) {
                in_external_popup = false;
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

            ui_screen_draw(&screen);
            
            if (in_external_popup) external_popup_draw_bot();


            if (in_info_card) {
                int returned = info_card_loop();
                if (returned) {
                    in_info_card = false;
                }
            }

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            // Top screen, drawn once per eye when 3D is on
            for (int eye = 0; begin_top_eye(eye); eye++) {
                draw_fade();

                begin_eye_layer(DEPTH_UI);
                ui_screen_draw(&screen_top);
                end_eye_layer();

                begin_eye_layer(DEPTH_POPUP);
                if (in_external_popup) external_popup_draw_top();
                end_eye_layer();
            }
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (external_start_level) {
            stop_mp3();
            game_state = STATE_GAME;
            playing_menu_loop = false;
            break;
        }

        if (exit_flag) {
            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}
