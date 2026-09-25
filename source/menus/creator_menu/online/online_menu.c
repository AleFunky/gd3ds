#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <citro2d.h>
#include "3ds/thread.h"

#include "level/main_levels.h"
#include "level_loading.h"
#include "main.h"
#include "graphics.h"
#include "menus/creator_menu/online/two_option_pop_up.h"
#include "mp3_player.h"
#include "save/saving.h"
#include "state.h"

#include "utils/folders.h"
#include "utils/network.h"
#include "utils/server_utils.h"
#include "utils/string_helpers.h"

#include "menus/core/ui_screen.h"
#include "menus/core/common_setters.h"
#include "menus/core/ui_element.h"
#include "menus/components/ui_window_button.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_particle.h"
#include "menus/creator_menu/online/online_level_menu.h"
#include "menus/creator_menu/external/external_popup.h"
#include "menus/creator_menu/search_menu.h"
#include "menus/creator_menu/online/online_menu.h"
#include "menus/settings_hub/info_card.h"
#include "menus/settings_hub/songs.h"
#include "menus/main_menu.h"

#include "fonts/bigFont.h"
#include "fonts/goldFont.h"
#include "utils/utils.h"

int curr_search_id;
int redownload = false;

static UILabel *error_label;
static UILabel *page_info_label;

static UISpinner *spinner;

static UIList *list;

static Thread thread;

static GenericTask search_task = {
    .func = search_levels
};

int search_result = -2;

const int demon_faces[6] = {
    NA_FACE,
    EASY_DEMON_FACE,
    MEDIUM_DEMON_FACE,
    HARD_DEMON_FACE,
    INSANE_DEMON_FACE,
    EXTREME_DEMON_FACE
};

const int difficulty_faces[6] = {
    NA_FACE,
    EASY_FACE,
    NORMAL_FACE,
    HARD_FACE,
    HARDER_FACE,
    INSANE_FACE
};

const int epics[4] = {
    0,
    MYTHIC_GLOW,
    EPIC_GLOW,
    LEGENDARY_GLOW,
};

static void action_clear_data(UIElement* e, const UIPropertyList *args) {
    if (search_entries) {
        for (int i = 0; i < searchEntriesLength; i++) {
            free(search_entries[i].description);
        }
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
    if(page_entry) {
        free(page_entry);
        page_entry = NULL;
    }
}

static void action_open_online_level_menu(UIElement* e, const UIPropertyList *args) {
    OnlineCardData *entry = e->userdata;
    curr_search_id = entry->entryId;
    online_menu_level_id = entry->levelId;
    redownload = entry->redownload; 
    ui_stack_push(&online_level_menu_def, ANIM_NONE, ANIM_NONE, PUSH_ROOT);
}

static void update_arrows(UIScreen *s) {
    if (searchEntriesLength == page_entry->amount) ui_run_func_on_tag(s, "nextpage", ui_enable_element); else ui_run_func_on_tag(s, "nextpage", ui_disable_element);
    if ((filters.currentPage) >= 1) ui_run_func_on_tag(s, "prevpage", ui_enable_element); else ui_run_func_on_tag(s, "prevpage", ui_disable_element);

    int total = page_entry->totalPages * page_entry->amount - 1;
    int current = page_entry->currentOffset + 1;
    int to = page_entry->currentOffset + page_entry->amount;

    // Cap to amount
    if (current > total) {
        current = total;
    }

    // Cap to amount
    if (to > total) {
        to = total;
    }

    char pageInfo[42];
    snprintf(pageInfo, sizeof(pageInfo), "%d to %d of %d", current, to, total);
    ui_label_set_text(page_info_label, pageInfo);
}

static void action_change_page(UIElement* e, const UIPropertyList *args) {
    filters.currentPage += ui_prop_int(&e->custom_properties, "page", 0);
    search_needs_refresh = true;
    ui_run_func_on_tag(e->screen, "nextpage", ui_disable_element);
    ui_run_func_on_tag(e->screen, "prevpage", ui_disable_element);
    thread = create_generic_thread(&search_task);
    ui_enable_element((UIElement *) spinner);
    if (list) ui_list_reset(list);
}

static void action_version_warning(UIElement* e, const UIPropertyList *args){
    VersionWarningData *e_data = e->userdata;
    if(!e_data) return;

    char buffer[256];
    if (e_data->wasUpdated) {
        snprintf(buffer, sizeof(buffer), "This level was uploaded <#60abef>before or in</> %.1f,\nbut was updated in a later version. It\n<#ffa54b>might not be playable</>.", GD_VERSION);
    } else {
        snprintf(buffer, sizeof(buffer), "This level was uploaded <#60abef>after</> %.1f.\nIt will most likely <#f25149>not be playable</>.", GD_VERSION);
    }

    InfoCardData *data = malloc(sizeof(InfoCardData));
    if(!data) return;

    data->text = strdup(buffer);
    data->title = strdup("Version Warning");
    data->customTitle = true;
    data->copied = true;

    ui_stack_push(&info_card_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    ui_stack_push_data(data);
}

static UIActionDef online_actions[] = {
    { "open_level_menu", action_open_online_level_menu },
    { "changepage", action_change_page },
    { "clear_data", action_clear_data },
    { "open_version_warning", action_version_warning }
};

static void populate_list() {
    ui_disable_element((UIElement *) spinner);
    ui_list_reset(list);
    for (int i = 0; i < searchEntriesLength; i++) {
        char tmp_name[32];
        char tmp_creator[36];
        char tmp_song[256];

        SearchEntry *entry = &search_entries[i];

        strncpy(tmp_name, entry->name, sizeof(tmp_name) - 1);
        
        // Get creator name
        char *creator_name = "By -";

        if (entry->creatorIndex < creatorEntriesLength) {
            creator_name = creator_entries[entry->creatorIndex].creatorName;
        }

        snprintf(tmp_creator, sizeof(tmp_creator), "<#%s>By %s</>", (creator_entries[entry->creatorIndex].userId == 0) ? "5AFFFF" : "FFFFFF", creator_name);

        // Get song name
        char *song_name = "Unknown";

        if (entry->songId != 0) {
            if (entry->songIndex < songEntriesLength) {
                song_name = song_entries[entry->songIndex].songTitle;
            }
        } else if (entry->songIndex != -1) {
            if (entry->mainSongId >= 0 && entry->mainSongId < current_main_level_pack->count) {
                song_name = (char *) current_main_level_pack->levels[entry->mainSongId].song_data.title;
            }
        }

        const char *song_color = (entry->songId != 0 ? "<#f982ff>" : "<#27d2ff>");

        snprintf(tmp_song, sizeof(tmp_song) - 1, "%s%s", song_color, song_name);

        truncate_filename(tmp_song, 35);

        float list_width = list->base.w * 0.5f;

        UIElement *card = (UIElement *)ui_create_rectangle(&default_screen);

        if (card) {
            ui_rectangle_set_color((UIRectangle *)card, (i & 1 ? C2D_Color32(194, 114, 62, 255) : C2D_Color32(161, 88, 48, 255)));
            ui_element_set_size(card, 0, 60);

            // Level name
            UILabel *name_label = ui_create_label(&default_screen);

            if (name_label) {
                ui_label_set_text(name_label, tmp_name);
                ui_element_set_position((UIElement *)name_label, -list_width + 48, -17);
                ui_element_set_scale((UIElement *)name_label, 0.54f);
                name_label->base.w = 120;
                ui_element_add_child(card, (UIElement *)name_label);
            }

            // Level creator
            UILabel *creator_label = ui_create_label(&default_screen);
            if (creator_label) {
                ui_label_set_text(creator_label, tmp_creator);
                ui_element_set_position((UIElement *)creator_label, -list_width + 48, -4.5f);
                ui_element_set_scale((UIElement *)creator_label, 0.45f);

                creator_label->font = 2;

                ui_element_add_child(card, (UIElement *)creator_label);
            }

            // copy icon
            UIImage *collaboration_icon = ui_create_image(&default_screen);
            if (collaboration_icon && entry->originalId != 0) {
                ui_image_set_image(collaboration_icon, 213, 0);
                ui_element_set_position((UIElement *)collaboration_icon, -list_width + 48 + get_text_length(&goldFont_fontCharset, 0.45f, false, creator_name) + 27, -4.5f);
                ui_element_set_scale((UIElement *)collaboration_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)collaboration_icon);
            }

            // high object count icon
            UIImage *high_object_icon = ui_create_image(&default_screen);
            if (high_object_icon && (entry->objCount >= (is_N3DS ? 44000 : 14000))) {
                ui_image_set_image(high_object_icon, 362, 0);
                ui_element_set_position((UIElement *)high_object_icon, -list_width + 48 + get_text_length(&goldFont_fontCharset, 0.45f, false, creator_name) + 27 + ((entry->originalId != 0) ? 11 : 0), -4.5f);
                ui_element_set_scale((UIElement *)high_object_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)high_object_icon);
            }

            // Level song
            UILabel *song_label = ui_create_label(&default_screen);
            if (song_label) {
                ui_label_set_text(song_label, tmp_song);
                ui_element_set_position((UIElement *)song_label, -list_width + 48, 7);
                ui_element_set_scale((UIElement *)song_label, 0.35f);
                song_label->base.w = 130;
                ui_element_add_child(card, (UIElement *)song_label);
            }

            // Level length
            UILabel *length_label = ui_create_label(&default_screen);
            if (length_label) {
                char *level_length = "Unkn.";
                if (IN_BOUNDS(entry->lengthNum, level_lengths)) {
                    level_length = (char *) level_lengths[entry->lengthNum];
                }

                ui_label_set_text(length_label, level_length);
                ui_element_set_position((UIElement *)length_label, -list_width + 60, 19.3f);
                ui_element_set_scale((UIElement *)length_label, 0.35f);
                length_label->base.w = 30;
                ui_element_add_child(card, (UIElement *)length_label);
            }

            // Downloads
            UILabel *download_value = ui_create_label(&default_screen);
            if (download_value) {
                char *tmp_value = truncate_number(entry->downloads);

                ui_label_set_text(download_value, tmp_value);
                ui_element_set_position((UIElement *)download_value, -list_width + 110, 19.3f);
                ui_element_set_scale((UIElement *)download_value, 0.35f);

                ui_element_add_child(card, (UIElement *)download_value);
            }

            // Likes
            UILabel *like_value = ui_create_label(&default_screen);
            if (like_value) {
                char *tmp_value = truncate_number(entry->likes);

                ui_label_set_text(like_value, tmp_value);
                ui_element_set_position((UIElement *)like_value, -list_width + 160, 19.3f);
                ui_element_set_scale((UIElement *)like_value, 0.35f);

                ui_element_add_child(card, (UIElement *)like_value);
            }

            // Stars
            UILabel *star_value = ui_create_label(&default_screen);
            if (star_value && entry->stars > 0) {
                char tmp_value[16];

                snprintf(tmp_value, sizeof(tmp_value), "%d", entry->stars);

                ui_label_set_text(star_value, tmp_value);
                ui_element_set_position((UIElement *)star_value, -list_width + 22, 20.3f);
                ui_element_set_scale((UIElement *)star_value, 0.35f);

                star_value->alignment = 1.f;

                ui_element_add_child(card, (UIElement *)star_value);
            }

            UIImage *featured_glow = ui_create_image(&default_screen);
            if (featured_glow && entry->featureScore > 0) {
                int featured_id = 0;
                int yOffset = 0;

                if(entry->epic > 0 && IN_BOUNDS(entry->epic, epics)){
                    featured_id = (gdps ? SUPER_GLOW : epics[entry->epic]);
                    yOffset = (gdps ? -1 : -2);
                } else if(entry->featureScore > 0) {
                    featured_id = FEATURED_GLOW;
                }

                if(!entry->stars) yOffset += 4;

                ui_image_set_image(featured_glow, featured_id, 0);
                ui_element_set_position((UIElement *)featured_glow, -list_width + 23 + (gdps ? 0.3 : 0), -8.5f + yOffset);
                ui_element_set_scale((UIElement *)featured_glow, 0.82f);

                ui_element_add_child(card, (UIElement *)featured_glow);
            }

            UIImage *difficulty_face = ui_create_image(&default_screen);
            if (difficulty_face) {
                int difficulty_id = NA_FACE;

                if(entry->isAuto) {
                    difficulty_id = AUTO_FACE;
                } else if (entry->isDemon && gdps) {
                    difficulty_id = 258;
                } else if(entry->isDemon && IN_BOUNDS(entry->difficulty, demon_faces)) {
                    difficulty_id = demon_faces[entry->difficulty];
                } else if (IN_BOUNDS(entry->difficulty, difficulty_faces)) {
                    difficulty_id = difficulty_faces[entry->difficulty];
                }

                ui_image_set_image(difficulty_face, difficulty_id, 0);
                ui_element_set_position((UIElement *)difficulty_face, -list_width + 23, (entry->stars > 0) ? -4 : 0);
                ui_element_set_scale((UIElement *)difficulty_face, 0.82f);

                ui_element_add_child(card, (UIElement *)difficulty_face);
            }

            UIImage *star_icon = ui_create_image(&default_screen);
            if (star_icon && entry->stars > 0) {
                ui_image_set_image(star_icon, 170, 0);
                ui_element_set_position((UIElement *)star_icon, -list_width + 29, 20);
                ui_element_set_scale((UIElement *)star_icon, 0.71f);

                ui_element_add_child(card, (UIElement *)star_icon);
            }

            UIImage *length_icon = ui_create_image(&default_screen);
            if (length_icon) {
                ui_image_set_image(length_icon, 197, 0);
                ui_element_set_position((UIElement *)length_icon, -list_width + 53, 20);
                ui_element_set_scale((UIElement *)length_icon, 0.5f);

                ui_element_add_child(card, (UIElement *)length_icon);
            }

            UIImage *download_icon = ui_create_image(&default_screen);
            if (download_icon) {
                ui_image_set_image(download_icon, 163, 0);
                ui_element_set_position((UIElement *)download_icon, -list_width + 103, 20);
                ui_element_set_scale((UIElement *)download_icon, 0.7f);

                ui_element_add_child(card, (UIElement *)download_icon);
            }

            UIImage *like_icon = ui_create_image(&default_screen);
            if (like_icon) {
                ui_image_set_image(like_icon, 97, 0);
                ui_element_set_position((UIElement *)like_icon, -list_width + 153, 19);
                ui_element_set_scale((UIElement *)like_icon, 0.5f);

                if (entry->likes < 0) {
                    ui_image_set_image(like_icon, DISLIKE_ICON, 0);
                }

                ui_element_add_child(card, (UIElement *)like_icon);
            }

            // Exclamation mark
            float version = derive_gj_version(entry->gameVersion);
            if (version > GD_VERSION) {
                UIButton *v_warn_button = ui_create_button(&default_screen);
                if (v_warn_button) {
                    VersionWarningData *data = malloc(sizeof(*data));
                    data->version = version;
                    if (entry->levelId <= LAST_GD_VERSION_ID) {
                        // Made in or before GD_VERSION but updated after it
                        data->wasUpdated = true;
                        ui_button_set_image(v_warn_button, 426, 0);
                    } else {
                        data->wasUpdated = false;
                        ui_button_set_image(v_warn_button, 315, 0);
                    }
                    float tmp = get_text_length(&bigFont_fontCharset, 0.54f, false, tmp_name);
                    // level name width is capped at 130 so we replicate that here
                    if (tmp > 120) tmp = 120;
                    ui_element_set_position((UIElement *)v_warn_button, -list_width + 48 + tmp + 10, -17);
                    ui_element_set_scale((UIElement *)v_warn_button, 0.3f);
                    v_warn_button->base.actions = parse_actions(
                        "open_menu[screen=\"info_card\" btm_anim=\"zoom\"],open_version_warning", 
                        online_actions, 
                        ARRAY_LEN(online_actions), 
                        &v_warn_button->base.action_count
                    );
                    ui_element_set_userdata((UIElement *)v_warn_button, data);
                    ui_element_add_child(card, (UIElement *)v_warn_button);
                }
            }

            UIWindowButton *button = ui_create_window_button(&default_screen);
            if (button) {
                // Store in the user data
                OnlineCardData *data = malloc(sizeof(*data));

                data->entryId = i;
                data->levelId = entry->levelId;
                data->redownload = false;

                SavedLevelDataEntry *saved_data = get_saved_level_data(entry->levelId);
                if (!saved_data) {
                    ui_window_button_set_style(button, 10);
                    ui_button_set_text((UIButton *)button, "Get");
                    button->base.textScale = 0.48f;
                } else {
                    if (saved_data->search_entry.levelVersion != entry->levelVersion) {
                        ui_window_button_set_style(button, 14);
                        ui_button_set_text((UIButton *)button, "Update");
                        button->base.textScale = 0.30f;
                        data->redownload = true;
                    } else {
                        ui_window_button_set_style(button, 5);
                        ui_button_set_text((UIButton *)button, "View");
                        button->base.textScale = 0.48f;
                    }
                }


                ui_element_set_position((UIElement *)button, list_width - 32, 0);
                ui_element_set_size((UIElement *)button, 48, 28);
                ui_element_set_action((UIElement *)button, action_open_online_level_menu);
                ui_element_set_userdata((UIElement *) button, data);
                ui_element_add_child(card, (UIElement *)button);
            }

            LevelDataEntry *level_data = get_online_level_data(entry->levelId);
            if (level_data && level_data->data.normal_progress == 100) {
                UIImage *completed_icon = ui_create_image(&default_screen);
                if (completed_icon) {
                    ui_image_set_image(completed_icon, 40, 0);
                    ui_element_set_position((UIElement *)completed_icon, list_width - 62, -17);
                    ui_element_set_scale((UIElement *)completed_icon, 0.7f);

                    ui_element_add_child(card, (UIElement *)completed_icon);
                }
            }

            ui_list_add(list, card);
        }
    }  
}

static void handle_errors(int code) {
    ui_disable_element((UIElement *) spinner);
    char temp[64];
    switch (code) {
        case -2:
            //me when gdps returns -2 when no levels are found
            if (gdps) break;
            ui_label_set_text(error_label, "An unknown error has\n occured.");
            break;
        case -1:
            break;
        case 6:
        case 7:   
            ui_label_set_text(error_label, "No\n<#41e24e>Internet</> connection!");
            break;
        case 28:
            ui_label_set_text(error_label, "Connection timed out!");
            break;
        case 42:
            break;
        default:
            snprintf(temp, sizeof(temp), "An unknown error has\noccurred.\n\nError code: %d", code);
            ui_label_set_text(error_label, temp);

    }
}

static void online_menu_init(UIScreen *s) {
    spinner = (UISpinner *) ui_get_element_by_tag(s, "spinner");

    list = (UIList *) ui_get_element_by_tag(s, "list");
    error_label = (UILabel *)ui_get_element_by_tag(s, "errorLabel");
    page_info_label = (UILabel *)ui_get_element_by_tag(&s->pair->screens[SCREEN_TOP], "pageinfo");

    ui_run_func_on_tag(s, "nextpage", ui_disable_element);
    ui_run_func_on_tag(s, "prevpage", ui_disable_element);

    search_result = -2;
    
    if (search_needs_refresh) {
        thread = create_generic_thread(&search_task);
    } else {
        if (list) { // No errors
            populate_list();
            update_arrows(s);
        }
    }

    play_menu_song();
}

static void online_menu_update(UIScreen *s, UIInput *i) {
    // Run when finished
        if (search_task.finished) {
            search_result = search_task.result;
            // Handle result
            if (search_result != 0 && search_needs_refresh) {
                handle_errors(search_result);
            } else if (list) { // No errors
                populate_list();
                update_arrows(s);
                search_needs_refresh = false;
            }
            search_task.finished = false;
        }
}

static void online_menu_exit() {
    if (search_task.running) {
        search_task.cancelled = true;
        threadJoin(thread, U64_MAX);
    }
    
    save_current_save_file(LEVEL_LIST_ONLINE);
}


const UIScreenDefPair online_def = {
    .name = "online_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_levels_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_levels.txt",
        .init = online_menu_init,
        .update = online_menu_update,
        .exit = online_menu_exit,
        .action_list = {
            .action_count = ARRAY_LEN(online_actions),
            .actions = online_actions
        }
    }
};
