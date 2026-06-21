#include <3ds.h>
#include <citro2d.h>

#include "level_complete.h"

#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "info_card.h"
#include "state.h"
#include "endwall.h"
#include "menus/components/ui_darken.h"
#include "main.h"
#include "particles/circles.h"
#include "menus/components/ui_particle.h"
#include "menus/components/ui_use_effect.h"

#include "menus/settings.h"
#include "utils/string_helpers.h"

#include "save/saving.h"

#define ANIM_DURATION 1.f
#define RESTART_ANIM_DURATION 0.5f

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

static UIElement *attempt_text;
static UIElement *jumps_text;
static UIElement *time_text;

static UIElement *completion_text;

static UIElement *coins_full[3];
static UIElement *particles[4];

char *practice_completion_text = "Well done... Now try to complete it<p>without any checkpoints!";

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
    "Auto Safe Mode cheat detected:<p>Noclipped through the end wall",
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

#define NUM_COMPLETION_TEXTS (sizeof(completion_texts) / sizeof(char *))
#define NUM_DO_NOCOMPLETION_TEXTS (sizeof(do_not_completion_texts) / sizeof(char *))


static void exit_level_complete(UIElement* e) {
    if (!animating_up) {
        play_sfx(&quit_sound, 1);
        yes_exit = true;
        animating_up = true;
        animating_down = false;
        anim_time = 0;
    }
}

static void restart_level(UIElement* e) {
    if (!animating_up) {
        ui_get_element_by_tag(&screen, "endDarken")->opacity = 0.f;
        ui_get_element_by_tag(&screen_top, "endDarken")->opacity = 0.f;
        
        play_sfx(&play_sound, 1);
        animating_up = true;
        animating_down = false;
        anim_time = 0;
        pause_playback_mp3();
    }
}

static void scale_bottom_buttons_anim(UIElement* e){
    float fade_value_scale = easeValue(ELASTIC_OUT, 0, 1, anim_time, ANIM_DURATION, 1.f);
    e->button.scaleX = fade_value_scale;
    e->button.scaleY = fade_value_scale;
}

static UIAction actions[] = {
    { "restart", restart_level },
    { "exit", exit_level_complete },
};

// This runs the initial animation of the menu coming off screen
static void run_start_animation(float delta) {
    float fade_value = easeValue(BOUNCE_OUT, 0, 240, anim_time, ANIM_DURATION, 1.f);\
    window_y_pos = -120 + fade_value;
    up_y_start = fade_value;

    UIElement *darken = ui_get_element_by_tag(&screen, "endDarken");
    darken->opacity = clampf(anim_time, 0.f, 0.6f);
    ui_darken_reset_opacity(darken);

    UIElement *darken_top = ui_get_element_by_tag(&screen_top, "endDarken");
    darken_top->opacity = clampf(anim_time, 0.f, 0.6f);
    ui_darken_reset_opacity(darken_top);

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

static void spawn_reward_firework(UIElement* e){
    float x = e->x;
    float y = e->y;
    UIElement *particle = particles[rewardAnimPhase];
    ui_particle_emit(particle, 60);

    ui_set_use_effect_col(
        ui_add_use_effect(
            ui_get_element_by_tag(&screen_top, "rewardCircle"),
        x, y, &death_effect),
    1.f, 0.75f, 0.f);
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
        ui_use_effect_update_pos(ui_get_element_by_tag(&screen_top, "rewardCircle"));
    }

    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);
    UIElement *rewardCenter = NULL;

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

        UIElement* coin = coins_full[rewardAnimPhase];
        rewardCenter = coin;
        
        coin->enabled = true;
        coin->image.scaleX = scale_value * 0.88f;
        coin->image.scaleY = scale_value * 0.88f;

        ui_image_set_tint(coin, C2D_Color32f(1, 1, 1, opacity_value));
    } else{
        if(showStars) {
            UIElement* star = ui_get_element_by_tag(&screen_top, "star");
            UIElement* star_text = ui_get_element_by_tag(&screen_top, "startext");
            rewardCenter = star;

            star->enabled = true;
            star_text->enabled = true;

            //scale around y = 128 (relative to window)
            int center = up_y_start - 128;
            star->y = ((scale_value * 0.7f) * (up_y_start - 127 - center) + center);
            star_text->y = ((scale_value * 0.55f) * (up_y_start - 83 - center) + center);

            star->image.scaleX = scale_value * 0.7f;
            star->image.scaleY = scale_value * 0.7f;
            star_text->label.scale = scale_value * 0.55f;

            ui_image_set_tint(star, C2D_Color32f(1, 1, 1, opacity_value));
            star_text->opacity = opacity_value;
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

void level_complete_init() {
    init = true;
    in_level_complete = true;
    ui_load_screen(&screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete_top.txt");
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete.txt");

    state.current_data.time_end = svcGetSystemTick() / (CPU_TICKS_PER_MSEC * 1000);

    char attempts[64];
    char jumps[64];
    char time[64];

    snprintf(attempts, sizeof(attempts), "Attempts: %d", state.current_data.attempts);
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

    attempt_text = ui_get_element_by_tag(&screen_top, "attempts");
    if (attempt_text) ui_label_set_text(attempt_text, attempts);

    jumps_text = ui_get_element_by_tag(&screen_top, "jumps");
    if (jumps_text) ui_label_set_text(jumps_text, jumps);

    time_text = ui_get_element_by_tag(&screen_top, "time");
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

    coins_full[0] = ui_get_element_by_tag(&screen_top, "coin1full");
    coins_full[1] = ui_get_element_by_tag(&screen_top, "coin2full");
    coins_full[2] = ui_get_element_by_tag(&screen_top, "coin3full");

    ui_run_func_on_tag(&screen_top, "coinfull", ui_disable_element);

    particles[0] = ui_get_element_by_tag(&screen_top, "coinParticle1");
    particles[1] = ui_get_element_by_tag(&screen_top, "coinParticle2");
    particles[2] = ui_get_element_by_tag(&screen_top, "coinParticle3");
    particles[3] = ui_get_element_by_tag(&screen_top, "starParticle");

    // Set completion text
    completion_text = ui_get_element_by_tag(&screen_top, "funnytext");
    
    LevelData *level_data_sel = (state.custom_level ? &level_data : &main_level_data[curr_level_id]);

    UIElement *star_text = ui_get_element_by_tag(&screen_top, "startext");

    char star_count[4];
    int stars = state.custom_level ? level_data_sel->stars : main_levels[curr_level_id].stars;
    snprintf(star_count, sizeof(star_count), "+%d", stars);
    ui_label_set_text(star_text, star_count);

    ui_disable_element(ui_get_element_by_tag(&screen_top, "star"));
    ui_disable_element(star_text);

    showStars = stars > 0 && level_data_sel->normal_progress < 100;

    if(state.custom_level == true || state.practice_mode || cheated) {
        ui_run_func_on_tag(&screen_top, "coin1", ui_disable_element);
        ui_run_func_on_tag(&screen_top, "coin2", ui_disable_element);
        ui_run_func_on_tag(&screen_top, "coin3", ui_disable_element);
    
        int start_index = 0;

        // Skip the "Not 1 attempt" line
        if (doNot) {
            if (state.current_data.attempts == 1) start_index++;
        }

        int text_index = random_int(start_index, (doNot ? NUM_DO_NOCOMPLETION_TEXTS : NUM_COMPLETION_TEXTS)- 1);

        if (doNot) {
            // If exactly 2 attempts, say "Not 1 attempt"
            if (state.current_data.attempts == 2) text_index = 0;

            // If on story madness, reroll to not say the story madness line
            if (text_index == 1 && contains(level_info.level_name, "story madness")) {
                do {
                    text_index = random_int(start_index, (doNot ? NUM_DO_NOCOMPLETION_TEXTS : NUM_COMPLETION_TEXTS)- 1);
                } while(text_index == 1);
            }
        }

        char *text = (doNot ? do_not_completion_texts[text_index] : completion_texts[text_index]);

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

        float text_scale;
        float scale = completion_text->label.scale;

        // Get text length in pixels
        float length = get_longest_line_length(&bigFont_fontCharset, scale, text);
    
        if (COMPLETION_TEXT_MAX_WIDTH < length) {
            text_scale = scale * (COMPLETION_TEXT_MAX_WIDTH / length);
        } else {
            text_scale = scale;
        }

        completion_text->label.scale = text_scale;
    } else {
        ui_run_func_on_tag(&screen_top, "funnytext", ui_disable_element);

        for(int i = 0; i < 3; i++){
            bool alreadyCollectedCoin = false;
            if((i == 0 && level_data_sel->coin1)
            || (i == 1 && level_data_sel->coin2)
            || (i == 2 && level_data_sel->coin3)){
                alreadyCollectedCoin = true;
            }
            
            UIElement* coin = coins_full[i];

            if(alreadyCollectedCoin){
                coin->enabled = true;
                coin->opacity = 1.f;
                coin->image.scaleX = 0.88f;
                coin->image.scaleY = 0.88f;
            }
        }
    }

    if (state.practice_mode) {
        ui_run_func_on_tag(&screen_top, "levelcomplete", ui_disable_element);
        if (doNot) ui_get_element_by_tag(&screen_top, "practicecomplete")->image.scaleX *= -1.f;
    } else {
        ui_run_func_on_tag(&screen_top, "practicecomplete", ui_disable_element);
        if (doNot) ui_get_element_by_tag(&screen_top, "levelcomplete")->image.scaleX *= -1.f;
    }

    ui_get_element_by_tag(&screen, "endDarken")->opacity = 0.f;
    ui_get_element_by_tag(&screen_top, "endDarken")->opacity = 0.f;
}

int level_complete_loop(float delta) {
    if (!init) return 0;

    if (animating_down) run_start_animation(delta);
    if (animating_reward && !state.practice_mode && !cheated) run_rewards_animation(delta);
    if (animating_up) run_end_animation(delta);

    if (yes_exit) {
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
        return 1;
    }

    if (restart) {
        ui_unload_screen(&screen);
        ui_unload_screen(&screen_top);
        return 2;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_update(&screen_top, &touch);

    return 0;
}

void level_complete_destroy() {
    init = false;
}

void draw_level_complete() {
    if (init) {
        if (get_fade_status()) {
            level_complete_loop(1.f/60);
        }

        ui_screen_draw(&screen);
    }
}
void draw_level_complete_top() {
    if (init) {
        if (get_fade_status()) {
            level_complete_loop(1.f/60);
        }
        
        ui_screen_draw(&screen_top);
    }
}