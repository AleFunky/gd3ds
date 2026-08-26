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
#include "menus/components/ui_progress_bar.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"
#include "state.h"
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
#include "online_level_warningbox.h"
#include "online_level_comments.h"
#include "settings.h"
#include "utils/json_config.h"

#define EASY_DEMON_FACE_1 259
#define MEDIUM_DEMON_FACE_1 261
#define HARD_DEMON_FACE_1 257
#define INSANE_DEMON_FACE_1 263
#define EXTREME_DEMON_FACE_1 265

const int demon_faces_1[] = {
    NA_FACE,
    EASY_DEMON_FACE_1,
    MEDIUM_DEMON_FACE_1,
    HARD_DEMON_FACE_1,
    INSANE_DEMON_FACE_1,
    EXTREME_DEMON_FACE_1
};

const int demon_face_featured_offsets[] = {
    0,
    -9,
    -8,
    -8,
    -8,
    -8
};

static NetworkTask level_task = {
    .func = get_level
};

static NetworkTask song_data_task = {
    .func = get_song_data
};

static DownloadTask song_task = {
    .path = USER_SONGS_DIR
};

static bool exit_flag = false;
static bool in_comments = false;
static bool in_delete = false;
static bool in_info_box = false;
static bool in_errorbox = false;
static bool in_warningbox = false;
static bool play_flag = false;
static bool comes_from_levels = false;

int result = -2;

bool pressed_play = false;
bool passed_highobj_warning = false;
bool passed_version_warning = false;
bool passed_song_warning = false;
bool refresh = false;

char download_speed[24]= "Speed: 0KB/s";

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static UILabel *level_name_label;
static UILabel *level_creator_label;
static UIImage *high_obj_icon_image;
static UIImage *collab_icon_image;
static UILabel *downloads_label;
static UIImage *likes_image;
static UILabel *likes_label;
static UILabel *length_label;
static UILabel *stars_label;
static UILabel *description_label;
static UILabel *level_id_label;
static UIImage *difficulty_face_image;
static UIImage *featured_glow_image;

static UISpinner *spinner;
static UIButton *play_button;
static UILabel *normal_percent_label;
static UILabel *practice_percent_label;
static UILabel *song_name_label;
static UILabel *song_artist_label;
static UILabel *song_status_label;
static UIProgressBar *song_progress_bar;
static UILabel *song_id_label;
static UILabel *speed_label;
static UILabel *song_size_label;
static UIButton *song_download_button;

static void update_download_button(){
    bool song_exists = check_song(search_entries[curr_search_id].songId);
    if (song_exists) {
        ui_disable_element((UIElement *)song_download_button);
    }
}

static void action_download(){
    if (!song_task.running && !song_data_task.running) {
        ui_button_set_image(song_download_button, 22, 0);
        ui_disable_element((UIElement *) song_status_label);
        ui_disable_element((UIElement *) song_id_label);
        ui_enable_element((UIElement *) song_progress_bar);
        ui_enable_element((UIElement *) speed_label);
        snprintf(download_speed, sizeof(download_speed), "Speed: 0KB/s");
        ui_label_set_text(speed_label, download_speed);
        create_network_thread(&song_data_task);
    } else {
        song_data_task.cancelled = true;
        song_task.cancelled = true;
    }
}

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
    comes_from_levels = false;
}

static int warning_result = 0;

static void action_play(UIElement *e) {
    if (result == 0 || comes_from_levels) {
        pressed_play = true;
    }
}

static void action_open_info(UIElement *e) {
    if (result == 0 || comes_from_levels){
        in_info_box = true;
        online_level_infobox_init();
    }
}

static void action_open_comments(UIElement *e) {
    if (result == 0 || comes_from_levels){
        in_comments = true;
        online_comments_init();
    }
}

static void action_delete_level(UIElement *e) {
    in_delete = true;
    delete_level_init();
}

void delete_level(){
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

void populate_level_info() {
    SearchEntry *entry_srch = &search_entries[curr_search_id];
    CreatorEntry *entry_c = &creator_entries[entry_srch->creatorIndex];
    SongEntry *entry_sng = &song_entries[entry_srch->songIndex];

    char *downloads = truncate_number(entry_srch->downloads);
    ui_label_set_text(downloads_label, downloads);

    char *likes = truncate_number(entry_srch->likes);
    ui_label_set_text(likes_label, likes);

    if (entry_srch->likes < 0) {
        ui_image_set_image(likes_image, DISLIKE_ICON, 0);
    }

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
    char creator[36] = "By -";
    snprintf(creator, sizeof(creator), "<#%s>By %s</>", (entry_c->userId == 0) ? "5AFFFF" : "FFFFFF", entry_c->creatorName );
    ui_label_set_text(level_creator_label, creator);

    // Song and song artist
    char *song_name = "Unknown";
    char *song_artist_name = "By Unknown";

    if (entry_srch->songId != 0) {
        // Custom song
        char tmp_songsize[24];
        snprintf(tmp_songsize, sizeof(tmp_songsize), "Size: %dMB", entry_sng->songSize);
        ui_label_set_text(song_size_label, tmp_songsize);

        if (entry_srch->songIndex < songEntriesLength) {
            song_name = entry_sng->songTitle;
            song_artist_name = entry_sng->artistName;
        }
        update_download_button();
    } else {
        // Main level song
        if (IN_BOUNDS(entry_srch->mainSongId, main_songs)) {
            song_name = (char *) main_songs[entry_srch->mainSongId].title;
            song_artist_name = (char *) main_songs[entry_srch->mainSongId].artist;
        }
        
        ui_disable_element((UIElement *) song_size_label);
        ui_disable_element((UIElement *)song_download_button);
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
    int difficulty_id = NA_FACE;

    int featured_demon_offset = 0;

    if(entry_srch->isAuto) {
        difficulty_id = AUTO_FACE;
    } else if(entry_srch->isDemon && IN_BOUNDS(entry_srch->difficulty, demon_faces_1)) {
        difficulty_face_image->base.y = 87 - 5;
        featured_demon_offset = demon_face_featured_offsets[entry_srch->difficulty];
        difficulty_id = demon_faces_1[entry_srch->difficulty];
    } else if (!entry_srch->isDemon && IN_BOUNDS(entry_srch->difficulty, difficulty_faces)) {
        difficulty_id = difficulty_faces[entry_srch->difficulty];
    }

    ui_image_set_image(difficulty_face_image, difficulty_id, 0);

    if (entry_srch->featureScore > 0) {
        int featured_id = 0;
        int yOffset = 0;

        if(entry_srch->epic > 0 && IN_BOUNDS(entry_srch->epic, epics)){
            featured_id = epics[entry_srch->epic];
            yOffset = -2;
        } else if(entry_srch->featureScore > 0) {
            featured_id = FEATURED_GLOW;
        }

        if(entry_srch->isDemon) yOffset += featured_demon_offset;

        featured_glow_image->base.y = 81.65 + yOffset;

        ui_image_set_image(featured_glow_image, featured_id, 0);
    } else{
        ui_disable_element((UIElement *)featured_glow_image);
    }

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
    float half_creator_length = get_text_length(&goldFont_fontCharset, 0.7f, false, entry_c->creatorName) / 2;

    // Original icon
    bool is_copy = entry_srch->originalId != 0;
    if (is_copy) {
        ui_element_set_position((UIElement *)collab_icon_image, 200 + half_creator_length + 24, collab_icon_image->base.y);
    } else {
        ui_disable_element((UIElement *)collab_icon_image);
    }

    // High object count icon
    bool high_obj_count = entry_srch->objCount >= (is_N3DS ? 44000 : 14000);
    if (high_obj_count) {
        ui_element_set_position((UIElement *)high_obj_icon_image, 200 + half_creator_length + 24 + (is_copy ? 13 : 0), high_obj_icon_image->base.y);
    } else {
        ui_disable_element((UIElement *)high_obj_icon_image);
    }
}

static void handle_errors(int code) {
    ui_disable_element((UIElement *) spinner);
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
            snprintf(error_message, sizeof(error_message), "No <#41e24e>Internet</> connection!");
            break;

        default:
            snprintf(error_message, sizeof(error_message), "An unknown error has occurred.<p>Error code: %d", result);
            break;
    }
    in_errorbox = true;
    online_errorbox_init(error_message);
}

static void handle_song_codes(int code) {
    // i have no idea how the actual codes work and why they give seemingly wrong results this is my best guess
    ui_disable_element((UIElement *) song_progress_bar);
    ui_disable_element((UIElement *) speed_label);
    ui_enable_element((UIElement *) song_id_label);
    ui_enable_element((UIElement *) song_status_label);
    ui_button_set_image(song_download_button, 57, 0);
    char message[64];
    switch (code) {
        case -3:
        case -2: 
            snprintf(message, sizeof(message), "<#f93219>Unknown error.</>");
            break;
        case 0:
            snprintf(message, sizeof(message), "<#00FF00>Download complete.</>");
            ui_disable_element((UIElement *) song_download_button);
        case 1:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", result);
            break;
    }
    ui_label_set_text(song_status_label, message);
}

static void handle_song_data_errors(int code) {
    ui_disable_element((UIElement *) song_progress_bar);
    ui_disable_element((UIElement *) speed_label);
    ui_enable_element((UIElement *) song_status_label);
    ui_enable_element((UIElement *) song_id_label);
    ui_button_set_image(song_download_button, 57, 0);
    char message[64] = "";
    switch (code) {
        case -3: 
            snprintf(message, sizeof(message), "<#f93219>Unknown data failure.</>");
            break;
        case -2:
            snprintf(message, sizeof(message), "<#f93219>Song not allowed for use.</>");
            break;
        case -1: 
            snprintf(message, sizeof(message), "<#f93219>Failed to fetch info.</>");
            break;
        case 1:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", result);
            break;
    }
    ui_label_set_text(song_status_label, message);
}

static void action_refresh_level(UIElement *e) {
    result = -2;
    refresh = true;
    ui_enable_element((UIElement *)spinner);
    ui_disable_element((UIElement *)play_button);
    create_network_thread(&level_task);
}

static void play_level() {
    play_flag = true;
    play_sfx(&play_sound, 1);

    state.custom_level = true;
    state.online_level = true;

    set_fade_status(FADE_STATUS_OUT);

    comes_from_levels = true;
    playing_menu_loop = false;
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"info", action_open_info },
    {"comments", action_open_comments },
    {"reload", action_refresh_level },
    {"deletelevel", action_delete_level },
    {"play", action_play },
    {"download", action_download },
};

void online_level_menu_loop() {
    exit_flag = false;
    in_comments = false;
    in_delete = false;
    in_warningbox = false;
    in_info_box = false;
    comments_need_refresh = true;
    play_flag = false;
    pressed_play = false;
    passed_highobj_warning = false;
    passed_version_warning = false;
    passed_song_warning = false;
    warning_result = 0;
    result = -2;
    refresh = false;

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

    likes_image = (UIImage *) ui_get_element_by_tag(&default_screen_top, "thumbsup");

    description_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "description");
    level_id_label = (UILabel *) ui_get_element_by_tag(&default_screen_top, "levelid");

    // Bottom screen elements
    normal_percent_label = (UILabel *) ui_get_element_by_tag(&default_screen, "normalprogressvalue");
    practice_percent_label = (UILabel *) ui_get_element_by_tag(&default_screen, "practiceprogressvalue");

    song_name_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songname");
    song_artist_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songartist");
    song_id_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songid");
    speed_label = (UILabel *) ui_get_element_by_tag(&default_screen, "speed");
    song_size_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songsize");
    song_download_button = (UIButton *) ui_get_element_by_tag(&default_screen, "downloadbtn");
    song_status_label = (UILabel *) ui_get_element_by_tag(&default_screen, "songstatus");
    song_progress_bar = (UIProgressBar *) ui_get_element_by_tag(&default_screen, "progressbar");

    spinner = (UISpinner *) ui_get_element_by_tag(&default_screen, "spinner");
    play_button = (UIButton *) ui_get_element_by_tag(&default_screen, "playbutton");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    ui_progress_bar_set_tint(song_progress_bar, C2D_Color32(50, 190, 240, 255));
    ui_disable_element((UIElement *) song_progress_bar);
    ui_disable_element((UIElement *) song_status_label);
    ui_disable_element((UIElement *) speed_label);

    if (!comes_from_levels) {
        ui_disable_element((UIElement *)play_button);
    } else {
        ui_disable_element((UIElement *)spinner);
        ui_enable_element((UIElement *)play_button);
    }

    
    
    populate_level_info();

    if (!comes_from_levels) {
        create_network_thread(&level_task);
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

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

        if (song_data_task.finished) {
            int song_data_result = -3;
            song_data_result = song_data_task.result;
            // Handle result
            if (song_data_result == 0) {
                char songId[10];
                snprintf(songId, sizeof(songId), "%d", search_entries[curr_search_id].songId);
                song_data_task.finished = false;
                song_task.url = song_entries[search_entries[curr_search_id].songIndex].songLink;
                song_task.song_id = songId;

                create_download_song_thread(&song_task);
            } else { handle_song_data_errors(song_data_result); }
            
        }

        if (song_task.running) {
            song_progress_bar->value = song_task.progress;
            snprintf(download_speed, sizeof(download_speed), "Speed: %dKB/s", song_task.speed);
            ui_label_set_text(speed_label, download_speed);
        }

        // Run when finished
        if (song_task.finished) {
            int song_result = -3;
            song_result = song_task.result;
            // Handle result

            handle_song_codes(song_result);
            song_task.finished = false;
        }

        // Run when finished
        if (level_task.finished) {
            result = level_task.result;
            // Handle result
            if (result != 0 && !comes_from_levels) {
                handle_errors(result);
            } else { // No errors
                ui_disable_element((UIElement *) spinner);
                ui_enable_element((UIElement *) play_button);
                if (refresh == true) {
                    populate_level_info();
                    refresh = false;
                }
            }
            level_task.finished = false;
        }

        if (pressed_play) {
            if (settingsState.skipHighObjWarning || (search_entries[curr_search_id].objCount < (is_N3DS ? 44000 : 14000))) passed_highobj_warning = true;
            if (settingsState.skipVersionWarning || (search_entries[curr_search_id].gameVersion < 22)) passed_version_warning = true;
            if (settingsState.skipSongWarning || (check_song(search_entries[curr_search_id].songId))) passed_song_warning = true;

            if (warning_result == 1) {
                pressed_play = false;
                passed_highobj_warning = false;
                passed_version_warning = false;
                passed_song_warning = false;
                warning_result = 0;
            }

            if (warning_result == 2 && passed_highobj_warning && passed_version_warning) passed_song_warning = true;

            if (warning_result == 2 && passed_highobj_warning) passed_version_warning = true;

            if (warning_result == 2) passed_highobj_warning = true;

            if (!passed_highobj_warning && !in_warningbox && pressed_play) {
                warning_result = 0;
                online_level_warningbox_init("High objects", "This level has a <#ffa54b>high object</> count<p>and might not be <#ff5a5a>fully playable</>.");
                in_warningbox = true;
            }

            if (passed_highobj_warning && !passed_version_warning && !in_warningbox && pressed_play) {
                warning_result = 0;
                online_level_warningbox_init("Version warning", "This level was made or updated in an<p><#ffa54b>incompatible game version</>. It might<p>not be <#ff5a5a>fully playable</>.");
                in_warningbox = true;
            }

            if (passed_highobj_warning && passed_version_warning && !passed_song_warning && !in_warningbox && pressed_play) {
                warning_result = 0;
                online_level_warningbox_init("Missing song", "This level uses a <#4c8cc7>custom song</> that<p>has not been <#36c244>downloaded</> yet. Play<p>without music?");
                in_warningbox = true;
            }

            if (passed_highobj_warning && passed_version_warning && passed_song_warning) {
                pressed_play = false;
                passed_highobj_warning = false;
                passed_version_warning = false;
                passed_song_warning = false;
                play_level();
            }
        }

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
            if(in_warningbox) online_level_warningbox_draw_bot();
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
            if (level_entry) {
                if (level_entry->levelString) free(level_entry->levelString);
                free(level_entry);
                level_entry = NULL;
            }

            if (comment_entries) {
                free(comment_entries);
                comment_entries = NULL;
            }
            game_state = STATE_ONLINE;
            break;
        }

        if (play_flag) {
            stop_mp3();
            game_state = STATE_GAME;
            break;
        }

        if (!in_info_box && !in_comments && !in_delete && !in_warningbox && !in_errorbox) ui_screen_update(&default_screen, &touch);

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
        if (in_warningbox)
        {
            int returned = online_level_warningbox_loop();
            if (returned != 0)
            {
                in_warningbox = false;
                warning_result = returned;
                char temp = warning_result + '0';
                output_log(&temp);
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

    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
