#include <3ds.h>
#include <citro2d.h>
#include "utils/precise_input.h"
#include "menus/core/common_setters.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_button.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "state.h"
#include "menus/components/ui_use_effect.h"
#include "particles/circles.h"

#include "settings.h"
#include "generic_disclaimer.h"

#include "gameplay.h"

#include "info_card.h"

#include "practice.h"

#include "save/saving.h"

bool game_paused = false;
bool in_level_complete = false;
static bool in_disclaimer = false;
static bool in_settings = false;

static UIImage *bg_gradient;
static UIProgressBar *progress_bar;
static UILabel *percent;
static UILabel *level_name;

static UIProgressBar *normal_progress;
static UILabel *normal_progress_val;
static UIProgressBar *practice_progress;
static UILabel *practice_progress_val;

static UISlider *music_slider_bar;
static UISlider *sound_slider_bar;

static UIImage *coin_1;
static UIImage *coin_2;
static UIImage *coin_3;

static UIImage *coins_full[3];
static bool coins_got[3];
static float coin_anims[3];
static bool coins_circles_spawned[3];

static UIImage *coin_1_top;
static UIImage *coin_2_top;
static UIImage *coin_3_top;

int decimal;

static void update_progress_bars() {
    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);

    normal_progress->value = level_data_sel->normal_progress;
    practice_progress->value = level_data_sel->practice_progress;

    char normal[32];
    char practice[32];
    snprintf(normal, sizeof(normal), "%d%%", level_data_sel->normal_progress);
    snprintf(practice, sizeof(practice), "%d%%", level_data_sel->practice_progress);

    ui_label_set_text(normal_progress_val, normal);
    ui_label_set_text(practice_progress_val, practice);

    ui_image_set_image(coin_1_top, (level_data_sel->coin1 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
    ui_image_set_image(coin_2_top, (level_data_sel->coin2 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
    ui_image_set_image(coin_3_top, (level_data_sel->coin3 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
}

static void reset_coin(LevelData *level_data_sel, int i){
    if((!level_data_sel->coin1 && i == 0)
        || (!level_data_sel->coin2 && i == 1)
        || (!level_data_sel->coin3 && i == 2)) { 
            coins_full[i]->base.enabled = false;
        } else {
            coins_full[i]->base.enabled = true;
    }
}

void reset_coins(){
    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);

    ui_use_effect_clear((UIUseEffect *) ui_get_element_by_tag(&default_screen, "coin_circle"));

    for(int i = 0; i < 3; i++){
        reset_coin(level_data_sel, i);
        coins_got[i] = false;
        coin_anims[i] = 0.f;
        coins_circles_spawned[i] = false;
    }
}

void pause_game() {
    if (state.end_wall_anim_playing) return;

    update_progress_bars();
    game_paused = true;
    if (song_loaded || state.practice_mode) pause_playback_mp3();
    if (!state.custom_level){
        ui_run_func_on_tag(&default_screen_top, "coin_1", ui_enable_element);
        ui_run_func_on_tag(&default_screen_top, "coin_2", ui_enable_element);
        ui_run_func_on_tag(&default_screen_top, "coin_3", ui_enable_element);
        ui_run_func_on_tag(&default_screen, "coin_1", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_2", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_3", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_1_full", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_2_full", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_3_full", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_circle", ui_disable_element);
    }
    ui_run_func_on_tag(&default_screen_top, "pause_menu", ui_enable_element);
    ui_run_func_on_tag(&default_screen, "paused", ui_enable_element);
    ui_run_func_on_tag(&default_screen, "not_paused", ui_disable_element);
    in_settings = false;
}

void unpause_game() {
    game_paused = false;
    if (pi_enabled) {
        sync_precise_input(true);
    }
    if (state.death_timer <= 0 && (song_loaded || state.practice_mode)) {
        unpause_playback_mp3();
    }
    if (!state.custom_level){
        ui_run_func_on_tag(&default_screen_top, "coin_1", ui_disable_element);
        ui_run_func_on_tag(&default_screen_top, "coin_2", ui_disable_element);
        ui_run_func_on_tag(&default_screen_top, "coin_3", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_1", ui_enable_element);
        ui_run_func_on_tag(&default_screen, "coin_2", ui_enable_element);
        ui_run_func_on_tag(&default_screen, "coin_3", ui_enable_element);
        for(int i = 0; i < 3; i++){
            reset_coin((state.custom_level ? &level_data : &main_level_data[curr_level_id]), i);
        }
        ui_run_func_on_tag(&default_screen, "coin_circle", ui_enable_element);
    }
    ui_run_func_on_tag(&default_screen_top, "pause_menu", ui_disable_element);
    ui_run_func_on_tag(&default_screen, "paused", ui_disable_element);
    ui_run_func_on_tag(&default_screen, "not_paused", ui_enable_element);
    
    if (!state.practice_mode) {
        ui_run_func_on_tag(&default_screen, "practice_buttons", ui_disable_element);
    }
    in_settings = false;
}

static void exit_level() {
    if (!exiting_level && game_paused){
        play_sfx(&quit_sound, 1);
        exiting_level = true;
        set_fade_status(FADE_STATUS_OUT);
    }
}

static void restart_level() {
    init_variables();
    reload_level();
    if (state.practice_mode) {
        if (settingsState.autoCheckpoints && player_gamemode_is_flying(&state.death_player) && pseudo_checkpoint_exists) {
            delete_last_checkpoint();
            pseudo_checkpoint_exists = false;
        }
        
        if (get_checkpoint_count() > 0) {
            restore_checkpoint();
        }
    } else if (song_loaded) seek_mp3(level_info.song_offset);

    reset_coins();

    unpause_game();
}

void open_settings() {
    in_settings = true;
    settings_init();
}

static void action_pause(UIElement *e) { 
    pause_game();
}

static void action_unpause(UIElement *e) {
    unpause_game();
}

static void action_exit(UIElement *e) {
    exit_level();
}

static void action_restart(UIElement *e) {
    restart_level();
}

static void action_open_settings(UIElement *e) {
    open_settings();
}

static void action_practice_mode(UIElement *e) {
    if (!state.practice_mode) {
        start_practice_mode();
    } else {
        exit_practice_mode();
    }

    action_unpause(e);
}

static void action_add_checkpoint(UIElement *e) {
    new_checkpoint();
}
static void action_remove_checkpoint(UIElement *e) {
    delete_last_checkpoint();
}

static UIAction actions[] = {
    {"pause", action_pause },
    {"unpause", action_unpause },
    {"exit", action_exit },
    {"restart", action_restart },
    {"settings", action_open_settings },
    {"practice", action_practice_mode },
    {"add_check", action_add_checkpoint },
    {"remove_check", action_remove_checkpoint },
};

void gameplay_screen_init() {
    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/gameplay.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");

    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/gameplay_top.txt");;
    progress_bar = (UIProgressBar *) ui_get_element_by_tag(&default_screen_top, "progressalert");
    percent = (UILabel *) ui_get_element_by_tag(&default_screen_top, "percent");
    level_name = (UILabel *) ui_get_element_by_tag(&default_screen_top, "level_title");

    Color color = get_white_if_black(p1_color);

    ui_progress_bar_set_tint(progress_bar, C2D_Color32(color.r, color.g, color.b, 255));
    
    ui_window_set_tint((UIWindow *) ui_get_element_by_tag(&default_screen_top, "bgwindow"), C2D_Color32(0, 0, 0, 150));
    ui_window_set_tint((UIWindow *) ui_get_element_by_tag(&default_screen, "bgwindow"), C2D_Color32(0, 0, 0, 150));

    ui_label_set_text(level_name, level_info.level_name);

    normal_progress = (UIProgressBar *) ui_get_element_by_tag(&default_screen_top, "normalprogress");
    normal_progress_val = (UILabel *) ui_get_element_by_tag(&default_screen_top, "normalprogressvalue");
    practice_progress = (UIProgressBar *) ui_get_element_by_tag(&default_screen_top, "practiceprogress");
    practice_progress_val = (UILabel *) ui_get_element_by_tag(&default_screen_top, "practiceprogressvalue");
    
    ui_progress_bar_set_tint(normal_progress, C2D_Color32(0, 255, 0, 255));
    ui_progress_bar_set_tint(practice_progress, C2D_Color32(0, 255, 255, 255));

    // hide coins if level is a custom level
    if(state.custom_level == true){
        ui_run_func_on_tag(&default_screen, "coin_1", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_2", ui_disable_element);
        ui_run_func_on_tag(&default_screen, "coin_3", ui_disable_element);
    }
    
    // Hide pause menu
    ui_run_func_on_tag(&default_screen_top, "coin_1", ui_disable_element);
    ui_run_func_on_tag(&default_screen_top, "coin_2", ui_disable_element);
    ui_run_func_on_tag(&default_screen_top, "coin_3", ui_disable_element);
    ui_run_func_on_tag(&default_screen_top, "pause_menu", ui_disable_element);
    ui_run_func_on_tag(&default_screen, "paused", ui_disable_element);
    ui_run_func_on_tag(&default_screen, "practice_buttons", ui_disable_element);

    coin_1 = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_1");
    coin_2 = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_2");
    coin_3 = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_3");

    coins_full[0] = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_1_full");
    coins_full[1] = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_2_full");
    coins_full[2] = (UIImage *) ui_get_element_by_tag(&default_screen, "coin_3_full");
    
    reset_coins();

    coin_1_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "coin_1");
    coin_2_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "coin_2");
    coin_3_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "coin_3");

    music_slider_bar = (UISlider*) ui_get_element_by_tag(&default_screen, "music_slider");
    sound_slider_bar = (UISlider*) ui_get_element_by_tag(&default_screen, "sound_slider");

    if (music_slider_bar) music_slider_bar->value = music_volume;
    if (sound_slider_bar) sound_slider_bar->value = sound_volume;
}

int gameplay_screen_top_loop() { 
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);

    decimal = 0;
    if (settingsState.decimalPercent) decimal = 2;
    if (settingsState.ultraDecimalPercent) decimal = MAX_DECIMAL_PERCENT;

    progress_bar->value = state.level_progress;
    snprintf(percent->text, 32, "%.*f%%", decimal, state.level_progress);

    ui_run_func_on_tag(&default_screen_top, "progressalert", ui_disable_element);
    ui_run_func_on_tag(&default_screen_top, "percent", ui_disable_element);
    ui_set_pos_on_tag(&default_screen_top, 200, 8, "percent");
    percent->alignment = 0.5;

    if (settingsState.showProgressBar) {
        ui_run_func_on_tag(&default_screen_top, "progressalert", ui_enable_element);
        ui_set_pos_on_tag(&default_screen_top, 282, 8, "percent");
        percent->alignment = 0;
    }

    if (settingsState.showProgressPercent) {
        ui_run_func_on_tag(&default_screen_top, "percent", ui_enable_element);
    }

    // Extra eyes only redraw, updating twice would run the animations at double speed
    if (!is_extra_eye()) ui_screen_update(&default_screen_top, &touch);
    ui_screen_draw(&default_screen_top);

    return false;
}

int gameplay_screen_bot_loop() {
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);

    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);

    coins_got[0] = state.current_data.coin1 && !level_data_sel->coin1;
    coins_got[1] = state.current_data.coin2 && !level_data_sel->coin2;
    coins_got[2] = state.current_data.coin3 && !level_data_sel->coin3;

    for(int i = 0; i < 3; i++){
        if(coins_got[i] && !game_paused){
            float scale = easeValue(BOUNCE_OUT, 2.f, 0.65f, coin_anims[i], 0.35f, 1.f);
            float opacity = easeValue(EASE_LINEAR, 0.f, 1.f, coin_anims[i], 0.1f, 1.f);

            coins_full[i]->base.enabled = true;
            ui_element_set_scale((UIElement *) coins_full[i], scale);
            ui_image_set_tint(coins_full[i], C2D_Color32f(1, 1, 1, opacity));

            if(coin_anims[i] >= 0.05f){
                if(!coins_circles_spawned[i]){
                    ui_set_use_effect_col(
                        ui_add_use_effect(
                            (UIUseEffect *) ui_get_element_by_tag(&default_screen, "coin_circle"), 
                        coins_full[i]->base.x, coins_full[i]->base.y, &end_wall_firework_circle),
                    1.f, 0.75f, 0.f);
                    
                    coins_circles_spawned[i] = true;
                }
            }

            coin_anims[i] += delta;
        }
    }

    if (game_paused) {
        if (music_slider_bar) music_volume = music_slider_bar->value;
        if (sound_slider_bar) sound_volume = sound_slider_bar->value;

        apply_volume_settings();
    }

    //level end buttons retraction animation
    float cutscene_timer = state.player.cutscene_timer;
    float complete_y_offset = easeValue(EASE_IN, 20.f, -20.f, cutscene_timer, 0.3f, 1.3f);
    float complete_y_offset_practice = easeValue(EASE_IN, 200.f, 270.f, cutscene_timer, 0.3f, 1.8f);

    coin_1->base.y = complete_y_offset;
    coin_2->base.y = complete_y_offset;
    coin_3->base.y = complete_y_offset;
    coins_full[0]->base.y = complete_y_offset;
    coins_full[1]->base.y = complete_y_offset;
    coins_full[2]->base.y = complete_y_offset;

    UIElement *pause_btn = ui_get_element_by_tag(&default_screen, "pause_btn");
    pause_btn->y = complete_y_offset;
    
    UIElement *add_checkpoint = ui_get_element_by_tag(&default_screen, "add_checkpoint");
    UIElement *remove_checkpoint = ui_get_element_by_tag(&default_screen, "remove_checkpoint");
    add_checkpoint->y = complete_y_offset_practice;
    remove_checkpoint->y = complete_y_offset_practice;

    if (state.practice_mode) {
        ui_button_set_image((UIButton *) ui_get_element_by_tag(&default_screen, "practice_mode"), 124, 0);
    } else {
        ui_run_func_on_tag(&default_screen, "practice_buttons", ui_disable_element);
        ui_button_set_image((UIButton *) ui_get_element_by_tag(&default_screen, "practice_mode"), 146, 0);
    }

    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_settings && !in_disclaimer && !in_info_card) {
        ui_screen_update(&default_screen, &touch);
    }

    ui_screen_draw(&default_screen);

    if (in_settings) {
        int returned = settings_loop();
        if (returned) {
            in_settings = false;
        }
    }

    if (in_disclaimer) {
        int returned = disclaimer_loop();
        if (returned) {
            in_disclaimer = false;
        }
    }

    if (in_info_card) {
        int returned = info_card_loop();
        if (returned) {
            in_info_card = false;
        }
    }

    return false;
}
