#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.h"
#include "text.h"

#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_spinner.h"
#include "menus/components/ui_window_button.h"
#include "menus/settings_hub/info_card.h"
#include "menus/updater.h"
#include "menus/updater_pop_up.h"
#include "menus/updater_settings.h"
#include "fonts/chatFont.h"

#include "utils/server_utils.h"
#include "utils/string_helpers.h"
#include "utils/utils.h"
#include "utils/folders.h"

static UIScreen *screen;

static UIList *list;
static UILabel *error_label;
static UIWindowButton *updater_button;
static UISpinner *spinner;

static GenericTask updater_task = {
    .func = check_for_updates
};

static Thread updater_thread;
bool checkedForUpdate = false;

static void action_update(UIElement *e, const UIPropertyList *p) {
    if (is_3DSX && is_citra() && !_3dsx_path) {
        InfoCardData *data = malloc(sizeof(InfoCardData));
        if (!data) return;

        data->text = strdup(".3dsx updates aren't supported on\nemulators! Please update the game\nmanually.");
        data->title = strdup("Error");
        data->customTitle = true;
        data->copied = true;

        ui_stack_push(&info_card_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
        ui_stack_push_data(data);
        return;
    }
    if (update_data->isAvailable && checkedForUpdate) ui_stack_push(&updater_pop_up_def, ANIM_NONE, ANIM_ZOOM, PUSH_NEXT); 
}

static void action_refresh_updates(UIElement *e, const UIPropertyList *p) {
    ui_label_set_text(error_label, "");
    ui_list_reset(list);
    ui_enable_element((UIElement *) spinner);
    ui_disable_element((UIElement *) updater_button);
    checkedForUpdate = false;
    updater_thread = create_generic_thread(&updater_task);
}

char *handle_updater_error_codes(int code) {
    switch (code) {
        case -3:
            return "No updates found.";
            break;
        case -2:
            return "Failed to find\nrelease!";
            break;
        case -1:
            return "Failed to parse\nresponse!";
            break;
        case 6:
        case 7:
            return "No internet\nconnection!";
            break;
        case 28:
            return "Connection timed\nout.";
            break;
        case 56: 
            return "Connection interrupted.";
            break;
        case 42:
            break;
        default:
            return "Unknown error.";
            break;
    }
    return "";
}

static UIActionDef updater_actions[] = {
    {"update", action_update },
    {"refresh", action_refresh_updates }
};

static void populate_info() {
    int list_width = list->base.w / 2;
    int textMarginX = 6;
    int textMarginY = 6;
    float textScale = 0.8;
    int lineHeight = 12;

    // add updates found text
    UIElement *card = (UIElement *)ui_create_rectangle(screen);
    if (card) {
        ui_rectangle_set_color((UIRectangle *)card, C2D_Color32(161, 88, 48, 255));
        ui_element_set_size(card, list->base.w, 38);
        // title
        UILabel *title = ui_create_label(screen);
        if (title) {
            ui_label_set_text(title, "Update found!");
            ui_element_set_scale((UIElement *)title, 0.7);
            ui_element_set_position((UIElement *)title, 0, 0);
            title->alignment = 0.5f;
            ui_element_add_child(card, (UIElement *)title);
        }
        ui_list_add(list, card);
    }

    // list commits for nightly
    if (useNightlyBranch && latestCommits) {

        // description card
        UIElement *info = (UIElement *)ui_create_rectangle(screen);
        if (info) {
            ui_rectangle_set_color((UIRectangle *)info, C2D_Color32(194, 114, 62, 255));
            ui_element_set_size(info, list->base.w, lineHeight * 1.2 + textMarginY * 2);
            // label
            UILabel *label = ui_create_label(screen);
            if (label) {
                ui_label_set_text(label, "Changes since last update:");
                ui_element_set_scale((UIElement *)label, textScale * 1.2);
                ui_element_set_position((UIElement *)label, -list_width + textMarginX, 0);
                label->font = 1;
                ui_element_add_child(info, (UIElement *)label);
            }
            ui_list_add(list, info);
        }

        for (int i = 0; i < latestCommitCount; i++) {
            char *buf = wrap_text(&chatFont_fontCharset, textScale, latestCommits[i], list->base.w - 2 * textMarginX);
            int lineCount = count_lines(buf, true);

            UIElement *card = (UIElement *)ui_create_rectangle(screen);
            if (card && i < 30 - 1) {
                ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
                ui_element_set_size(card, list->base.w, lineCount * lineHeight + textMarginY * 2);
                
                // commit title
                UILabel *title = ui_create_label(screen);
                if (title) {
                    title->base.w = list->base.w - 2 * textMarginX; 
                    ui_label_set_text(title, buf);

                    ui_element_set_position((UIElement *)title, -list_width + textMarginX, 0);
                    ui_element_set_scale((UIElement *)title, textScale);

                    title->font = 1;

                    ui_element_add_child(card, (UIElement *)title);
                }

                ui_list_add(list, card);
            } else if (card) {
                ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
                ui_element_set_size(card, list->base.w, lineHeight + textMarginY * 2);
                
                // ... and more
                UILabel *title = ui_create_label(screen);
                if (title) {
                    title->base.w = list->base.w - 2 * textMarginX; 
                    ui_label_set_text(title, "... and more!");

                    ui_element_set_position((UIElement *)title, -list_width + textMarginX, 0);
                    ui_element_set_scale((UIElement *)title, textScale);

                    title->font = 1;

                    ui_element_add_child(card, (UIElement *)title);
                }

                ui_list_add(list, card);
            }
        }
    } else if (update_data->releaseBody) {
        int baseLineCount = 0;
        char **infoText = split_string(update_data->releaseBody, '\n', &baseLineCount, true);
        // description card
        UIElement *info = (UIElement *)ui_create_rectangle(screen);
        if (info) {
            ui_rectangle_set_color((UIRectangle *)info, C2D_Color32(194, 114, 62, 255));
            ui_element_set_size(info, list->base.w, lineHeight + textMarginY * 2);
            // label
            UILabel *label = ui_create_label(screen);
            if (label) {
                ui_label_set_text(label, update_data->releaseTitle);
                ui_element_set_scale((UIElement *)label, textScale * 1.2);
                ui_element_set_position((UIElement *)label, -list_width + textMarginX, 0);
                label->font = 1;
                ui_element_add_child(info, (UIElement *)label);
            }
            ui_list_add(list, info);
        }
        // body 
        for (int i = 0; i < baseLineCount; i++) {
            char *buf = wrap_text(&chatFont_fontCharset, textScale, infoText[i], list->base.w - 2 * textMarginX);
            int lineCount = count_lines(buf, true);

            UIElement *card = (UIElement *)ui_create_rectangle(screen);
            if (card) {
                ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
                ui_element_set_size(card, list->base.w, lineCount * lineHeight + textMarginY * 2);
                
                // commit title
                UILabel *title = ui_create_label(screen);
                if (title) {
                    title->base.w = list->base.w - 2 * textMarginX; 
                    ui_label_set_text(title, buf);

                    ui_element_set_position((UIElement *)title, -list_width + textMarginX, 0);
                    ui_element_set_scale((UIElement *)title, textScale);

                    title->font = 1;

                    ui_element_add_child(card, (UIElement *)title);
                }

                ui_list_add(list, card);
            }   
        }
        free_string_array(infoText, baseLineCount);
    }
    // add footer so the install update button doesnt cover up the last bit of info
    UIElement *footer = (UIElement *)ui_create_rectangle(screen);
    if (footer) {
        ui_rectangle_set_color((UIRectangle *)footer, C2D_Color32(161, 88, 48, 0));
        ui_element_set_size(footer, list->base.w, 15);
        ui_list_add(list, footer);
    }
}

static void updater_init(UIScreen *s) {
    screen = s;
    list = (UIList *)ui_get_element_by_tag(s, "list");
    updater_button = (UIWindowButton *)ui_get_element_by_tag(s, "updatebutton");
    spinner = (UISpinner *)ui_get_element_by_tag(s, "spinner");
    error_label = (UILabel *)ui_get_element_by_tag(s, "errorlabel");
    if (checkedForUpdate && update_data->isAvailable) {
        ui_disable_element((UIElement *) spinner);
        ui_label_set_text(error_label, update_data->releaseTitle);
        populate_info();
    } else {
        ui_disable_element((UIElement *) updater_button);
        ui_enable_element((UIElement *) spinner);
        updater_thread = create_generic_thread(&updater_task);
    }
}

static void updater_update(UIScreen *s, UIInput *i) {
    // Run when finished
    if (updater_task.finished) {
        ui_disable_element((UIElement *) spinner);
        // Handle result
        if (updater_task.result == 0) {
            if (update_data->isAvailable) {
                ui_enable_element((UIElement *) updater_button);
                // ui_label_set_text(error_label, update_data->releaseTitle);
                populate_info();
                checkedForUpdate = true;
            } else output_log("no updates found");
        } else {
            output_log(handle_updater_error_codes(updater_task.result));
            ui_label_set_text(error_label, handle_updater_error_codes(updater_task.result));
        }
        updater_task.finished = false;
    }
}

const UIScreenDefPair updater_def = {
    .name = "updater",
    .btm = {
        .path = "romfs:/menus/updater.txt",
        .init = updater_init,
        .update = updater_update,
        .action_list = {
            .action_count = ARRAY_LEN(updater_actions),
            .actions = updater_actions
        }
    },
};