#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_rectangle.h"
#include "main.h"
#include "utils/folders.h"
#include "utils/server_utils.h"
#include "search_menu.h"
#include "online_menu.h"
#include "fonts/chatFont.h"

#include "state.h"

static bool yes_exit = false;
bool comments_need_refresh = true;

static UIList *list;
static UILabel *error_label;

static UIScreen screen = {
    .isBottom = true,
};

void exit_comments(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_comments }
};

static void handle_comment_errors(int code) {
    char temp[64];
    switch (code) {
        case -2:
            ui_label_set_text(error_label, "An unknown error has<p> occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            ui_label_set_text(error_label, "No<p><#599cff>Internet</> connection!");
            break;

        default:
            snprintf(temp, sizeof(temp), "An unknown error has<p>occurred.<p><p>Error code: %d", code);
            ui_label_set_text(error_label, temp);

    }
}

void populate_comments()
{

    ui_list_reset(list);
    for (int i = 0; i < commentEntriesLength; i++)
    {
        char username[25];
        char timestamp[129];
        int percent = comment_entries[i].percent;
        int likes = comment_entries[i].likes;

        strncpy(username, comment_entries[i].name, sizeof(username) - 1);
        strncpy(timestamp, comment_entries[i].commentAge, sizeof(timestamp) - 1);

        truncate_filename(username, 18);

        float list_width = list->base.w * 0.5f;
        float list_height = 70 * 0.5f;

        UIElement *card = (UIElement *)ui_create_rectangle(&default_screen.ctx);

        if (card)
        {
            ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 44, 255)));
            ui_element_set_size(card, 0, 70);

            UIWindow *bg_window = ui_create_window(&default_screen.ctx);
            if (bg_window)
            {
                ui_element_set_size((UIElement *)bg_window, 2 * list_width - 10, 2 * list_height - 10);
                ui_element_set_position((UIElement *)bg_window, 0, 0);

                bg_window->atlas = C2D_SpriteSheetGetImage(window_sheet, 2);
                bg_window->border = bg_window->atlas.subtex->width / 3;
                bg_window->color = C2D_Color32(130, 64, 33, 100);
                ui_element_add_child(card, (UIElement *)bg_window);
            }

            // Comment author
            UILabel *username_label = ui_create_label(&default_screen.ctx);
            if (username_label)
            {
                ui_label_set_text(username_label, username);
                ui_element_set_position((UIElement *)username_label, -list_width + 10, -list_height + 15);
                ui_element_set_scale((UIElement *)username_label, 0.6f);

                username_label->font = 2;

                ui_element_add_child(card, (UIElement *)username_label);
            }

            // Comment content
            UILabel *content_label = ui_create_label(&default_screen.ctx);
            if (content_label)
            {
                char *wrapped_content = wrap_text(&chatFont_fontCharset, content_label->base.scaleX, comment_entries[i].content, 270);
                char *desc = strdup(wrapped_content);
                ui_label_set_text(content_label, desc);
                free(desc);
                ui_element_set_position((UIElement *)content_label, -list_width + 12, -list_height + 34);
                ui_element_set_scale((UIElement *)content_label, 0.68f);

                content_label->font = 1;

                ui_element_add_child(card, (UIElement *)content_label);
            }

            // Comment percent
            UILabel *percent_label = ui_create_label(&default_screen.ctx);
            if (percent_label)
            {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "<#00000096>%d%%", percent);
                ui_label_set_text(percent_label, tmp_value);
                ui_element_set_position((UIElement *)percent_label, list_width - 11, -list_height + 15);
                ui_element_set_scale((UIElement *)percent_label, 0.63f);

                percent_label->font = 1;
                percent_label->alignment = 1;

                ui_element_add_child(card, (UIElement *)percent_label);
            }

            // Comment likes
            UILabel *like_value = ui_create_label(&default_screen.ctx);
            if (like_value)
            {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "%d", likes);

                ui_label_set_text(like_value, tmp_value);
                ui_element_set_position((UIElement *)like_value, -list_width + 28, list_height - 16);
                ui_element_set_scale((UIElement *)like_value, 0.35f);

                // like_value->alignment = 1.f;

                ui_element_add_child(card, (UIElement *)like_value);
            }

            UIImage *like_icon = ui_create_image(&default_screen.ctx);
            if (like_icon)
            {
                ui_image_set_image(like_icon, 166, 0);
                ui_element_set_position((UIElement *)like_icon, -list_width + 18, list_height - 16);
                ui_element_set_scale((UIElement *)like_icon, 0.9f);

                ui_element_add_child(card, (UIElement *)like_icon);
            }

            // Comment timestamp
            UILabel *timestamp_value = ui_create_label(&default_screen.ctx);
            if (timestamp_value)
            {
                char tmp_value[256];
                snprintf(tmp_value, sizeof(tmp_value), "<#0000007D>%s", timestamp);

                ui_label_set_text(timestamp_value, tmp_value);
                ui_element_set_position((UIElement *)timestamp_value, list_width - 8, list_height - 15);
                ui_element_set_scale((UIElement *)timestamp_value, 0.65f);

                timestamp_value->font = 1;
                timestamp_value->alignment = 1.f;

                ui_element_add_child(card, (UIElement *)timestamp_value);
            }
            ui_list_add(list, card);
        }
    }
}

void online_comments_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_comments.txt");

    ui_screen_open(&screen, ANIM_ZOOM_SUBTLE);

    list = (UIList *) ui_get_element_by_tag(&screen, "list");

    int result = -2;

    if (comments_need_refresh) {
        result = get_comments(search_entries[curr_search_id].levelId, 1);
    }
   
    // Handle result
    if (result != 0 && comments_need_refresh) {
        handle_comment_errors(result);
    } else if (list) { // No errors
        populate_comments();
        comments_need_refresh = false;
    }

    yes_exit = false;
}

int online_comments_loop() {

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