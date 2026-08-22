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
#include "menus/external_level_infobox.h"
#include "menus/online_level_comments.h"
#include "menus/online_level_infobox.h"
#include "menus/refresh_online_level.h"
#include "menus/delete_online_level.h"
#include "utils/server_utils.h"
#include "utils/string_helpers.h"
#include "menus/online_menu.h"
#include "menus/external_popup.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "songs.h"
#include "online_level_errorbox.h"
#include "online_level_comments.h"

static bool exit_flag = false;
static bool in_info_box = false;
static bool in_comments = false;
static bool in_refresh = false;
static bool in_delete = false;
static bool in_errorbox = false;

int result = -2;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UILabel *level_name_label;
static UILabel *level_creator_label;
static UIImage *high_obj_icon_image;
static UIImage *collab_icon_image;
static UILabel *downloads_label;
static UILabel *likes_label;
static UILabel *length_label;
static UILabel *stars_label;
static UILabel *description_label;
static UILabel *level_id_label;
static UIImage *difficulty_face_image;
static UIImage *featured_glow_image;

static UILabel *normal_percent_label;
static UILabel *practice_percent_label;
static UILabel *song_name_label;
static UILabel *song_artist_label;
static UILabel *song_id_label;
static UILabel *song_size_label;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_info(UIElement *e) {
    if (result == 0){
        in_info_box = true;
        online_level_infobox_init();
    }
}

static void action_open_comments(UIElement *e) {
    if (result == 0){
        in_comments = true;
        online_comments_init();
    }
}

static void action_refresh_level(UIElement *e) {
    in_refresh = true;
    refresh_level_init();
}

static void action_delete_level(UIElement *e) {
    in_delete = true;
    delete_level_init();
}

void delete_level(){
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"info", action_open_info },
    {"comments", action_open_comments },
    {"reload", action_refresh_level },
    {"deletelevel", action_delete_level },
};

void populate_level_info() {
    SearchEntry *entry_srch = &search_entries[curr_search_id];
    CreatorEntry *entry_c = &creator_entries[entry_srch->creatorIndex];
    SongEntry *entry_sng = &song_entries[entry_srch->songIndex];

    char *downloads = truncate_number(entry_srch->downloads);
    ui_label_set_text(downloads_label, downloads);

    char *likes = truncate_number(entry_srch->likes);
    ui_label_set_text(likes_label, likes);

    // Length
    char *length = "Unkn.";
    if (IN_BOUNDS(entry_srch->lengthNum, level_lengths)) {
        length = (char *) level_lengths[entry_srch->lengthNum];
    }
    ui_label_set_text(length_label, length);

    // Level ID
    char lvlid[32];
    snprintf(lvlid, sizeof(lvlid), "<#78aaf0>ID: %d", entry_srch->levelId);
    ui_label_set_text(level_id_label, lvlid);

    // Creator
    char creator[26];
    snprintf(creator, sizeof(creator), "By: %s", entry_c->creatorName);
    ui_label_set_text(level_creator_label, creator);

    // Song and song artist
    char *song_name = "Unknown";
    char *song_artist_name = "Unknown";

    if (entry_srch->songId != 0) {
        // Custom song
        char tmp_songsize[24];
        snprintf(tmp_songsize, sizeof(tmp_songsize), "Size: %dMB", entry_sng->songSize);
        ui_label_set_text(song_size_label, tmp_songsize);

        if (entry_srch->songIndex < songEntriesLength) {
            song_name = entry_sng->songTitle;
            song_artist_name = entry_sng->artistName;
        }
    } else {
        // Main level song
        if (IN_BOUNDS(entry_srch->mainSongId, main_songs)) {
            song_name = (char *) main_songs[entry_srch->mainSongId].title;
            song_artist_name = (char *) main_songs[entry_srch->mainSongId].artist;
        }
        
        ui_disable_element((UIElement *) song_size_label);
        ui_run_func_on_tag(&default_screen, "downloadbtn", ui_disable_element);
    }

    // Song artist again
    char song_artist[132];
    snprintf(song_artist, sizeof(song_artist), "By: %s", song_artist_name);
    ui_label_set_text(song_artist_label, song_artist);
    ui_label_set_text(song_name_label, song_name);
    
    // Star count
    char star_count[4];
    snprintf(star_count, sizeof(star_count), "%d", entry_srch->stars);
    ui_label_set_text(stars_label, star_count);

    // Unrated
    if (entry_srch->stars == 0) {
        ui_run_func_on_tag(&default_screen_top, "star", ui_disable_element);
        ui_element_set_position((UIElement *) featured_glow_image, 165, 97.65);
        ui_element_set_position((UIElement *) difficulty_face_image, 165, 93);
    }

    ui_label_set_text(level_name_label, entry_srch->name);
    
    // Set difficulty
    int difficulty_id = difficulty_stars[0];

    if (IN_BOUNDS(entry_srch->stars, difficulty_stars)) {
        difficulty_id = difficulty_stars[entry_srch->stars];
    }

    ui_image_set_image(difficulty_face_image, difficulty_id, 0);

    // Set featured
    if (entry_srch->featureScore == 0) ui_disable_element((UIElement *)featured_glow_image);

    // Description
    char *wrapped_description = wrap_text(&chatFont_fontCharset, description_label->base.scaleX, entry_srch->description, 270);
    char *desc = strdup(wrapped_description);
    ui_label_set_text(description_label, desc);
    free(desc);

    // Song id
    char song_id[16];
    snprintf(song_id, sizeof(song_id), "SongID: %d", (entry_srch->songId == 0) ? entry_srch->mainSongId : entry_sng->ngSongId);
    ui_label_set_text(song_id_label, song_id );

    // Level icons
    float half_creator_length = get_text_length(&goldFont_fontCharset, 0.7f, false, creator) / 2;

    // Original icon
    bool is_copy = entry_srch->originalId != 0;
    if (is_copy) {
        ui_element_set_position((UIElement *)collab_icon_image, 200 + half_creator_length + 8, collab_icon_image->base.y);
    } else {
        ui_disable_element((UIElement *)collab_icon_image);
    }

    // High object count icon
    bool high_obj_count = entry_srch->objCount >= 42000;
    if (high_obj_count) {
        ui_element_set_position((UIElement *)high_obj_icon_image, 200 + half_creator_length + 8 + (is_copy ? 13 : 0), high_obj_icon_image->base.y);
    } else {
        ui_disable_element((UIElement *)high_obj_icon_image);
    }
}

static void handle_errors(int code) {
    char error_message[64];
    switch (code) {
        case -2: 
            snprintf(error_message, sizeof(error_message), "%s", "An unknown error has occured.");
            break;
        case -1:
            snprintf(error_message, sizeof(error_message), "%s", "Failed to download level!<p>Please try again later.");
            break;
        case 6:
        case 7:
            snprintf(error_message, sizeof(error_message), "No Internet connection!");
            break;

        default:
            snprintf(error_message, sizeof(error_message), "An unknown error has occurred.<p>Error code: %d", result);
            break;
    }
    in_errorbox = true;
    online_errorbox_init(error_message);
}

void online_level_menu_loop() {
    exit_flag = false;
    in_comments = false;
    in_delete = false;
    in_refresh = false;
    in_info_box = false;
    comments_need_refresh = true;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu.txt");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu_top.txt");

    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    // Top screen elements

    level_name_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "levelname");
    level_creator_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "creatorname");

    collab_icon_image = (UIImage *) ui_get_element_by_tag(&default_screen_top, "collab");
    high_obj_icon_image = (UIImage *) ui_get_element_by_tag(&default_screen_top, "highobj");

    downloads_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "downloadcount");
    likes_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "likecount");
    length_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "length");
    stars_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "starcount");

    difficulty_face_image = (UIImage *) ui_get_element_by_tag(&default_screen_top, "difficultyface");
    featured_glow_image = (UIImage *) ui_get_element_by_tag(&default_screen_top, "glow");

    description_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "description");
    level_id_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "levelid");

    // Bottom screen elements
    normal_percent_label = (UILabel *) ui_get_element_by_tag(&default_screen, "normalprogressvalue");
    practice_percent_label = (UILabel *) ui_get_element_by_tag(&default_screen, "practiceprogressvalue");

    song_name_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songname");
    song_artist_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songartist");
    song_id_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songid");
    song_size_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songsize");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    populate_level_info();

    result = get_level_data(search_entries[curr_search_id].levelId);

    // Handle result
    if (result != 0) {
        handle_errors(result);
    } else {
        // TODO: handle level data
    }

    set_fade_status(FADE_STATUS_IN);
    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;
        
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
            if(in_info_box) online_level_infobox_draw_bot();
            if(in_comments) online_comments_draw();
            if(in_delete) delete_level_draw_bot();
            if(in_refresh) refresh_level_draw_bot();
            if(in_errorbox) online_errorbox_draw_bot();

            change_blending(true);
            draw_touch_effect();
            change_blending(false);
            
            // Top screen, drawn once per eye when 3D is on
            for (int eye = 0; begin_top_eye(eye); eye++) {
                draw_fade();

                begin_eye_layer(DEPTH_UI);
                ui_screen_draw(&default_screen_top);
                end_eye_layer();

                begin_eye_layer(DEPTH_POPUP);
                if(in_info_box) online_level_infobox_draw_top();
                end_eye_layer();
            }
            C2D_ViewReset();
            C3D_FrameEnd(0);


        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_ONLINE;
            break;
        }

        if (!in_info_box && !in_comments && !in_delete && !in_refresh && !in_errorbox) ui_screen_update(&default_screen, &touch);

        if (in_info_box)
        {
            int returned = online_level_infobox_loop();
            if (returned)
            {
                in_info_box = false;
            }
        }

        if (in_comments)
        {
            int returned = online_comments_loop();
            if (returned)
            {
                in_comments = false;
            }
        }
        if (in_delete)
        {
            int returned = delete_level_loop();
            if (returned)
            {
                in_delete = false;
            }
        }
        if (in_refresh)
        {
            int returned = refresh_level_loop();
            if (returned)
            {
                in_refresh = false;
            }
        }
        if (in_errorbox)
        {
            int returned = online_errorbox_loop();
            if (returned)
            {
                in_errorbox = false;
            }
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    

    if (level_entry) {
        if (level_entry->levelString) free(level_entry->levelString);
        free(level_entry);
    }

    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
