#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "level/main_levels.h"
#include "main.h"
#include "menus/core/ui_element.h"
#include "menus/settings_hub/info_card.h"
#include "state.h"
#include "save/saving.h"
#include "utils/string_helpers.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/creator_menu/external/external_levels.h"
#include "menus/settings_hub/songs.h"
#include "menus/creator_menu/external/external_level_infobox.h"
#include "menus/creator_menu/external/external_popup.h"
#include "utils/utils.h"

#include "fonts/chatFont.h"

static UIScreen *screen;
static UIScreen *screen_top;

static UILabel *level_name;
static UILabel *creator_name;
static UILabel *description;

static UILabel *downloads_label;
static UILabel *likes_label;
static UILabel *stars_label;
static UILabel *level_id_label;
static UILabel *song_label;

static UIImage *difficulty_face;
static UIImage *like_image;

static UIProgressBar *normal_progress;
static UILabel *normal_progress_val;
static UIProgressBar *practice_progress;
static UILabel *practice_progress_val;

static Thread thread;

static int stars_num = 0;

const int difficulty_stars[MAX_STARS + 1] = {
    NA_FACE,
    AUTO_FACE,
    EASY_FACE,
    NORMAL_FACE,
    HARD_FACE,
    HARD_FACE,
    HARDER_FACE,
    HARDER_FACE,
    INSANE_FACE,
    INSANE_FACE,
    DEMON_FACE
};

static char *gmd = NULL;


static void open_level(UIElement *e, const UIPropertyList *args) {
    state.custom_level = true;

    stop_mp3();
    playing_menu_loop = false;
    ui_stack_push_game_state(STATE_GAME);
}

static UIActionDef external_popup_actions[] = {
    { "play", open_level },
};

static void set_progress() {
    normal_progress->value = current_level_entry->data.normal_progress;
    practice_progress->value = current_level_entry->data.practice_progress;

    char normal[32];
    char practice[32];
    snprintf(normal, sizeof(normal), "%d%%", current_level_entry->data.normal_progress);
    snprintf(practice, sizeof(practice), "%d%%", current_level_entry->data.practice_progress);

    ui_label_set_text(normal_progress_val, normal);
    ui_label_set_text(practice_progress_val, practice);
}

static void set_name_creator(char *gmd) {
    char *name = extract_gmd_key((const char *) gmd, "k2", "s");
    if (name) {
        ui_label_set_text(level_name, name);
        snprintf(level_info.level_name, sizeof(level_info.level_name), "%s", name);
        free(name);
    } else {
        snprintf(level_info.level_name, sizeof(level_info.level_name), "%s", default_name);
    }

    char tmp[512];
    char *creator = extract_gmd_key((const char *) gmd, "k5", "s");
    if (creator) {
        snprintf(tmp, sizeof(tmp), "By %s", creator);
        ui_label_set_text(creator_name, tmp);
        snprintf(level_info.creator_name, sizeof(level_info.creator_name), "%s", creator);
        free(creator);
    } else {
        snprintf(level_info.creator_name, sizeof(level_info.creator_name), "%s", default_name);
    }

    // Load data
    char file[516];
    snprintf(file, sizeof(file), "ext_%s_%s", level_info.level_name, level_info.creator_name);
    
    current_level_entry = get_or_add_level_to_external_file(&external_file, file);
    
    set_progress();
}

static void set_description(char *gmd) {
    char *desc = extract_gmd_key((const char *)gmd, "k3", "s");
    if (!desc)
        return;

    bool double_decode = strncmp(gmd, "<?xml version", 13) != 0;

    fix_base64_url(desc);
    unsigned char *decoded = malloc(strlen(desc) + 1);
    int decoded_len = base64_decode(desc, decoded);

    if (decoded_len > 0) {
        decoded[decoded_len] = '\0';

        // If the gmd doesn't have <?xml version at the start, the description is encoded twice in base 64
        if (double_decode) {
            fix_base64_url((char *)decoded);

            unsigned char *decoded2 = malloc(decoded_len + 1);
            int decoded2_len = base64_decode((char *)decoded, decoded2);

            free(decoded);

            if (decoded2_len > 0) {
                decoded2[decoded2_len] = '\0';

                char *wrapped = wrap_text(&chatFont_fontCharset, description->base.scaleX, (char *)decoded2, MAX_DESCRIPTION_WIDTH);

                ui_label_set_text(description, wrapped);
            }

            free(decoded2);
        // Normal description, as it should be
        } else {
            char *wrapped = wrap_text(&chatFont_fontCharset, description->base.scaleX, (char *)decoded, MAX_DESCRIPTION_WIDTH);

            ui_label_set_text(description, wrapped);

            free(decoded);
        }
    } else {
        free(decoded);
    }

    free(desc);
}

static void set_downloads_likes(char *gmd) {
    char *downloads = extract_gmd_key((const char *) gmd, "k11", "i");
    if (downloads) {
        int download_count = atoi(downloads);
        char *truncated_downloads = truncate_number(download_count);
        free(downloads);

        if (!truncated_downloads) return;

        ui_label_set_text(downloads_label, truncated_downloads);    
        free(truncated_downloads);
    }

    char *likes = extract_gmd_key((const char *) gmd, "k22", "i");
    if (likes) {
        int like_count = atoi(likes);
        char *truncated_likes = truncate_number(like_count);
        free(likes);

        if (!truncated_likes) return;

        ui_label_set_text(likes_label, truncated_likes);

        if (like_count < 0) {
            ui_image_set_image(like_image, DISLIKE_ICON, 0);
        }
        free(truncated_likes);
    }
}

static void set_stars(char *gmd) {
    char *stars = extract_gmd_key((const char *) gmd, "k26", "i");
    if (stars) {
        ui_label_set_text(stars_label, stars);
        stars_num = atoi(stars);
        
        // Clamp
        if (stars_num > MAX_STARS) {
            stars_num = 0;
        }

        if (current_level_entry->data.stars != stars_num) {
            current_level_entry->data.stars = stars_num;
        }

        free(stars);
    } else {
        stars_num = 0;
    }
}

static void set_level_id(char *gmd) {
    char tmp[512];

    char *level_id = extract_gmd_key((const char *) gmd, "k1", "i");
    if (level_id) {
        snprintf(tmp, sizeof(tmp), "<#3F2215>ID: %s</>", level_id);
        free(level_id);

        ui_label_set_text(level_id_label, tmp);
    }
}

static void set_song_id(char *gmd) {
    char tmp[512];

    int custom_song_id = -1;
    int song_id = 0;

    char *gmd_custom_song_id = extract_gmd_key((const char *) gmd, "k45", "i");
    if (gmd_custom_song_id) {
        custom_song_id = atoi(gmd_custom_song_id); // Custom song id
        free(gmd_custom_song_id);
    }
    char *gmd_song_id = extract_gmd_key((const char *) gmd, "k8", "i");
    if (gmd_song_id) {
        song_id = atoi(gmd_song_id); // Official song id
        free(gmd_song_id);
    }

    if (custom_song_id > 0) {
        if (check_song(custom_song_id)) {
            snprintf(tmp, sizeof(tmp), "Using song: %d.mp3", custom_song_id);
        } else {
            snprintf(tmp, sizeof(tmp), "Using song: %d.mp3 (NOT FOUND)", custom_song_id);
        }
    } else {
        char *song_name = "Unknown";
        if (song_id >= 0 && song_id < current_main_level_pack->count) {
            song_name = current_main_level_pack->levels[song_id].song_data.title;
        }
        snprintf(tmp, sizeof(tmp), "Using song: %s", song_name);
    }

    ui_label_set_text(song_label, tmp);
}

static void set_difficulty() {
    int face = difficulty_stars[0];
    if (IN_BOUNDS(stars_num, difficulty_stars)) {
        face = difficulty_stars[stars_num];
    }
    ui_image_set_image(difficulty_face, face, 0);
}


static int load_gmd(GenericTask *task) {
    size_t out_size;
    gmd = read_file(state.custom_level_path, &out_size);
    
    if (gmd == NULL) return 1;

    set_name_creator(gmd);
    set_description(gmd);
    set_downloads_likes(gmd);
    set_stars(gmd);
    set_difficulty();
    set_level_id(gmd);
    set_song_id(gmd);

    return 0;
}

static GenericTask task = {
    .func = load_gmd
};

static void external_popup_init_top(UIScreen *s) {
    screen_top = s;
    level_name      = (UILabel *) ui_get_element_by_tag(screen_top, "levelname");
    creator_name    = (UILabel *) ui_get_element_by_tag(screen_top, "creatorname");
    description     = (UILabel *) ui_get_element_by_tag(screen_top, "description");
    downloads_label = (UILabel *) ui_get_element_by_tag(screen_top, "downloadcount");
    likes_label     = (UILabel *) ui_get_element_by_tag(screen_top, "likecount");
    stars_label     = (UILabel *) ui_get_element_by_tag(screen_top, "stars");
    difficulty_face = (UIImage *) ui_get_element_by_tag(screen_top, "difficultyface");
    level_id_label  = (UILabel *) ui_get_element_by_tag(screen_top, "levelid");
    like_image      = (UIImage *) ui_get_element_by_tag(screen_top, "likeimage");
    ui_run_func_on_tag(screen_top, "info", ui_disable_element);
}

void show_level_load_error_message() {
    // Level gave error
    char tmp[512];

    int message_id = level_result - 1;
    char *message = "Ultra unknown error.";
    if (IN_BOUNDS(message_id, error_strings)) {
        message = (char *) error_strings[message_id]; 
    }

    snprintf(tmp, sizeof(tmp), "%s", message);

    InfoCardData *ext_error_data = malloc(sizeof(InfoCardData));
    if(!ext_error_data) return;

    ext_error_data->text = strdup(tmp);
    ext_error_data->copied = true;
    ext_error_data->title = strdup("Error");
    ext_error_data->customTitle = true;

    ui_stack_push(&info_card_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    ui_stack_push_data(ext_error_data);

    level_result = 0;
}

void external_popup_update(UIScreen *s, UIInput *u) {
    if (level_result) {
        show_level_load_error_message();
    }

    if (exiting_level) {
        set_progress();
        exiting_level = false;
    }
}

void external_popup_init(UIScreen *s) {
    screen = s;
    song_label            = (UILabel *) ui_get_element_by_tag(screen, "songid");
    normal_progress       = (UIProgressBar *) ui_get_element_by_tag(screen, "normalprogress");
    normal_progress_val   = (UILabel *) ui_get_element_by_tag(screen, "normalprogressvalue");
    practice_progress     = (UIProgressBar *) ui_get_element_by_tag(screen, "practiceprogress");
    practice_progress_val = (UILabel *) ui_get_element_by_tag(screen, "practiceprogressvalue");

    ui_progress_bar_set_tint(normal_progress, C2D_Color32(0, 255, 0, 255));
    ui_progress_bar_set_tint(practice_progress, C2D_Color32(0, 255, 255, 255));

    thread = create_generic_thread(&task);
    ui_run_func_on_tag(screen, "info", ui_disable_element);

    free(gmd);
}

void external_popup_update_top(UIScreen *s, UIInput *u) {
    if (task.finished) {
        ui_run_func_on_tag(screen, "spinner", ui_disable_element);
        ui_run_func_on_tag(screen, "info", ui_enable_element);
        ui_run_func_on_tag(screen_top, "info", ui_enable_element);
        ui_run_func_on_tag(screen_top, "spinner", ui_disable_element);

        task.finished = false;
    }
}

const UIScreenDefPair external_popup_def = {
    .name = "external_popup",
    .top = {
        .path = "romfs:/menus/creator_menu/external/external_pop_up_top.txt",
        .init = external_popup_init_top,
        .update = external_popup_update_top
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/external/external_pop_up.txt",
        .init = external_popup_init,
        .update = external_popup_update,
        .action_list = {
            .action_count = ARRAY_LEN(external_popup_actions),
            .actions = external_popup_actions
        }
    }
};