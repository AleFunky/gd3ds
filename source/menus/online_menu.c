#include <3ds.h>
#include <stdlib.h>
#include <citro2d.h>
#include "level/main_levels.h"
#include "level_loading.h"
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
#include "search_menu.h"
#include "songs.h"

static bool exit_flag = false;

static int new_state;
int curr_search_id;

static UILabel *error_label;
static UILabel *page_info_label;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UIList *list;

typedef struct {
    int entryId;
} OnlineCardData;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_online_level_menu(UIElement* e) {
    OnlineCardData *entry = e->userdata;
    curr_search_id = entry->entryId;
    new_state = STATE_ONLINE_LEVEL;
    set_fade_status(FADE_STATUS_OUT);
}

static void update_arrows() {
    if (search_filters.currentPage <= page_entry->totalPages - 1) ui_run_func_on_tag(&default_screen, "nextpage", ui_enable_element); else ui_run_func_on_tag(&default_screen, "nextpage", ui_disable_element);
    if ((search_filters.currentPage) >= 1) ui_run_func_on_tag(&default_screen, "prevpage", ui_enable_element); else ui_run_func_on_tag(&default_screen, "prevpage", ui_disable_element);

        char pageInfo[32];
    snprintf(pageInfo, 42 - 1, "%d to %d of %d", page_entry->currentOffset, page_entry->currentOffset + page_entry->amount, page_entry->totalPages * page_entry->amount - 1);
    ui_label_set_text(page_info_label, pageInfo);

}

static void populate_list() {
    ui_list_reset(list);
    for (int i = 0; i < searchEntriesLength; i++) {
        char tmp_name[32];
        char tmp_creator[32];
        char tmp_song[256];

        SearchEntry *entry = &search_entries[i];

        strncpy(tmp_name, entry->name, sizeof(tmp_name) - 1);
        
        // Get creator name
        char *creator_name = "Unknown";

        if (entry->creatorIndex < creatorEntriesLength) {
            creator_name = creator_entries[entry->creatorIndex].creatorName;
        }

        strncpy(tmp_creator, creator_name, sizeof(tmp_creator) - 1);

        // Get song name
        char *song_name = "Unknown";

        if (entry->songId != 0) {
            if (entry->songIndex < songEntriesLength) {
                song_name = song_entries[entry->songIndex].songTitle;
            }
        } else {
            if (IN_BOUNDS(entry->mainSongId, main_songs)) {
                song_name = (char *) main_songs[entry->mainSongId].title;
            }
        }

        const char *song_color = (entry->songId != 0 ? "<#f982ff>" : "<#27d2ff>");

        snprintf(tmp_song, sizeof(tmp_song) - 1, "%s%s", song_color, song_name);

        truncate_filename(tmp_song, 35);

        float list_width = list->base.w * 0.5f;

        UIElement *card = (UIElement *)ui_create_rectangle(&default_screen.ctx);

        if (card) {
            ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
            ui_element_set_size(card, 0, 60);

            // Level name
            UILabel *name_label = ui_create_label(&default_screen.ctx);
            if (name_label) {
                ui_label_set_text(name_label, tmp_name);
                ui_element_set_position((UIElement *)name_label, -list_width + 48, -17);
                ui_element_set_scale((UIElement *)name_label, 0.54f);
                name_label->base.w = 130;
                ui_element_add_child(card, (UIElement *)name_label);
            }

            // Level creator
            UILabel *creator_label = ui_create_label(&default_screen.ctx);
            if (creator_label) {
                ui_label_set_text(creator_label, tmp_creator);
                ui_element_set_position((UIElement *)creator_label, -list_width + 48, -4.5f);
                ui_element_set_scale((UIElement *)creator_label, 0.45f);

                creator_label->font = 2;

                ui_element_add_child(card, (UIElement *)creator_label);
            }

            // copy icon
            UIImage *collaboration_icon = ui_create_image(&default_screen.ctx);
            if (collaboration_icon && entry->originalId != 0) {
                ui_image_set_image(collaboration_icon, 213, 0);
                ui_element_set_position((UIElement *)collaboration_icon, -list_width + 48 + get_text_length(&goldFont_fontCharset, 0.45f, false, tmp_creator) + 10, -4.5f);
                ui_element_set_scale((UIElement *)collaboration_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)collaboration_icon);
            }

            // high object count icon
            UIImage *high_object_icon = ui_create_image(&default_screen.ctx);
            if (high_object_icon && (entry->objCount >= 44000)) {
                ui_image_set_image(high_object_icon, 362, 0);
                ui_element_set_position((UIElement *)high_object_icon, -list_width + 48 + get_text_length(&goldFont_fontCharset, 0.45f, false, tmp_creator) + 10 + ((entry->originalId != 0) ? 11 : 0), -4.5f);
                ui_element_set_scale((UIElement *)high_object_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)high_object_icon);
            }

            // Level song
            UILabel *song_label = ui_create_label(&default_screen.ctx);
            if (song_label) {
                ui_label_set_text(song_label, tmp_song);
                ui_element_set_position((UIElement *)song_label, -list_width + 48, 7);
                ui_element_set_scale((UIElement *)song_label, 0.35f);
                song_label->base.w = 130;
                ui_element_add_child(card, (UIElement *)song_label);
            }

            // Level length
            UILabel *length_label = ui_create_label(&default_screen.ctx);
            if (length_label) {
                char *level_length = "Unkn.";
                if (IN_BOUNDS(entry->lengthNum, level_lengths)) {
                    level_length = (char *) level_lengths[entry->lengthNum];
                }

                ui_label_set_text(length_label, level_length);
                ui_element_set_position((UIElement *)length_label, -list_width + 60, 19.3f);
                ui_element_set_scale((UIElement *)length_label, 0.35f);
                
                ui_element_add_child(card, (UIElement *)length_label);
            }

            // Downloads
            UILabel *download_value = ui_create_label(&default_screen.ctx);
            if (download_value) {
                char *tmp_value = truncate_number(entry->downloads);

                ui_label_set_text(download_value, tmp_value);
                ui_element_set_position((UIElement *)download_value, -list_width + 110, 19.3f);
                ui_element_set_scale((UIElement *)download_value, 0.35f);

                ui_element_add_child(card, (UIElement *)download_value);
            }

            // Likes
            UILabel *like_value = ui_create_label(&default_screen.ctx);
            if (like_value) {
                char *tmp_value = truncate_number(entry->likes);

                ui_label_set_text(like_value, tmp_value);
                ui_element_set_position((UIElement *)like_value, -list_width + 160, 19.3f);
                ui_element_set_scale((UIElement *)like_value, 0.35f);

                ui_element_add_child(card, (UIElement *)like_value);
            }

            // Stars
            UILabel *star_value = ui_create_label(&default_screen.ctx);
            if (star_value && entry->stars > 0) {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "%d", entry->stars);

                ui_label_set_text(star_value, tmp_value);
                ui_element_set_position((UIElement *)star_value, -list_width + 22, 20.3f);
                ui_element_set_scale((UIElement *)star_value, 0.35f);

                star_value->alignment = 1.f;

                ui_element_add_child(card, (UIElement *)star_value);
            }

            UIImage *featured_glow = ui_create_image(&default_screen.ctx);
            if (featured_glow && entry->featureScore > 0) {
                ui_image_set_image(featured_glow, 72, 0);
                ui_element_set_position((UIElement *)featured_glow, -list_width + 23, -8.5f);
                ui_element_set_scale((UIElement *)featured_glow, 0.82f);

                ui_element_add_child(card, (UIElement *)featured_glow);
            }

            UIImage *difficulty_face = ui_create_image(&default_screen.ctx);
            if (difficulty_face) {
                int difficulty_id = difficulty_stars[0];

                if (IN_BOUNDS(entry->stars, difficulty_stars)) {
                    difficulty_id = difficulty_stars[entry->stars];
                }

                ui_image_set_image(difficulty_face, difficulty_id, 0);
                ui_element_set_position((UIElement *)difficulty_face, -list_width + 23,  (entry->stars > 0) ? -4 : 0);
                ui_element_set_scale((UIElement *)difficulty_face, 0.82f);

                ui_element_add_child(card, (UIElement *)difficulty_face);
            }

            UIImage *star_icon = ui_create_image(&default_screen.ctx);
            if (star_icon && entry->stars > 0) {
                ui_image_set_image(star_icon, 170, 0);
                ui_element_set_position((UIElement *)star_icon, -list_width + 29, 20);
                ui_element_set_scale((UIElement *)star_icon, 0.71f);

                ui_element_add_child(card, (UIElement *)star_icon);
            }

            UIImage *length_icon = ui_create_image(&default_screen.ctx);
            if (length_icon) {
                ui_image_set_image(length_icon, 197, 0);
                ui_element_set_position((UIElement *)length_icon, -list_width + 53, 20);
                ui_element_set_scale((UIElement *)length_icon, 0.5f);

                ui_element_add_child(card, (UIElement *)length_icon);
            }

            UIImage *download_icon = ui_create_image(&default_screen.ctx);
            if (download_icon) {
                ui_image_set_image(download_icon, 163, 0);
                ui_element_set_position((UIElement *)download_icon, -list_width + 103, 20);
                ui_element_set_scale((UIElement *)download_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)download_icon);
            }

            UIImage *like_icon = ui_create_image(&default_screen.ctx);
            if (like_icon) {
                ui_image_set_image(like_icon, 97, 0);
                ui_element_set_position((UIElement *)like_icon, -list_width + 153, 19);
                ui_element_set_scale((UIElement *)like_icon, 0.5f);

                if (entry->likes < 0) {
                    ui_image_set_image(like_icon, DISLIKE_ICON, 0);
                }

                ui_element_add_child(card, (UIElement *)like_icon);
            }

            UIWindowButton *button = ui_create_window_button(&default_screen.ctx);
            if (button) {
                // Store in the user data
                OnlineCardData *data = malloc(sizeof(*data));

                data->entryId = i;

                ui_window_button_set_style(button, 5);
                ui_button_set_text((UIButton *)button, "View");

                button->base.textScale = 0.48f;

                ui_element_set_position((UIElement *)button, list_width - 32, 0);
                ui_element_set_size((UIElement *)button, 48, 28);
                ui_element_set_action((UIElement *)button, action_open_online_level_menu);
                ui_element_set_userdata((UIElement *) button, data);
                ui_element_add_child(card, (UIElement *)button);
            }

            ui_list_add(list, card);
        }
    }  
}

static void handle_errors(int code) {
    char temp[64];
    switch (code) {
        case -2:
            ui_label_set_text(error_label, "An unknown error has<p> occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            ui_label_set_text(error_label, "No<p><#41e24e>Internet</> connection!");
            break;

        default:
            snprintf(temp, sizeof(temp), "An unknown error has<p>occurred.<p><p>Error code: %d", code);
            ui_label_set_text(error_label, temp);

    }
}

static void action_change_page(UIElement* e) {
    search_filters.currentPage += ui_prop_int(&e->custom_properties, "page", 0);
    search_needs_refresh = true;
    
    int search_result = -2;

    if (search_needs_refresh) {
        search_result = search_levels();
    }
   
    // Handle result
    if (search_result != 0 && search_needs_refresh) {
        handle_errors(search_result);
    } else if (list) { // No errors
        populate_list();
        search_needs_refresh = false;
        update_arrows();
    }
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"open_level_menu", action_open_online_level_menu },
    {"changepage", action_change_page }
};

void online_menu_loop() {
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
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_levels_top.txt");

    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    list = (UIList *) ui_get_element_by_tag(&default_screen, "list");
    error_label = (UILabel *)ui_get_element_by_tag(&default_screen, "errorLabel");
    page_info_label = (UILabel *)ui_get_element_by_tag(&default_screen_top, "pageinfo");

    ui_run_func_on_tag(&default_screen, "nextpage", ui_disable_element);
    ui_run_func_on_tag(&default_screen, "prevpage", ui_disable_element);

    int search_result = -2;

    if (search_needs_refresh) {
        search_result = search_levels();
    }
   
    // Handle result
    if (search_result != 0 && search_needs_refresh) {
        handle_errors(search_result);
    } else if (list) { // No errors
        populate_list();
        search_needs_refresh = false;
        update_arrows();
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
            if (search_entries) {
                if (search_entries->description) free(search_entries->description);
                free(search_entries);
                search_entries = NULL;
            }
            if (creator_entries) {
                free(creator_entries);
                creator_entries = NULL;
            }
            if (song_entries) {
                free(song_entries);
                song_entries = NULL;
            }

            game_state = STATE_SEARCH_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));

    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
