#include <3ds.h>
#include <citro2d.h>

#include "level_complete.h"

#include "menus/core/common_setters.h"

#include "math_helpers.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "mp3_player.h"
#include "level_select.h"
#include "state.h"
#include "level_loading.h"
#include "particles/circles.h"

#include "menus/components/ui_darken.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_particle.h"
#include "menus/components/ui_use_effect.h"
#include "menus/settings_hub/settings.h"

#include "utils/string_helpers.h"

#include "save/saving.h"

#define ANIM_DURATION 1.f
#define RESTART_ANIM_DURATION 0.5f

#define SECRET_COIN_UI2_ID 401

#define COIN_UNCOLLECTED_CUSTOM_ID 62
#define COIN_UNCOLLECTED_CUSTOM_SHEET 4
#define COIN_UNCOLLECTED_CUSTOM_SCALE 0.968f

static bool yes_exit = false;
static bool restart = false;
static bool init = false;

static bool animating_down = false;
static bool animating_reward = false;
static bool animating_up = false;

static float anim_time = 0;

static float up_y_start = 0;
static float window_y_pos = 0;

static int rewardAnimPhase = 0;
static float rewardAnimTime = 0.f;
static bool playedRewardSFX;

static bool showStars;

static UIScreen screen_top;
static UIScreen screen = {
    .isBottom = true
};

static UILabel *attempt_text;
static UILabel *deaths_text;
static UILabel *jumps_text;
static UILabel *time_text;

static UILabel *completion_text;

static UIImage *coins_full[3];
static UIImage *coins_base[3];
static UIParticle *particles[4];

char *practice_completion_text = "Well done... Now try to complete it\nwithout any checkpoints!";

char *do_not_completion_texts[] = {
    "Not 1 attempt",
    "You beat this instead of Story Madness...",
    "That was kinda sloppy",
    "Well done... now beat it in the PC version",
    "Good, now beat it with your eyes closed",
    "!evisserpmI",
    "CBF detected, loser!",
    "Hacked. This level is clearly impossible",
    "Noclip Accuracy: 0.01%",
    "Would be better if it was a harder level",
    "Would be better if it was an easier level",
    "Auto Safe Mode cheat detected: Using a 3DS",
    "Auto Safe Mode cheat detected:\nNoclipped through the end wall",
    "I lied, you got 99%",
    "I have no words.... oh wait",
    "we really doing anything now"
};

char *completion_texts[] = {
    "Impressive",
    "Awesome!",
    "Not bad!",
    "Well done!",
    "Challenge Breaker!",
    "Warp Speed!",
    "You are... The One!",
    "How is this possible!?",
    "You beat me...",
    "Reflex Master!",
    "Skillful!",
    "Y u do dis?",
    "Good Job!",
    "Incredible!",
    "I am speechless...",
    "Brilliant!",
};


static void exit_level_complete(UIElement* e, const UIPropertyList *args) {
    if (!animating_up) {
        play_sfx(&quit_sound, 1);
        yes_exit = true;
        animating_up = true;
        animating_down = false;
        anim_time = 0;
    }
}

static void restart_level(UIElement* e, const UIPropertyList *args) {
    if (!animating_up) {
        play_sfx(&play_sound, 1);
        animating_up = true;
        animating_down = false;
        anim_time = 0;
        pause_playback_mp3();
    }
}

static void scale_bottom_buttons_anim(UIElement* e){
    UIButton *button = (UIButton *) e;

    float fade_value_scale = easeValue(ELASTIC_OUT, 0, 1, anim_time, ANIM_DURATION, 1.f);
    
    ui_element_set_scale((UIElement *) button, fade_value_scale);
}

static UIActionDef actions[] = {
    { "restart", restart_level },
    { "exit", exit_level_complete },
};

// This runs the initial animation of the menu coming off screen
static void run_start_animation(float delta) {
    float fade_value = easeValue(BOUNCE_OUT, 0, 240, anim_time, ANIM_DURATION, 1.f);\
    window_y_pos = -120 + fade_value;
    up_y_start = fade_value;

    UIDarken *darken = (UIDarken *) ui_get_element_by_tag(&screen, "endDarken");
    darken->base.opacity = clampf(anim_time, 0.f, 0.6f);
    ui_darken_reset_opacity(darken);

    ui_run_func_on_tag(&screen, "bottomWindow", scale_bottom_buttons_anim);
    
    ui_set_pos_on_tag(&screen, SCREEN_BOT_WIDTH / 2, window_y_pos, "window");
    ui_set_pos_on_tag(&screen_top, SCREEN_WIDTH / 2, window_y_pos, "window");

    if(anim_time > 0.6f){
        animating_reward = true;
    }

    // Animation end
    if (anim_time >= ANIM_DURATION) {
        animating_down = false;
        anim_time = 0;
    }
    anim_time += delta;
}

static void spawn_reward_firework(UIImage* e){
    float x = e->base.x;
    float y = e->base.y;

    UIParticle *particle = particles[rewardAnimPhase];
    ui_particle_emit(particle, 60);

    ui_set_use_effect_col(
        ui_add_use_effect(
            (UIUseEffect *) ui_get_element_by_tag(&screen_top, "rewardCircle"),
        x, y, &death_effect),
    1.f,
    level_is_unrated_online() ? USER_COIN_UNRATED_G / 255.f : (state.custom_level ? 1.f : 0.75f),
    level_is_unrated_online() ? USER_COIN_UNRATED_B / 255.f : (state.custom_level ? 1.f : 0.f));
}

// This plays the animation of the coins popping into place and the stars
static void run_rewards_animation(float delta){
    if(rewardAnimPhase > 3){
        animating_reward = false;
        return;
    }

    //move particles and use effects to align with coin when still animating the window coming onscreen
    if(animating_down || animating_up){
        ui_particle_update_pos(particles[rewardAnimPhase]);
        ui_use_effect_update_pos((UIUseEffect *) ui_get_element_by_tag(&screen_top, "rewardCircle"));
    }

    LevelData *level_data_sel = &current_level_entry->data;
    UIImage *rewardCenter = NULL;

    float scale_value = easeValue(BOUNCE_OUT, 3.f, 1.f, rewardAnimTime, 0.35f, 1.f);
    float opacity_value = easeValue(EASE_LINEAR, 0.f, 1.f, rewardAnimTime, 0.1f, 1.f);
    //coins
    if(rewardAnimPhase < 3){
        //Skip uncollected/already collected coins
        if((rewardAnimPhase == 0 && (!state.current_data.coin1 || level_data_sel->coin1))
        || (rewardAnimPhase == 1 && (!state.current_data.coin2 || level_data_sel->coin2))
        || (rewardAnimPhase == 2 && (!state.current_data.coin3 || level_data_sel->coin3))){
            rewardAnimPhase++;
            rewardAnimTime = 0;
            return;
        }

        UIImage* coin = coins_full[rewardAnimPhase];
        rewardCenter = coin;
        
        coin->base.enabled = true;

        ui_element_set_scale((UIElement *) coin, scale_value * 0.88f);

        ui_image_set_tint(coin, level_is_unrated_online()
            ? C2D_Color32(USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B, (u8)(opacity_value * 255.f))
            : C2D_Color32f(1, 1, 1, opacity_value));
    } else{
        if(showStars) {
            UIImage* star = (UIImage *) ui_get_element_by_tag(&screen_top, "star");
            UILabel* star_text = (UILabel *) ui_get_element_by_tag(&screen_top, "startext");
            rewardCenter = star;

            star->base.enabled = true;
            star_text->base.enabled = true;

            //scale around y = 128 (relative to window)
            int center = up_y_start - 128;
            star->base.y = ((scale_value * 0.7f) * (up_y_start - 127 - center) + center);
            star_text->base.y = ((scale_value * 0.55f) * (up_y_start - 83 - center) + center);

            ui_element_set_scale((UIElement *) star, scale_value * 0.7f);
            ui_element_set_scale_xy((UIElement *) star_text, scale_value * 0.55f, scale_value * 0.55f);

            ui_image_set_tint(star, C2D_Color32f(1, 1, 1, opacity_value));
            star_text->base.opacity = opacity_value;
        } else {
            return;
        }
    }

    if(rewardAnimTime >= 0.1f){
        if(!playedRewardSFX){
            play_sfx(&coin_sound, 1);
            spawn_reward_firework(rewardCenter);
            playedRewardSFX = true;
        }
    }

    if (rewardAnimTime >= 0.35f) {
        playedRewardSFX = false;
        rewardAnimPhase++;
        rewardAnimTime = 0;
    }

    rewardAnimTime += delta;
}

// This runs the animation that happens when you press "restart"
static void run_end_animation(float delta) {
    float fade_value = easeValue(QUAD_IN, up_y_start, 0, anim_time, RESTART_ANIM_DURATION, 1.f);
    window_y_pos = -120 + fade_value;

    ui_set_pos_on_tag(&screen, SCREEN_BOT_WIDTH / 2, window_y_pos, "window");
    ui_set_pos_on_tag(&screen_top, SCREEN_WIDTH / 2, window_y_pos, "window");
    
    // Animation end
    if (anim_time >= RESTART_ANIM_DURATION) {
        animating_up = false;
        restart = true;
        anim_time = 0;
    }
    anim_time += delta;
}

#define COMPLETION_TEXT_MAX_WIDTH 250.f

static const UIScreenDefinition level_complete_def = {
    .action_list = {
        .actions = actions,
        .action_count = ARRAY_LEN(actions)
    }
};

static void enable_saved_coins(LevelData *level_data_sel) {
    for (int i = 0; i < 3; i++) {
        bool alreadyCollectedCoin = false;
        if ((i == 0 && level_data_sel->coin1)
        || (i == 1 && level_data_sel->coin2)
        || (i == 2 && level_data_sel->coin3)) {
            alreadyCollectedCoin = true;
        }

        UIImage *coin = coins_full[i];

        if (alreadyCollectedCoin) {
            coin->base.enabled = true;
            coin->base.opacity = 1.f;

            ui_element_set_scale((UIElement *) coin, 0.88f);
        }
    }
}

void level_complete_init() {
    init = true;
    in_level_complete = true;

    level_unrated_online_refresh();
    
    ui_unload_screen(&screen);
    ui_unload_screen(&screen_top);
    
    screen.def = &level_complete_def;

    ui_load_screen_old(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete_top.txt");
    ui_load_screen_old(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete.txt");

    state.current_data.time_end = svcGetSystemTick() / (CPU_TICKS_PER_MSEC * 1000);

    char attempts[64];
    char deaths[64];
    char jumps[64];
    char time[64];

    snprintf(attempts, sizeof(attempts), "Attempts: %d", state.current_data.attempts);
    snprintf(deaths, sizeof(deaths), "Deaths: %d", state.current_data.deaths + state.current_data.noclip_deaths);
    snprintf(jumps, sizeof(jumps), "Jumps: %d", state.current_data.jumps);

    float timer = state.current_data.time_end - state.current_data.time_start;

    int hours   = (int)(timer) / (60 * 60);
    int minutes = (int)(timer / 60) % 60;
    int seconds = (int)(timer) % 60;

    if (hours) {
        snprintf(time, sizeof(time), "Time: %d:%02d:%02d", hours, minutes, seconds);
    } else {
        snprintf(time, sizeof(time), "Time: %02d:%02d", minutes, seconds);
    }

    attempt_text = (UILabel *) ui_get_element_by_tag(&screen_top, "attempts");
    if (attempt_text) ui_label_set_text(attempt_text, attempts);

    deaths_text = (UILabel *) ui_get_element_by_tag(&screen_top, "deaths");
    if (deaths_text) {
        ui_label_set_text(deaths_text, deaths);
        if (cheats_used[CHEAT_NOCLIP]) ui_enable_element((UIElement *) deaths_text);
        else ui_disable_element((UIElement *) deaths_text);
    }

    jumps_text = (UILabel *) ui_get_element_by_tag(&screen_top, "jumps");
    if (jumps_text) ui_label_set_text(jumps_text, jumps);

    time_text = (UILabel *) ui_get_element_by_tag(&screen_top, "time");
    if (time_text) ui_label_set_text(time_text, time);

    yes_exit = false;
    restart = false;
    animating_down = true;
    animating_reward = false;
    animating_up = false;
    anim_time = 0;
    window_y_pos = 0;

    rewardAnimPhase = 0;
    rewardAnimTime = 0.f;
    playedRewardSFX = false;

    coins_full[0] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin1full");
    coins_full[1] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin2full");
    coins_full[2] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin3full");

    coins_base[0] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin1");
    coins_base[1] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin2");
    coins_base[2] = (UIImage *) ui_get_element_by_tag(&screen_top, "coin3");

    int coin_display_count = get_level_coin_count();
    if (coin_display_count > 3) coin_display_count = 3;

    if (state.custom_level) {
        for (int i = 0; i < 3; i++) {
            ui_image_set_image(coins_full[i], SECRET_COIN_UI2_ID, 0);
            if (level_is_unrated_online()) ui_image_set_tint(coins_full[i], USER_COIN_UNRATED_TINT);
        }
    }

    ui_run_func_on_tag(&screen_top, "coinfull", ui_disable_element);

    particles[0] = (UIParticle *) ui_get_element_by_tag(&screen_top, "coinParticle1");
    particles[1] = (UIParticle *) ui_get_element_by_tag(&screen_top, "coinParticle2");
    particles[2] = (UIParticle *) ui_get_element_by_tag(&screen_top, "coinParticle3");
    particles[3] = (UIParticle *) ui_get_element_by_tag(&screen_top, "starParticle");

    if (level_is_unrated_online()) {
        for (int i = 0; i < 3; i++) {
            particles[i]->particle.cfg.startColorRed    = USER_COIN_UNRATED_R / 255.f;
            particles[i]->particle.cfg.startColorGreen  = USER_COIN_UNRATED_G / 255.f;
            particles[i]->particle.cfg.startColorBlue   = USER_COIN_UNRATED_B / 255.f;
            particles[i]->particle.cfg.finishColorRed   = USER_COIN_UNRATED_R / 255.f;
            particles[i]->particle.cfg.finishColorGreen = USER_COIN_UNRATED_G / 255.f;
            particles[i]->particle.cfg.finishColorBlue  = USER_COIN_UNRATED_B / 255.f;
        }
    }

    // Set completion text
    completion_text = (UILabel *) ui_get_element_by_tag(&screen_top, "funnytext");
    
    LevelData *level_data_sel = &current_level_entry->data;

    UILabel *star_text = (UILabel *) ui_get_element_by_tag(&screen_top, "startext");

    char star_count[4];
    int stars = level_data_sel->stars;
    snprintf(star_count, sizeof(star_count), "+%d", stars);
    ui_label_set_text(star_text, star_count);

    ui_disable_element(ui_get_element_by_tag(&screen_top, "star"));
    ui_disable_element((UIElement *) star_text);

    showStars = stars > 0 && level_data_sel->normal_progress < 100;

    if(state.custom_level == true || state.practice_mode || cheated) {
        if (state.custom_level && !state.practice_mode && !cheated) {
            for (int k = 0; k < 3; k++) {
                bool saved = (k == 0 && level_data_sel->coin1)
                          || (k == 1 && level_data_sel->coin2)
                          || (k == 2 && level_data_sel->coin3);
                if (k < coin_display_count && !saved) {
                    ui_image_set_image(coins_base[k], COIN_UNCOLLECTED_CUSTOM_ID, COIN_UNCOLLECTED_CUSTOM_SHEET);
                    ui_element_set_scale((UIElement *) coins_base[k], COIN_UNCOLLECTED_CUSTOM_SCALE);
                    if (level_is_unrated_online()) ui_image_set_tint(coins_base[k], USER_COIN_UNRATED_TINT);
                    coins_base[k]->base.enabled = true;
                } else {
                    coins_base[k]->base.enabled = false;
                }
            }
            enable_saved_coins(level_data_sel);
        } else {
            ui_run_func_on_tag(&screen_top, "coin1", ui_disable_element);
            ui_run_func_on_tag(&screen_top, "coin2", ui_disable_element);
            ui_run_func_on_tag(&screen_top, "coin3", ui_disable_element);
        }
    
        int start_index = 0;

        // Skip the "Not 1 attempt" line
        if (settingsState.doNot) {
            if (state.current_data.attempts == 1) start_index++;
        }

        int text_index = random_int(start_index, (settingsState.doNot ? ARRAY_LEN(do_not_completion_texts) : ARRAY_LEN(completion_texts))- 1);

        if (settingsState.doNot) {
            // If exactly 2 attempts, say "Not 1 attempt"
            if (state.current_data.attempts == 2) text_index = 0;

            // If on story madness, reroll to not say the story madness line
            if (text_index == 1 && contains(level_info.level_name, "story madness")) {
                do {
                    text_index = random_int(start_index, (settingsState.doNot ? ARRAY_LEN(do_not_completion_texts) : ARRAY_LEN(completion_texts))- 1);
                } while(text_index == 1);
            }
        }

        char *text = (settingsState.doNot ? do_not_completion_texts[text_index] : completion_texts[text_index]);

        char tmp[512];

        if (state.practice_mode) {
            text = practice_completion_text;
        }
        
        if (cheated) {
            char enabled_cheats[256] = "";
            size_t len = 0;
            
            for (int i = 0; i < CHEAT_COUNT; i++) {
                if (cheats_used[i]) {
                    if (len > 0) {
                        len += snprintf(enabled_cheats + len,
                                        sizeof(enabled_cheats) - len,
                                        ", ");
                    }

                    len += snprintf(enabled_cheats + len,
                                    sizeof(enabled_cheats) - len,
                                    "%s",
                                    cheat_names[i]);
                }
            }
            snprintf(tmp, sizeof(tmp), "Safe mode - %s", enabled_cheats);
            text = tmp;
        }
        
        ui_label_set_text(completion_text, text);
    } else {
        ui_run_func_on_tag(&screen_top, "funnytext", ui_disable_element);

        enable_saved_coins(level_data_sel);
    }

    for (int k = coin_display_count; k < 3; k++) {
        if (coins_base[k]) coins_base[k]->base.enabled = false;
    }

    for (int k = 0; k < 3; k++) {
        float coin_x = (SCREEN_WIDTH / 2.f) + (k - (coin_display_count - 1) / 2.f) * 50.f;
        if (coins_base[k]) ui_element_set_position((UIElement *) coins_base[k], coin_x, coins_base[k]->base.y);
        if (coins_full[k]) ui_element_set_position((UIElement *) coins_full[k], coin_x, coins_full[k]->base.y);
        if (particles[k]) {
            particles[k]->base.x = coin_x;
            ui_particle_update_pos(particles[k]);
        }
    }

    if (get_level_coin_count() > 0 && !state.practice_mode && !cheated) {
        ui_run_func_on_tag(&screen_top, "funnytext", ui_disable_element);
    }

    if (state.practice_mode) {
        ui_run_func_on_tag(&screen_top, "levelcomplete", ui_disable_element);
        if (settingsState.doNot) {
            UIImage *level_complete_text = (UIImage *) ui_get_element_by_tag(&screen_top, "practicecomplete");
            if (level_complete_text) {
                ui_element_set_scale_x((UIElement *) level_complete_text, level_complete_text->base.scaleX * -1);
            }
        }
    } else {
        ui_run_func_on_tag(&screen_top, "practicecomplete", ui_disable_element);
        if (settingsState.doNot) {
            UIImage *level_complete_text = (UIImage *) ui_get_element_by_tag(&screen_top, "levelcomplete");
            if (level_complete_text) {
                ui_element_set_scale_x((UIElement *) level_complete_text, level_complete_text->base.scaleX * -1);
            }
        }
    }

    ui_get_element_by_tag(&screen, "endDarken")->opacity = 0.f;
}

int level_complete_loop(UIInput *touch) {
    if (!init) return 0;

    if (animating_down) run_start_animation(delta);
    if (animating_reward && !state.practice_mode && !cheated) run_rewards_animation(delta);
    if (animating_up) run_end_animation(delta);

    if (yes_exit) {
        return 1;
    }

    if (restart) {
        return 2;
    }

    ui_screen_update(&screen, touch);
    ui_screen_update(&screen_top, touch);

    return 0;
}

void level_complete_destroy() {
    init = false;
}

void draw_level_complete() {
    if (init) {
        ui_screen_draw(&screen);
    }
}
void draw_level_complete_top() {
    if (init) {
        ui_screen_draw(&screen_top);
    }
}
