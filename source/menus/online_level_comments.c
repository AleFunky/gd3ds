#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "main.h"
#include "icons.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "utils/folders.h"
#include "utils/server_utils.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_icon.h"
#include "menus/external_popup.h"
#include "menus/icon_kit.h"
#include "menus/search_menu.h"
#include "menus/online_menu.h"
#include "menus/palette_kit.h"

#include "state.h"

static bool yes_exit = false;
bool comments_need_refresh = true;
int comments_sort_type = 1;
int current_comments_page = 0;

static UIList *list;
static UILabel *error_label;

static UIButton *sort_by_recency_button;
static UIButton *sort_by_likes_button;

static UISpinner *spinner;

static UIScreen screen = {
    .isBottom = true,
};

static Thread thread;

static NetworkTask comments_task = {
    .func = get_comments
};

void action_exit_comments(UIElement* e) {
    if (comments_task.running) {
        comments_task.cancelled = true;
        threadJoin(thread, U64_MAX);
    }

    yes_exit = true;
}

static void update_comment_arrows(bool disableArrows) {
    if (commentEntriesLength == 10 && !disableArrows) ui_run_func_on_tag(&screen, "nextpage", ui_enable_element); else ui_run_func_on_tag(&screen, "nextpage", ui_disable_element);
    if ((current_comments_page) >= 1 && !disableArrows) ui_run_func_on_tag(&screen, "prevpage", ui_enable_element); else ui_run_func_on_tag(&screen, "prevpage", ui_disable_element);
}

static void handle_comment_errors(int code) {
    ui_disable_element((UIElement *)spinner);
    ui_enable_element((UIElement *)error_label);
    char temp[64];
    switch (code) {
        case -2:
            //gdps returns -2 when no comments are found idky 
            if (gdps) break;
            ui_label_set_text(error_label, "An unknown error has\n occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            ui_label_set_text(error_label, "No\n<#41e24e>Internet</> connection!");
            update_comment_arrows(true);
            break;

        default:
            snprintf(temp, sizeof(temp), "An unknown error has\noccurred.\n\nError code: %d", code);
            ui_label_set_text(error_label, temp);
            break;
    }
}

void populate_comments() {
    ui_list_reset(list);
    ui_disable_element((UIElement *) spinner);
    for (int i = 0; i < commentEntriesLength; i++) {
        char username[36] = "-";
        snprintf(username, sizeof(username), "<#%s>%s</>", (comment_entries[i].authorAccountId == 0) ? "5AFFFF" : "FFFFFF", comment_entries[i].name);

        char timestamp[136];
        int percent = comment_entries[i].percent;
        int likes = comment_entries[i].likes;

        snprintf(timestamp, sizeof(timestamp) - 1, "%s ago", comment_entries[i].commentAge);

        float list_width = list->base.w * 0.5f;
        float list_height = 86 * 0.5f;

        UIElement *card = (UIElement *)ui_create_rectangle(&default_screen.ctx);

        if (card) {
            ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 44, 255)));
            ui_element_set_size(card, 0, 86);

            UIWindow *bg_window = ui_create_window(&default_screen.ctx);
            if (bg_window) {
                ui_element_set_size((UIElement *)bg_window, 2 * list_width - 10, 2 * list_height - 10);
                ui_element_set_position((UIElement *)bg_window, 0, 0);

                bg_window->atlas = C2D_SpriteSheetGetImage(window_sheet, 2);
                bg_window->border = bg_window->atlas.subtex->width / 3;
                bg_window->color = C2D_Color32(130, 64, 33, 100);
                ui_element_add_child(card, (UIElement *)bg_window);
            }

            // Commenter's icon (robtop server exclusive)
            UIIcon *user_icon = ui_create_icon(&default_screen.ctx);
            if (user_icon && !gdps) {
                //output_log("icon type: %d\nicon id: %d\nicon p1: %d\n icon p2: %d\n\n", comment_entries[i].iconType, comment_entries[i].playerIcon, comment_entries[i].col1, comment_entries[i].col2);
                int iconType = comment_entries[i].iconType;
                int iconIndex = comment_entries[i].playerIcon;

                // check if provided player icon is in bounds
                if (iconType >= GAMEMODE_COUNT) {
                    iconType = 0;
                    iconIndex = 1;
                } else if (iconIndex >= gamemode_icon_count[iconType]) iconIndex = 1;

                
                user_icon->glow = comment_entries[i].glow;
                ui_icon_set_gamemode_index(user_icon, iconType, iconIndex);
                ui_icon_set_glow(user_icon, colors[gd_to_gd3ds_color_table[comment_entries[i].glowCol]]);
                ui_icon_set_p1(user_icon, colors[gd_to_gd3ds_color_table[comment_entries[i].col1]]);
                ui_icon_set_p2(user_icon, colors[gd_to_gd3ds_color_table[comment_entries[i].col2]]);
                
                ui_element_set_position((UIElement *)user_icon, -list_width + 20, -list_height + 15);
                ui_element_set_scale((UIElement *)user_icon, 0.5f);

                ui_element_add_child(card, (UIElement *)user_icon);
            }

            // Comment author
            UILabel *username_label = ui_create_label(&default_screen.ctx);
            if (username_label) {
                ui_label_set_text(username_label, username);
                ui_element_set_position((UIElement *)username_label, -list_width + (gdps ? 10 : 30), -list_height + 15);
                ui_element_set_scale((UIElement *)username_label, 0.6f);

                username_label->font = 2;

                ui_element_add_child(card, (UIElement *)username_label);
            }

            // Mod badge

            UIImage *badge = ui_create_image(&default_screen.ctx);
            if (badge && comment_entries[i].modBadge > 0) {
                int badgeOffset = 0;
                badgeOffset += comment_entries[i].modBadge;
                if (comment_entries[i].modBadge > 2) badgeOffset = 2;
                ui_image_set_image(badge, 371 + badgeOffset, 0);
                ui_element_set_position((UIElement *)badge, -list_width + (gdps ? 15 : 30) + get_text_length(&goldFont_fontCharset, 0.7, false, comment_entries[i].name) + 3, -list_height + 15);
                ui_element_set_scale((UIElement *)badge, 0.7f);

                ui_element_add_child(card, (UIElement *)badge);
            }

            // Comment content
            UILabel *content_label = ui_create_label(&default_screen.ctx);
            if (content_label) {
                char *wrapped_content = wrap_text(&chatFont_fontCharset, content_label->base.scaleX, comment_entries[i].content, 270);
                char *desc = strdup(wrapped_content);
                char tinted_desc[strlen(desc) + 13];
                snprintf(tinted_desc, sizeof(tinted_desc), "<%s>%s</>", ((comment_entries[i].modBadge == 0) ? "#FFFFFF" : comment_entries[i].modCommentColor), desc);
                ui_label_set_text(content_label, tinted_desc);
                free(desc);
                ui_element_set_position((UIElement *)content_label, -list_width + 12, 0 - 1.5);
                ui_element_set_scale((UIElement *)content_label, 0.68f);

                content_label->font = 1;

                ui_element_add_child(card, (UIElement *)content_label);
            }

            // Comment percent
            UILabel *percent_label = ui_create_label(&default_screen.ctx);
            if (percent_label && percent > 0) {
                char tmp_value[24];

                snprintf(tmp_value, sizeof(tmp_value), "<#00000096>%d%%", percent);
                ui_label_set_text(percent_label, tmp_value);
                ui_element_set_position((UIElement *)percent_label, list_width - 11, -list_height + 15);
                ui_element_set_scale((UIElement *)percent_label, 0.63f);

                percent_label->font = 1;
                percent_label->alignment = 1;

                ui_element_add_child(card, (UIElement *)percent_label);
            }

            // Comment likes

            UIImage *like_icon = ui_create_image(&default_screen.ctx);
            if (like_icon) {
                ui_image_set_image(like_icon, 97, 0);
                ui_element_set_position((UIElement *)like_icon, -list_width + 18, list_height - 16);
                ui_element_set_scale((UIElement *)like_icon, 0.7f);

                if (likes < 0) {
                    ui_image_set_image(like_icon, DISLIKE_ICON, 0);
                }

                ui_element_add_child(card, (UIElement *)like_icon);
            }

            UILabel *like_value = ui_create_label(&default_screen.ctx);
            if (like_value) {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "%d",  likes);

                ui_label_set_text(like_value, tmp_value);
                ui_element_set_position((UIElement *)like_value, -list_width + 28, list_height - 16);
                ui_element_set_scale((UIElement *)like_value, 0.35f);

                // like_value->alignment = 1.f;

                ui_element_add_child(card, (UIElement *)like_value);
            }

            // Comment timestamp
            if(!gdps){
                UILabel *timestamp_value = ui_create_label(&default_screen.ctx);
                if (timestamp_value) {
                    char tmp_value[sizeof(timestamp) + 14 - 1];
                    snprintf(tmp_value, sizeof(tmp_value), "<#0000007D>%s", timestamp);

                    ui_label_set_text(timestamp_value, tmp_value);
                    ui_element_set_position((UIElement *)timestamp_value, list_width - 8, list_height - 15);
                    ui_element_set_scale((UIElement *)timestamp_value, 0.65f);

                    timestamp_value->font = 1;
                    timestamp_value->alignment = 1.f;

                    ui_element_add_child(card, (UIElement *)timestamp_value);
                }
            }
            ui_list_add(list, card);
        }
    }
}

static void action_refresh_comments(UIElement* e) {
    int buttonType = ui_prop_int(&e->custom_properties, "sorttype", 0);

    if (buttonType != -1) {
        ui_button_set_image(sort_by_recency_button, 422, 0);
        ui_button_set_image(sort_by_likes_button, 422, 0);
        ui_button_set_image((UIButton *)e, 423, 0);

        current_comments_page = 0;

        comments_sort_type = buttonType;
    }
    ui_list_reset(list);
    ui_disable_element((UIElement *) error_label);
    ui_enable_element((UIElement *)spinner);
    
    thread = create_network_thread(&comments_task);
}

static void action_change_comments_page(UIElement* e) {
    current_comments_page += ui_prop_int(&e->custom_properties, "page", 0);
    
    ui_list_reset(list);
    update_comment_arrows(false);
    ui_enable_element((UIElement *)spinner);
    thread = create_network_thread(&comments_task);
}

static UIAction actions[] = {
    { "exit", action_exit_comments },
    { "refresh", action_refresh_comments },
    { "changepage", action_change_comments_page }
};

void online_comments_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_comments.txt");
    ui_screen_open(&screen, ANIM_ZOOM_SUBTLE);

    list = (UIList *) ui_get_element_by_tag(&screen, "list");

    error_label = (UILabel *) ui_get_element_by_tag(&screen, "errorlabel");
    sort_by_recency_button = (UIButton *) ui_get_element_by_tag(&screen, "sortbyrecency");
    sort_by_likes_button = (UIButton *) ui_get_element_by_tag(&screen, "sortbylikes");

    spinner = (UISpinner *) ui_get_element_by_tag(&screen, "spinner");

    ui_button_set_image(sort_by_likes_button, 423, 0);
    ui_disable_element((UIElement *) spinner);
    ui_disable_element((UIElement *) error_label);
    update_comment_arrows(true);
    if (comments_need_refresh) {
        ui_enable_element((UIElement *)spinner);
        current_comments_page = 0;
        comments_sort_type = 1;
        create_network_thread(&comments_task);
    } else {
        if (list) { // No need to fetch new comments
            ui_label_set_text(error_label, "");
            populate_comments();
            update_comment_arrows(false);
        }
    }

    yes_exit = false;
}

int online_comments_loop() {

    // Run when finished
    if (comments_task.finished) {
        int result = comments_task.result;
        // Handle result
        if (result != 0 && comments_need_refresh) {
            handle_comment_errors(result);
        } else { // No errors
            ui_label_set_text(error_label, "");
            update_comment_arrows(false);
            populate_comments();
            comments_need_refresh = false;
        }
        comments_task.finished = false;
    }

    if (yes_exit) {        
        ui_unload_screen(&screen);
        return true;
    }
    
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);

    return false;
}

void online_comments_draw() {
    ui_screen_draw(&screen);
}