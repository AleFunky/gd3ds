#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "main.h"
#include "external_level_infobox.h"
#include "menus/external_levels.h"

#include "save/saving.h"

#include "fonts/chatFont.h"

#include "state.h"

static bool yes_exit = false;

static bool in_infobox = false;

static UIScreen screen_top = {
};
static UIScreen screen = {
    .isBottom = true,
};

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

static int stars_num = 0;

#define MAX_STARS 10

#define NA_FACE     251
#define AUTO_FACE   267
#define EASY_FACE   252
#define NORMAL_FACE 253
#define HARD_FACE   254
#define HARDER_FACE 255
#define INSANE_FACE 256
#define DEMON_FACE  258

#define DISLIKE_ICON 56

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

#define MAX_NAME_WIDTH 200
#define MAX_DESCRIPTION_WIDTH 300
#define MAX_DOWNLOADS_WIDTH 60
#define MAX_LIKES_WIDTH 60

void external_popup_enter_from_level();

void exit_external_popup(UIElement* e) {
    yes_exit = true;
}

static void open_level(UIElement *e) {
    play_sfx(&play_sound, 1);

    state.custom_level = true;

    set_fade_status(FADE_STATUS_OUT);
    
    external_start_level = true; 
    external_popup_enter_from_level();
}

static void open_info(UIElement *e) {
    in_infobox = true;
    external_level_infobox_init();
}

static UIAction actions[] = {
    { "exit", exit_external_popup },
    { "play", open_level },
    { "info", open_info }
};

static void set_name_creator(char *gmd) {
    char *name = extract_gmd_key((const char *) gmd, "k2", "s");
    if (name) {
        ui_label_set_scale_from_width(level_name, name, MAX_NAME_WIDTH);
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
    load_level_progress(file);
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
    char tmp[512];

    char *downloads = extract_gmd_key((const char *) gmd, "k11", "i");
    if (downloads) {
        snprintf(tmp, sizeof(tmp), "%s", downloads);
        free(downloads);
        
        ui_label_set_scale_from_width(downloads_label, tmp, MAX_DOWNLOADS_WIDTH);

        ui_label_set_text(downloads_label, tmp);
    }

    char *likes = extract_gmd_key((const char *) gmd, "k22", "i");
    if (likes) {
        snprintf(tmp, sizeof(tmp), "%s", likes);

        ui_label_set_scale_from_width(likes_label, tmp, MAX_LIKES_WIDTH);

        ui_label_set_text(likes_label, tmp);

        if (atoi(likes) < 0) {
            ui_image_set_image(like_image, DISLIKE_ICON, 0);
        }
        free(likes);
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

        if (level_data.stars != stars_num) {
            level_data.stars = stars_num;
            save_level_progress();
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
        snprintf(tmp, sizeof(tmp), "Using song: %s", main_levels[song_id].level_name);
    }

    ui_label_set_text(song_label, tmp);
}

static void set_progress() {
    normal_progress->value = level_data.normal_progress;
    practice_progress->value = level_data.practice_progress;

    char normal[32];
    char practice[32];
    snprintf(normal, sizeof(normal), "%d%%", level_data.normal_progress);
    snprintf(practice, sizeof(practice), "%d%%", level_data.practice_progress);

    ui_label_set_text(normal_progress_val, normal);
    ui_label_set_text(practice_progress_val, practice);
}

static void set_difficulty() {
    ui_image_set_image(difficulty_face, difficulty_stars[stars_num], 0);
}

void external_popup_enter_from_level() {
    finish_animation(&screen_top);
    finish_animation(&screen);
}

void external_popup_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_pop_up.txt");
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/external_pop_up_top.txt");

    ui_screen_open(&screen, ANIM_ZOOM_SUBTLE);
    ui_screen_open(&screen_top, ANIM_ZOOM_SUBTLE);

    level_name      = (UILabel *) ui_get_element_by_tag(&screen_top, "levelname");
    creator_name    = (UILabel *) ui_get_element_by_tag(&screen_top, "creatorname");
    description     = (UILabel *) ui_get_element_by_tag(&screen_top, "description");
    downloads_label = (UILabel *) ui_get_element_by_tag(&screen_top, "downloadcount");
    likes_label     = (UILabel *) ui_get_element_by_tag(&screen_top, "likecount");
    stars_label     = (UILabel *) ui_get_element_by_tag(&screen_top, "stars");
    difficulty_face = (UIImage *) ui_get_element_by_tag(&screen_top, "difficultyface");
    level_id_label  = (UILabel *) ui_get_element_by_tag(&screen_top, "levelid");
    song_label      = (UILabel *) ui_get_element_by_tag(&screen, "songid");
    
    like_image = (UIImage *) ui_get_element_by_tag(&screen_top, "likeimage");

    normal_progress = (UIProgressBar *) ui_get_element_by_tag(&screen, "normalprogress");
    normal_progress_val = (UILabel *) ui_get_element_by_tag(&screen, "normalprogressvalue");
    practice_progress = (UIProgressBar *) ui_get_element_by_tag(&screen, "practiceprogress");
    practice_progress_val = (UILabel *) ui_get_element_by_tag(&screen, "practiceprogressvalue");

    ui_progress_bar_set_tint(normal_progress, C2D_Color32(0, 255, 0, 255));
    ui_progress_bar_set_tint(practice_progress, C2D_Color32(0, 255, 255, 255));

    size_t out_size;
    char *gmd = read_file(state.custom_level_path, &out_size);
    if (!gmd) return;

    set_name_creator(gmd);
    set_description(gmd);
    set_downloads_likes(gmd);
    set_stars(gmd);
    set_difficulty();
    set_level_id(gmd);
    set_song_id(gmd);
    
    free(gmd);

    yes_exit = false;
    in_infobox = false;
}

int external_popup_loop() {
    if (yes_exit) {
        free_level_progress();
        
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
        return true;
    }

    set_progress();

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_infobox) ui_screen_update(&screen, &touch);
    ui_screen_update(&screen_top, &touch);
    
    if (in_infobox) {
        int returned = external_level_infobox_loop();
        if (returned) {
            in_infobox = false;
        }
    } 
    return false;
}

void external_popup_draw_top() {
    ui_screen_draw(&screen_top);
}

void external_popup_draw_bot() {
    ui_screen_draw(&screen);
    if (in_infobox) external_level_infobox_draw();
}