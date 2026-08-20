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
#include "songs.h"



static bool exit_flag = false;
static bool in_info_box = false;
static bool in_comments = false;
static bool in_refresh = false;
static bool in_delete = false;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UILabel *level_name;
static UILabel *level_creator;
static UILabel *downloads;
static UILabel *likes;
static UILabel *length;
static UILabel *stars;
static UILabel *description;
static UILabel *level_id;
static UIImage *difficulty_face;
static UIImage *featured_glow;

static UILabel *normal_percent;
static UILabel *practice_percent;
static UILabel *song_name;
static UILabel *song_artist;
static UILabel *song_id;
static UILabel *song_size;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_open_info(UIElement *e) {
    in_info_box = true;
    online_level_infobox_init();
}

static void action_open_comments(UIElement *e) {
    in_comments = true;
    online_comments_init();
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

    char *tmp_downloads = truncate_number(entry_srch->downloads);
    ui_label_set_text(downloads, tmp_downloads);

    char *tmp_likes = truncate_number(entry_srch->likes);
    ui_label_set_text(likes, tmp_likes);

    char *tmp_length = "Unkn.";
    if (IN_BOUNDS(entry_srch->lengthNum, level_lengths)) {
        tmp_length = (char *) level_lengths[entry_srch->lengthNum];
    }
    ui_label_set_text(length, tmp_length);

    char tmp_lvlid[32] = "";
    snprintf(tmp_lvlid, 32, "<#78aaf0>ID: %d", entry_srch->levelId);

    char tmp_creator[26] = "";
    snprintf(tmp_creator, 26, "By: %s", entry_c->creatorName);
    ui_label_set_text(level_creator, tmp_creator);

    // get song and song artist
    char *tmp_song = "Unknown";
    char *tmp_songartist = "Unknown";
    char *tmp_songartist2 = "";

    if (entry_srch->songId != 0) {
        char tmp_songsize[24] = "";
        snprintf(tmp_songsize, 24, "Size: %dMB", entry_sng->songSize);
        ui_label_set_text(song_size, tmp_songsize);

        if (entry_srch->songIndex < songEntriesLength) {
                tmp_song = entry_sng->songTitle;
                tmp_songartist = entry_sng->artistName;
            }

    } else {
        if (IN_BOUNDS(entry_srch->mainSongId, main_songs)) {
            tmp_song = (char *) main_songs[entry_srch->mainSongId].title;
            tmp_songartist = (char *) main_songs[entry_srch->mainSongId].artist;
        }
        if (IN_BOUNDS(entry_srch->mainSongId, main_songs)) {
            
        }
        
        ui_disable_element((UIElement *) song_size);
        ui_run_func_on_tag(&default_screen, "downloadbtn", ui_disable_element);
    }

    char tmp_starcount[4];
    snprintf(tmp_starcount, 4, "%d", entry_srch->stars);
    ui_label_set_text(stars, tmp_starcount);
    if (entry_srch->stars == 0) ui_run_func_on_tag(&default_screen_top, "star", ui_disable_element);

    ui_label_set_text(level_name, entry_srch->name);
    
    int difficulty_id = 0;

    if (IN_BOUNDS(entry_srch->stars, difficulty_stars)) {
            difficulty_id = difficulty_stars[entry_srch->stars];
        }

    ui_image_set_image(difficulty_face, difficulty_id, 0);

    if (entry_srch->featureScore == 0) ui_disable_element((UIElement *)featured_glow);
    char *tmp_desc = "";
    tmp_desc = strdup(wrap_text(&chatFont_fontCharset, description->base.scaleX, entry_srch->description, 270));
    
    ui_label_set_text(description, tmp_desc);
    ui_label_set_text(level_id, tmp_lvlid);

    snprintf(tmp_songartist2, 132, "By: %s", tmp_songartist);

    ui_label_set_text(song_name, tmp_song);

    char tmp_songid[16] = "";
    snprintf(tmp_songid, 16, "SongID: %d", (entry_srch->songId == 0) ? entry_srch->mainSongId : entry_sng->ngSongId);

    ui_label_set_text(song_id, tmp_songid);
    ui_label_set_text(song_artist, tmp_songartist2);
};

void online_level_menu_loop() {
    exit_flag = false;
    in_comments = false;
    in_delete = false;
    in_refresh = false;
    in_info_box = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu.txt");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/online_level_menu_top.txt");

    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    // top screen elements

    level_name = (UILabel *) ui_get_element_by_tag(&default_screen_top, "levelname");
    level_creator = (UILabel *) ui_get_element_by_tag(&default_screen_top, "creatorname");

    downloads = (UILabel *) ui_get_element_by_tag(&default_screen_top, "downloadcount");
    likes = (UILabel *) ui_get_element_by_tag(&default_screen_top, "likecount");
    length = (UILabel *) ui_get_element_by_tag(&default_screen_top, "length");
    stars = (UILabel *) ui_get_element_by_tag(&default_screen_top, "starcount");

    difficulty_face = (UIImage *) ui_get_element_by_tag(&default_screen_top, "difficultyface");
    featured_glow = (UIImage *) ui_get_element_by_tag(&default_screen_top, "glow");

    description = (UILabel *) ui_get_element_by_tag(&default_screen_top, "description");
    level_id = (UILabel *) ui_get_element_by_tag(&default_screen_top, "levelid");

    // bottom screen elements

    normal_percent = (UILabel *) ui_get_element_by_tag(&default_screen, "normalprogressvalue");
    practice_percent = (UILabel *) ui_get_element_by_tag(&default_screen, "practiceprogressvalue");

    song_name = (UILabel *) ui_get_element_by_tag(&default_screen, "songname");
    song_artist = (UILabel *) ui_get_element_by_tag(&default_screen, "songartist");
    song_id = (UILabel *) ui_get_element_by_tag(&default_screen, "songid");
    song_size = (UILabel *) ui_get_element_by_tag(&default_screen, "songsize");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    // uncomment this to bypass getting levelobject 
    // int result = 0;

    int result = get_level_data(search_entries[curr_search_id].levelId);
    populate_level_info();

    if (result == 0){
        //idk do something i don't have a use for the data in here within this menu
    }
    else
    {
        char tmp[16];
        snprintf(tmp, 16, "%d", result);
        ui_label_set_text(level_name, tmp);
    };

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

        if (!in_info_box && !in_comments && !in_delete && !in_refresh) ui_screen_update(&default_screen, &touch);

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
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
