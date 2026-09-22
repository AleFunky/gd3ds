#include <3ds.h>
#include <citro2d.h>

#include "math_helpers.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "state.h"
#include "save/config.h"
#include "utils/folders.h"
#include "level_loading.h"

#include "menus/core/common_setters.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_button.h"

#include "menus/settings_hub/settings.h"
#include "menus/settings_hub/info_card.h"
#include "menus/creator_menu/external/external_levels.h"
#include "menus/creator_menu/external/external_popup.h"
#include "menus/main_menu.h"
#include "menus/level_select.h"
#include "menus/gameplay.h"

#include "fonts/bigFont.h"

static bool reload_pending;
static char reload_path[320];

char current_path[320] = { 0 };
char last_path[320] = { 1 };

typedef struct {
    char path[256];
    bool is_dir;
} LevelCardData;

static void open_folder(UIElement *e, const UIPropertyList* args);

static void open_external_popup(UIElement *e, const UIPropertyList* args) {
    LevelCardData *entry = e->userdata;
    strcpy(state.custom_level_path, entry->path);
    ui_stack_push(&external_popup_def, ANIM_ZOOM_SUBTLE, ANIM_ZOOM_SUBTLE, PUSH_NEXT);
}

void load_level_folder(char *folder, UIScreen *s) {
    UIList *list = (UIList *) ui_get_element_by_tag(s, "list");
    UILabel *path_label = (UILabel *) ui_get_element_by_tag(s, "path");
    
    ui_run_func_on_tag(s, "no_levels", ui_disable_element);

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

            card = (UIElement *) ui_create_rectangle(s);

            if (card) {
                ui_rectangle_set_color((UIRectangle *) card, color);
                ui_element_set_size(card, 0, 28);
                
                UIButton *button = ui_create_button(s);
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

                UIImage *icon = ui_create_image(s);
                if (icon) {
                    ui_image_set_image(icon, (entry->is_dir ? 320 : 420), 0);
                    ui_element_set_position((UIElement *) icon, -list_width + 15, 0);
                    ui_element_set_scale((UIElement *) icon, 0.58f);

                    ui_element_add_child(card, (UIElement *) icon);
                }

                // Name
                UILabel *label = ui_create_label(s);
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
            ui_run_func_on_tag(s, "no_levels", ui_enable_element);
        }
    } else {
        ui_run_func_on_tag(s, "no_levels", ui_enable_element);
    }

    strncpy(last_path, current_path, sizeof(last_path));
}

static void action_go_back(UIElement *e, const UIPropertyList *args) {
    if (strlen(current_path) > 0) {
        go_back_directory(current_path);
        load_level_folder(current_path, e->screen);
    }
}

// Go my warning suppresion gadget
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

static void open_folder(UIElement *e, const UIPropertyList* args) {
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

static UIActionDef external_actions[] = {
    {"go_back", action_go_back },
};

static void external_levels_init(UIScreen *s) {
    load_level_folder(current_path, s);

    play_menu_song();
}

static void external_levels_update(UIScreen *s, UIInput *i) {
    if (reload_pending) {
        load_level_folder(reload_path, s);

        reload_pending = false;
    }
}

const UIScreenDefPair external_def = {
    .name = "external_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/external/external_levels_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/external/external_levels.txt",
        .init = external_levels_init,
        .update = external_levels_update,
        .action_list = {
            .action_count = ARRAY_LEN(external_actions),
            .actions = external_actions
        }
    }
};