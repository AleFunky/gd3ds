#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "3ds/thread.h"
#include "3ds/types.h"

#include "main.h"
#include "menus/core/ui_element.h"
#include "mp3_player.h"
#include "graphics.h"
#include "save/saving.h"
#include "state.h"
#include "utils/folders.h"
#include "utils/server_utils.h"
#include "utils/string_helpers.h"
#include "utils/json_config.h"

#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_window_button.h"

#include "menus/creator_menu/search_menu.h"
#include "menus/creator_menu/external/external_level_infobox.h"
#include "menus/creator_menu/online/online_level_comments.h"
#include "menus/creator_menu/online/online_level_infobox.h"
#include "menus/creator_menu/online/two_option_pop_up.h"
#include "menus/creator_menu/online/online_menu.h"
#include "menus/creator_menu/external/external_popup.h"
#include "menus/creator_menu/online/online_level_comments.h"
#include "menus/settings_hub/songs.h"
#include "menus/settings_hub/settings.h"
#include "menus/settings_hub/info_card.h"

#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "utils/utils.h"

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

static GenericTask level_task = {
    .func = get_level
};

static Thread level_thread;

static GenericTask song_data_task = {
    .func = get_song_data
};

static Thread song_data_thread;

static DownloadTask song_task = {
    .path = USER_SONGS_DIR
};

static Thread song_thread;

static bool already_played_online_level = false;

int result = -2;

bool refresh = false;
bool song_exists = false;

int warning_step = 0;

char download_speed[24] = "Speed: 0 B/s";

static UIScreen *screen_top;
static UIScreen *screen;

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

static UIProgressBar *normal_percent_prog;
static UIProgressBar *practice_percent_prog;

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

bool in_warning = false;
bool in_warning_screen = false;

typedef enum WarningPopupStep {
    WARNING_HIGH_OBJECT,
    WARNING_VERSION,
    WARNING_MISSING_SONG,
    WARNING_DONE
} WarningPopupStep;

const char* warning_titles[] = {
    "High objects",
    "Version warning",
    "Missing song"
};

const char* warnings[] = {
    "This level has a <#ffa54b>high object</> count\nand might not be <#ff5a5a>fully playable</>.",
    "This level was made or updated in an\n<#ffa54b>incompatible game version</>. It might\nnot be <#ff5a5a>fully playable</>.",
    "This level uses a <#4c8cc7>custom song</> that\nhas not been <#36c244>downloaded</> yet. Play\nwithout music?"
};

static void update_download_button(){
    bool song_exists = check_song(search_entries[curr_search_id].songId);
    if (song_exists) {
        ui_disable_element((UIElement *)song_download_button);
    }
}

static void action_download(){
    if (!song_task.running && !song_data_task.running) {
        song_progress_bar->value = 0;
        ui_button_set_image(song_download_button, 22, 0);
        ui_disable_element((UIElement *) song_status_label);
        ui_disable_element((UIElement *) song_id_label);
        ui_enable_element((UIElement *) song_progress_bar);
        ui_enable_element((UIElement *) speed_label);
        snprintf(download_speed, sizeof(download_speed), "Speed: 0 B/s");
        ui_label_set_text(speed_label, download_speed);
        song_data_thread = create_generic_thread(&song_data_task);
    } else {
        if (song_data_task.running) {
            song_data_task.cancelled = true;
            threadJoin(song_data_thread, U64_MAX);
        }

        if (song_task.running) {
            song_task.cancelled = true;
            threadJoin(song_thread, U64_MAX);
        }
    }
}

static void open_warning(){
    char *warning = "Ultra unknown error.";
    if (IN_BOUNDS(warning_step, warnings)) {
        warning = (char *) warnings[warning_step]; 
    } else return;

    char *warning_title = "What?";
    if (IN_BOUNDS(warning_step, warning_titles)) {
        warning_title = (char *) warning_titles[warning_step]; 
    } else return;

    TwoOptionPopupData *warning_data = malloc(sizeof(TwoOptionPopupData));
    if(!warning_data) return;

    warning_data->text = strdup(warning);
    warning_data->title = strdup(warning_title);
    warning_data->proceed_text = strdup("Play");
    warning_data->cancel_text = strdup("Cancel");

    ui_stack_push(&warning_pop_up_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    ui_stack_push_data(warning_data);
}

static void play_level() {
    play_sfx(&play_sound, 1);

    state.custom_level = true;
    state.online_level = true;

    already_played_online_level = true;

    stop_mp3();
    playing_menu_loop = false;

    ui_stack_push_game_state(STATE_GAME);
}

void check_warnings_and_play(){
    if(result != 0) return;

    if(!already_played_online_level){
        bool should_warn;

        while(warning_step <= WARNING_DONE){
            switch(warning_step){
                case WARNING_HIGH_OBJECT:
                    int obj_count = (is_N3DS ? 44000 : 14000);
                    should_warn = search_entries[curr_search_id].objCount >= obj_count;
                    break;
                case WARNING_VERSION:
                    should_warn = derive_gj_version(search_entries[curr_search_id].gameVersion) > GD_VERSION;
                    break;
                case WARNING_MISSING_SONG:
                    int song_id = search_entries[curr_search_id].songId;
                    should_warn = !(song_id == 0 || check_song(song_id));
                default:
                    break;
            }

            if(should_warn) break;
            warning_step++;
        }

        if(should_warn){
            open_warning();
            warning_step++;
            return;
        }
    }

    play_level();
}

static void action_play(UIElement *e, const UIPropertyList *args) {
    check_warnings_and_play();
}

static void action_open_info(UIElement *e, const UIPropertyList *args) {
    if (result == 0 || already_played_online_level){
        ui_stack_push(&online_infobox_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    }
}

void delete_level(){
    // logic to delete level (post save revamp slop)
}

static void action_open_delete_level(){
    if (result == 0 || already_played_online_level){
        TwoOptionPopupData *delete_level_data = malloc(sizeof(TwoOptionPopupData));
        if(!delete_level_data) return;

        delete_level_data->text = strdup("Are you sure you want to\ndelete this level?");
        delete_level_data->title = strdup("Delete Level");
        delete_level_data->proceed_text = strdup("Yes");
        delete_level_data->cancel_text = strdup("No");

        ui_stack_push(&delete_pop_up_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
        ui_stack_push_data(delete_level_data);
    }
}

static void update_progress_bars() {
    LevelData *data = &current_level_entry->data;
    normal_percent_prog->value = data->normal_progress;
    practice_percent_prog->value = data->practice_progress;

    char normal[16];
    char practice[16];
    snprintf(normal, sizeof(normal), "%d%%", data->normal_progress);
    snprintf(practice, sizeof(practice), "%d%%", data->practice_progress);

    ui_label_set_text(normal_percent_label, normal);
    ui_label_set_text(practice_percent_label, practice);
}

static void populate_level_info() {
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
        snprintf(tmp_songsize, sizeof(tmp_songsize), "Size: %.1fMB", entry_sng->songSize);
        ui_label_set_text(song_size_label, tmp_songsize);

        if (entry_srch->songIndex < songEntriesLength) {
            song_name = entry_sng->songTitle;
            song_artist_name = entry_sng->artistName;
        }
        update_download_button();
    } else {
        // Main level song
        if (entry_srch->mainSongId >= 0 && entry_srch->mainSongId < current_main_level_pack->count) {
            song_name = (char *) current_main_level_pack->levels[entry_srch->mainSongId].song_data.title;
            song_artist_name = (char *) current_main_level_pack->levels[entry_srch->mainSongId].song_data.artist;
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
        ui_run_func_on_tag(screen_top, "star", ui_disable_element);
        ui_element_set_position((UIElement *) featured_glow_image, 165, 97.65);
        ui_element_set_position((UIElement *) difficulty_face_image, 165, 93);
    }

    ui_label_set_text(level_name_label, entry_srch->name);
    
    // Set difficulty
    int difficulty_id = NA_FACE;

    int featured_demon_offset = 0;

    if(entry_srch->isAuto) {
        difficulty_id = AUTO_FACE;
    } else if(entry_srch->isDemon && gdps) {
        difficulty_id = 258;
    } else if (entry_srch->isDemon && IN_BOUNDS(entry_srch->difficulty, demon_faces_1)) {
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
            featured_id = (gdps ? SUPER_GLOW : epics[entry_srch->epic]);
            yOffset = -2;
        } else if(entry_srch->featureScore > 0) {
            featured_id = FEATURED_GLOW;
        }

        if(!entry_srch->stars) yOffset += 4;

        if(entry_srch->isDemon) yOffset += featured_demon_offset;

        featured_glow_image->base.x = 165 + (gdps ? 0.3 : 0);
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
    snprintf(song_id, sizeof(song_id), "SongID: %d", (entry_srch->songId == 0) ? entry_srch->mainSongId + 1 : entry_sng->ngSongId);
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
    
    char key[16];
    snprintf(key, sizeof(key), "%d", entry_srch->levelId);
    
    current_level_entry = get_or_add_level_to_server_file(current_server_file, key, LEVEL_LIST_ONLINE);

    current_level_entry->data.level_id = entry_srch->levelId;

    update_progress_bars();
}

static void handle_errors(int code) {
    ui_disable_element((UIElement *) spinner);
    char error_message[64];
    switch (code) {
        case -2: 
            snprintf(error_message, sizeof(error_message), "%s", "An unknown error has occured.");
            break;
        case -1:
            snprintf(error_message, sizeof(error_message), "%s", "Failed to download level!\nPlease try again later.");
            break;
        case 6:
        case 7:
            snprintf(error_message, sizeof(error_message), "No <#41e24e>Internet</> connection!");
            break;
        case 28:
            snprintf(error_message, sizeof(error_message), "Connection timed out!");
            break;
        case 42:
            break;
        default:
            snprintf(error_message, sizeof(error_message), "An unknown error has occurred.\nError code: %d", result);
            break;
    }
    InfoCardData *data = malloc(sizeof(InfoCardData));
    if(!data) return;

    data->text = strdup(error_message);
    data->title = strdup("Error");
    data->customTitle = true;
    data->copied = true;

    ui_stack_push(&info_card_def, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    ui_stack_push_data(data);
}

static void handle_song_codes(int code) {
    ui_disable_element((UIElement *) song_progress_bar);
    ui_disable_element((UIElement *) speed_label);
    ui_enable_element((UIElement *) song_id_label);
    ui_enable_element((UIElement *) song_status_label);
    ui_button_set_image(song_download_button, 57, 0);
    char message[64];
    switch (code) {
        case -4:
            snprintf(message, sizeof(message), "<#f93219>Download failed.</>");
            break;
        case -3:
        case -2: 
            snprintf(message, sizeof(message), "<#f93219>Unknown error.</>");
            break;
        case 0:
            snprintf(message, sizeof(message), "<#00FF00>Download complete.</>");
            ui_disable_element((UIElement *) song_download_button);
            break;
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        case 42:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 28:
            snprintf(message, sizeof(message), "Connection timed out!");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", code);
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
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        case 42:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 28:
            snprintf(message, sizeof(message), "Connection timed out!");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", result);
            break;
    }
    ui_label_set_text(song_status_label, message);
}

static void action_refresh_level(UIElement *e, const UIPropertyList *props) {
    result = -2;
    refresh = true;
    ui_enable_element((UIElement *)spinner);
    ui_disable_element((UIElement *)play_button);
    level_thread = create_generic_thread(&level_task);
}

static UIActionDef online_level_actions[] = {
    {"info", action_open_info },
    {"delete", action_open_delete_level},
    {"reload", action_refresh_level },
    {"play", action_play },
    {"download", action_download },
};

static void online_level_init_top (UIScreen *s) {
    screen_top = s;
    // Top screen elements
    level_name_label = (UILabel *) ui_get_element_by_tag(screen_top, "levelname");
    level_creator_label = (UILabel *) ui_get_element_by_tag(screen_top, "creatorname");

    collab_icon_image = (UIImage *) ui_get_element_by_tag(screen_top, "collab");
    high_obj_icon_image = (UIImage *) ui_get_element_by_tag(screen_top, "highobj");

    downloads_label = (UILabel *) ui_get_element_by_tag(screen_top, "downloadcount");
    likes_label = (UILabel *) ui_get_element_by_tag(screen_top, "likecount");
    length_label = (UILabel *) ui_get_element_by_tag(screen_top, "length");
    stars_label = (UILabel *) ui_get_element_by_tag(screen_top, "starcount");

    difficulty_face_image = (UIImage *) ui_get_element_by_tag(screen_top, "difficultyface");
    featured_glow_image = (UIImage *) ui_get_element_by_tag(screen_top, "glow");

    likes_image = (UIImage *) ui_get_element_by_tag(screen_top, "thumbsup");

    description_label = (UILabel *) ui_get_element_by_tag(screen_top, "description");
    level_id_label = (UILabel *) ui_get_element_by_tag(screen_top, "levelid");
}

static void online_level_init (UIScreen *s) {
    comments_need_refresh = true;
    warning_step = 0;
    result = -2;
    refresh = false;

    screen = s;

    if(gdps) ui_disable_element(ui_get_element_by_tag(s, "garage"));

    // Bottom screen elements
    normal_percent_prog = (UIProgressBar *) ui_get_element_by_tag(screen, "normalprogress");
    practice_percent_prog = (UIProgressBar *) ui_get_element_by_tag(screen, "practiceprogress");
    normal_percent_label = (UILabel *) ui_get_element_by_tag(screen, "normalprogressvalue");
    practice_percent_label = (UILabel *) ui_get_element_by_tag(screen, "practiceprogressvalue");

    song_name_label = (UILabel *) ui_get_element_by_tag(screen, "songname");
    song_artist_label = (UILabel *) ui_get_element_by_tag(screen, "songartist");
    song_id_label = (UILabel *) ui_get_element_by_tag(screen, "songid");
    speed_label = (UILabel *) ui_get_element_by_tag(screen, "speed");
    song_size_label = (UILabel *) ui_get_element_by_tag(screen, "songsize");
    song_download_button = (UIButton *) ui_get_element_by_tag(screen, "downloadbtn");
    song_status_label = (UILabel *) ui_get_element_by_tag(screen, "songstatus");
    song_progress_bar = (UIProgressBar *) ui_get_element_by_tag(screen, "progressbar");

    spinner = (UISpinner *) ui_get_element_by_tag(screen, "spinner");
    play_button = (UIButton *) ui_get_element_by_tag(screen, "playbutton");

    ui_progress_bar_set_tint(normal_percent_prog, C2D_Color32(0, 255, 0, 255));
    ui_progress_bar_set_tint(practice_percent_prog, C2D_Color32(0, 255, 255, 255));
    ui_progress_bar_set_tint(song_progress_bar, C2D_Color32(50, 190, 240, 255));

    ui_disable_element((UIElement *) song_progress_bar);
    ui_disable_element((UIElement *) song_status_label);
    ui_disable_element((UIElement *) speed_label);
    
    play_menu_song();
    
    if (!already_played_online_level) {
        ui_disable_element((UIElement *)play_button);
    } else {
        ui_disable_element((UIElement *)spinner);
        ui_enable_element((UIElement *)play_button);
    }

    populate_level_info();

    if (!already_played_online_level) {
        level_thread = create_generic_thread(&level_task);
    }
}

static void online_level_menu_update(UIScreen *s, UIInput *i) {
    if (level_result) {
        show_level_load_error_message();
    }

    if (exiting_level) {
        update_progress_bars();
        exiting_level = false;
    }

    if (song_data_task.finished) {
        int song_data_result = -3;
        song_data_result = song_data_task.result;
        // Handle result
        if (song_data_result == 0) {
            song_data_task.finished = false;
            snprintf(song_task.song_id, sizeof(song_task.song_id), "%d", search_entries[curr_search_id].songId);
            song_task.url = song_entries[search_entries[curr_search_id].songIndex].songLink;
            song_thread = create_download_song_thread(&song_task);
        } else { handle_song_data_errors(song_data_result); }
        
    }

    if (song_task.running) {
        song_progress_bar->value = song_task.progress;

        
        char *speed = truncate_speed(song_task.speed);
        if (speed) {
            snprintf(download_speed, sizeof(download_speed), "Speed: %s", speed);
            ui_label_set_text(speed_label, download_speed);
            free(speed);
        }
    }

    // Run when finished
    if (song_task.finished) {
        // Handle result
        handle_song_codes(song_task.result);
        song_task.finished = false;
    }

    // Run when finished
    if (level_task.finished) {
        result = level_task.result;
        // Handle result
        if (result != 0 && !already_played_online_level) {
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
}

static void online_level_menu_exit() {
    if (level_entry) {
        if (level_entry->levelString) free(level_entry->levelString);
        free(level_entry);
        level_entry = NULL;
    }
    
    if (comment_entries) {
        comment_entries = NULL;
        free(comment_entries);
    }

    if (level_task.running) {
        level_task.cancelled = true;
        threadJoin(level_thread, U64_MAX);
    }

    already_played_online_level = false;
}

const UIScreenDefPair online_level_menu_def = {
    .name = "online_level_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_level_menu_top.txt",
        .init = online_level_init_top
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_menu.txt",
        .init = online_level_init,
        .update = online_level_menu_update,
        .exit = online_level_menu_exit,
        .action_list = {
            .action_count = ARRAY_LEN(online_level_actions),
            .actions = online_level_actions
        }
    }
};