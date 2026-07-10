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


static bool exit_flag = false;

static int new_state;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UIList *list;

typedef struct SavedLevelsEntries {
    char *name;
    char *creator;
    char *song;
    char *length;
    int downloads;
    int likes;
    int stars;
} SavedLevelsEntries;


static const SavedLevelsEntries saved_levels[] = {
    { "Level 1", "Creator", "<#f982ff>Song", "Long", 111, 123, 10 },
    { "hi i am a longer levelname", "superlongcreator", "<#f982ff>Lalalalalala", "Long", 111, 123, 9 },
    { "revolution", "funnygame", "<#f982ff>Something Something Something", "Long", 111, 123, 2 },
};

UIElement entries1[ARRAY_LEN(saved_levels)];

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
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    C3D_FrameEnd(0);

    new_state = 0;
    exit_flag = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/saved_levels.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/saved_levels_top.txt");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    list = (UIList *) ui_get_element_by_tag(&default_screen, "list");

    if (list) {
        for (int i = 0; i < ARRAY_LEN(saved_levels); i++) {
            char name[256];
            char creator[256];
            char song[256];
            char length[256];
            int downloads = saved_levels[i].downloads;
            int likes = saved_levels[i].likes;
            int stars = saved_levels[i].stars;

            strncpy(name, saved_levels[i].name, sizeof(name) - 1);
            strncpy(creator, saved_levels[i].creator, sizeof(creator) - 1);
            strncpy(song, saved_levels[i].song, sizeof(song) - 1);
            strncpy(length, saved_levels[i].length, sizeof(length) - 1);

            
            truncate_filename(name, 13);
            truncate_filename(creator, 20);
            truncate_filename(song, 25);

            float list_width = list->base.w * 0.5f;

            UIElement *card = (UIElement *) ui_create_rectangle(&default_screen.ctx);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
                ui_element_set_size(card, 0, 60);

                // Level name
                UILabel *name_label = ui_create_label(&default_screen.ctx);
                if (name_label) {
                    ui_label_set_text(name_label, name);
                    ui_element_set_position((UIElement *) name_label, -list_width + 50, -17);
                    ui_element_set_scale((UIElement *) name_label, 0.54f);

                    ui_element_add_child(card, (UIElement *) name_label);
                }

                // Level creator
                UILabel *creator_label = ui_create_label(&default_screen.ctx);
                if (creator_label) {
                    ui_label_set_text(creator_label, creator);
                    ui_element_set_position((UIElement *) creator_label, -list_width + 50, -4.5f);
                    ui_element_set_scale((UIElement *) creator_label, 0.45f);

                    creator_label->font = 2;

                    ui_element_add_child(card, (UIElement *) creator_label);
                }

                // Level song
                UILabel *song_label = ui_create_label(&default_screen.ctx);
                if (song_label) {
                    ui_label_set_text(song_label, song);
                    ui_element_set_position((UIElement *) song_label, -list_width + 50, 7);
                    ui_element_set_scale((UIElement *) song_label, 0.35f);

                    ui_element_add_child(card, (UIElement *) song_label);
                }

                // Level song
                UILabel *length_label = ui_create_label(&default_screen.ctx);
                if (length_label) {
                    ui_label_set_text(length_label, length);
                    ui_element_set_position((UIElement *) length_label, -list_width + 62, 19);
                    ui_element_set_scale((UIElement *) length_label, 0.35f);

                    ui_element_add_child(card, (UIElement *) length_label);
                }

                // Downloads
                UILabel *download_value = ui_create_label(&default_screen.ctx);
                if (download_value) {
                    char tmp_value[16];

                    snprintf(tmp_value, sizeof(tmp_value), "%d", downloads);

                    ui_label_set_text(download_value, tmp_value);
                    ui_element_set_position((UIElement *) download_value, -list_width + 112, 19);
                    ui_element_set_scale((UIElement *) download_value, 0.35f);
                    
                    ui_element_add_child(card, (UIElement *) download_value);
                }

                // Likes
                UILabel *like_value = ui_create_label(&default_screen.ctx);
                if (like_value) {
                    char tmp_value[16];

                    snprintf(tmp_value, sizeof(tmp_value), "%d", likes);

                    ui_label_set_text(like_value, tmp_value);
                    ui_element_set_position((UIElement *) like_value, -list_width + 162, 19);
                    ui_element_set_scale((UIElement *) like_value, 0.35f);
                    
                    ui_element_add_child(card, (UIElement *) like_value);
                }

                // Stars
                UILabel *star_value = ui_create_label(&default_screen.ctx);
                if (star_value) {
                    char tmp_value[16];

                    snprintf(tmp_value, sizeof(tmp_value), "%d", stars);

                    ui_label_set_text(star_value, tmp_value);
                    ui_element_set_position((UIElement *) star_value, -list_width + 23, 20);
                    ui_element_set_scale((UIElement *) star_value, 0.35f);
                    
                    star_value->alignment = 1.f;
                    
                    ui_element_add_child(card, (UIElement *) star_value);
                }

                UIImage *difficulty_face = ui_create_image(&default_screen.ctx);
                if (difficulty_face) {
                    ui_image_set_image(difficulty_face, 258, 0);
                    ui_element_set_position((UIElement *) difficulty_face, -list_width + 24, -4);
                    ui_element_set_scale((UIElement *) difficulty_face, 0.82f);

                    ui_element_add_child(card, (UIElement *) difficulty_face);
                }

                UIImage *star_icon = ui_create_image(&default_screen.ctx);
                if (star_icon) {
                    ui_image_set_image(star_icon, 170, 0);
                    ui_element_set_position((UIElement *) star_icon, -list_width + 30, 20);
                    ui_element_set_scale((UIElement *) star_icon, 0.71f);

                    ui_element_add_child(card, (UIElement *) star_icon);
                }

                UIImage *length_icon = ui_create_image(&default_screen.ctx);
                if (length_icon) {
                    ui_image_set_image(length_icon, 197, 0);
                    ui_element_set_position((UIElement *) length_icon, -list_width + 55, 20);
                    ui_element_set_scale((UIElement *) length_icon, 0.5f);

                    ui_element_add_child(card, (UIElement *) length_icon);
                }

                UIImage *download_icon = ui_create_image(&default_screen.ctx);
                if (download_icon) {
                    ui_image_set_image(download_icon, 163, 0);
                    ui_element_set_position((UIElement *) download_icon, -list_width + 105, 20);
                    ui_element_set_scale((UIElement *) download_icon, 0.7f);

                    ui_element_add_child(card, (UIElement *) download_icon);
                }

                UIImage *like_icon = ui_create_image(&default_screen.ctx);
                if (like_icon) {
                    ui_image_set_image(like_icon, 166, 0);
                    ui_element_set_position((UIElement *) like_icon, -list_width + 155, 19);
                    ui_element_set_scale((UIElement *) like_icon, 0.7f);

                    ui_element_add_child(card, (UIElement *) like_icon);
                }

                UIWindowButton *button = ui_create_window_button(&default_screen.ctx);
                if (button) {
                    ui_window_button_set_style(button, 5);
                    ui_button_set_text((UIButton *) button, "View");

                    button->base.textScale = 0.48f;

                    ui_element_set_position((UIElement *) button, list_width - 32, 0);
                    ui_element_set_size((UIElement *) button, 48, 28);
                    ui_element_set_action((UIElement *) button, action_open_level_menu);
                    ui_element_add_child(card, (UIElement *) button);
                }

                ui_list_add(list, card);
            }
        }
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
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

            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();

            ui_screen_draw(&default_screen_top);
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
