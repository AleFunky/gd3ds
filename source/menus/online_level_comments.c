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

#include "state.h"

static bool yes_exit = false;

static UIList *list;

typedef struct CommentEntries {
    char *username;
    char *content;
    int percent;
    int likes;
    char *timestamp;
} CommentEntries;


static const CommentEntries comments[] = {
    { "user 1", "i hate this level", 54, 123, "3 years ago" },
    { "hi my user is really long", "i like this level", -5, 123, "2 days ago" },
    { "crap", "i have no opinion on this level.", 200, 9999, "-5 seconds ago" },
};

UIElement entries2[ARRAY_LEN(comments)];

static UIScreen screen = {
    .isBottom = true,
};

void exit_comments(UIElement* e) {
    yes_exit = true;
}

static UIAction actions[] = {
    { "exit", exit_comments }
};

void online_comments_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_comments.txt");

    ui_screen_open(&screen, ANIM_ZOOM_SUBTLE);

    list = (UIList *) ui_get_element_by_tag(&screen, "list");

    if (list)
    {
        for (int i = 0; i < ARRAY_LEN(comments); i++)
        {
            char username[256];
            char content[256];
            char timestamp[32];
            int percent = comments[i].percent;
            int likes = comments[i].likes;

            strncpy(username, comments[i].username, sizeof(username) - 1);
            strncpy(content, comments[i].content, sizeof(content) - 1);
            strncpy(timestamp, comments[i].timestamp, sizeof(timestamp) - 1);

            truncate_filename(username, 13);

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
                    ui_label_set_text(content_label, content);
                    ui_element_set_position((UIElement *)content_label, -list_width + 12, -list_height + 28);
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
                    ui_element_set_position((UIElement *)percent_label, list_width -11, -list_height + 15);
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
                    ui_element_set_position((UIElement *)timestamp_value, list_width -8, list_height - 15);
                    ui_element_set_scale((UIElement *)timestamp_value, 0.65f);

                    timestamp_value->font = 1;
                    timestamp_value->alignment = 1.f;
                    
                    ui_element_add_child(card, (UIElement *)timestamp_value);
                }
                ui_list_add(list, card);
            }
        }
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