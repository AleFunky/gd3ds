#include "new_best.h"
#include "math_helpers.h"
#include "main.h"
#include "state.h"
#include "utils/gfx.h"
#include "easing.h"
#include "graphics.h"
#include "save/config.h"

#include "menus/core/ui_screen.h"
#include "menus/settings_hub/settings.h"
#include "menus/level_select.h"

#include "fonts/goldFont.h"
#include "fonts/bigFont.h"

#include "utils/string_helpers.h"

#define NOT_100_ID 0
#define IMPOSSIBLE_TIMING_ID 1
#define STORY_MADNESS_ID 2
#define DIED_TO_ID 3
#define HARDER_THAN_ID 4

char *new_best_text[] = {
    [NOT_100_ID] = "Not 100%",
    [IMPOSSIBLE_TIMING_ID] = "Impossible timing",
    [STORY_MADNESS_ID] = "Have you tried Story Madness yet?",
    [DIED_TO_ID] = "<DIED TO X>",
    [HARDER_THAN_ID] = "Harder than %d%% btw!",
    "<#5AFFFF>I'm green for an amazing reason</>",
    "6000 FPS would've fixed that",
    "!tseB weN",
    "+1 dollar",
    ":(",
    "A swing and a miss...",
    "Absolutely peak",
    "Ain't going anywhere",
    "Alefunky detected progress",
    "Another 1000 attempts and you’ll beat it",
    "At 1x speed, does this look possible to you?",
    "Avert your eyes...",
    "Back to 0%% we go",
    "Best new!",
    "Better luck next time",
    "Better than ever",
    "Better than yesterday",
    "Bew Nest!",
    "Blame it on RobTop",
    "Blame the lag",
    "Blame the low resolution display",
    "Buff it",
    "Can I offer you a New Best! in these trying times?",
    "Can we cut that out in post?",
    "Cry if you want to",
    "Did you reach here in the real GD?",
    "Dies of death",
    "Do a practice run next time",
    "Do it the same, but, uhhh, better",
    "Do you have muscle dementia",
    "Erm... so that just happened!",
    "Failure.",
    "Focus, M.",
    "free click",
    "GAME OVER!",
    "Generation Retro is better",
    "Get back on grief",
    "GGWP",
    "Git gud nub",
    "Git Gud",
    "Give up",
    "Go to sleep",
    "GO",
    "Good job!",
    "Good luck on veryfing grief",
    "Good one",
    "Hands cramped yet?",
    "Have you tried Story Madness yet?",
    "haxx0r mode disabled lololol",
    "hey i didn't see it, did it go in? i wasn't looking did you win? did you hit the end wall",
    "Hey. Go wash your dishes",
    "HIGH SCORE!",
    "horse",
    "How can you die it's just pixels on screen",
    "HOW",
    "I bet that hurt",
    "I can do better",
    "Impossible timing",
    "It was the fps!!",
    "Its easy bro, just click",
    "jajajaja you have not win",
    "Just beat it!",
    "Keep going, I'm betting on you",
    "Lag is not an excuse",
    "Let me guess, it didn't register",
    "Level Complete... just kidding!",
    "Level Uncomplete!",
    "Lol",
    "Maybe next time",
    "Maybe you should eat something",
    "mefewe 98%% was worse",
    "Most cubes can interact with spikes just fine, yours is just allergic.",
    "Nerf it",
    "Never gonna give you up",
    "New %% when",
    "NEW BEST! INTERESTING",
    "New worst!",
    "Not 0%%",
    "Not 97%% btw",
    "Not 100%%",
    "Not event 1%% of Grief",
    "Not GG",
    "Not quite my tempo",
    "Oh that me? Pure aura!",
    "Okay buddy",
    "oops mb",
    "Oops",
    "Ouch",
    "peter alert",
    "Please scream loudly",
    "Poor screen...",
    "Press ALT F4 for noclip",
    "Pro combination: Home Button then X",
    "Progress alert!",
    "Progress NOT saved loser",
    "Progress? In this economy?",
    "Push further!",
    "Quit while you're ahead",
    "Ragebait",
    "Really?!",
    "Revive for $2.99",
    "Rip bozo",
    "Sad",
    "Should've dodged that GameObject",
    "SIGSEGV",
    "Skill issue",
    "Stop clip farming",
    "STOP DYING THERE",
    "That was fun to watch",
    "That was kinda amateurish",
    "That was painful to watch",
    "There's a long path ahead still!",
    "This is a new best message",
    "Thought you were gonna fluke it?",
    "Too late... or too early",
    "Try harder next time",
    "Try it in Famidash",
    "Try with your feet",
    "Well",
    "Welp",
    "What did I miss?",
    "What's this, some kind of impossible timing?",
    "Whatever",
    "Why didn't you jump",
    "womp womp",
    "Wrap it up",
    "Wrong click pattern",
    "YOLO attempt",
    "you actually SUCK and it was a fluke",
    "you are fuk map",
    "You didn't do it!",
    "You really couldn't do better?",
    "You should try practice mode",
    "You successed failfully"
};

char *died_to_text[DEATH_REASON_COUNT] = {
    "Bro died magically",
    "Bro died to a <i646s3>spike",
    "Bro died to a <#000000><i639s3></>saw",
    "Bro died to a <i696s3>block",
    "Bro died to a <i731s3>slope",
    "Bro fell out of the level",
    "Bro died to the ground",
};

typedef enum {
    SCALE_IN,
    WAITING,
    SCALE_OUT,
} NewBestState;

typedef struct {
    bool active;

    int progress;
    int text_id;
    
    NewBestState state;

    float scale;

    float initial_scale;
    float target_scale;
    float timer;
    float duration;
    EaseTypes ease;
} NewBestPopup;

static NewBestPopup new_best_popup;

void init_new_best_popup(int progress) {
    new_best_popup.active = true;
    new_best_popup.state = SCALE_IN;
    new_best_popup.ease = ELASTIC_OUT;

    new_best_popup.text_id = random_int(0, ARRAY_LEN(new_best_text) - 1);
    new_best_popup.progress = progress;
    

    // 50% chance on >95% death to say "not 100%"
    if (progress >= 95 && random_int(0, 100) <= 50) {
        new_best_popup.text_id = NOT_100_ID;
    }

    // Cycles impossible timing
    if (progress == 42 && !state.custom_level && curr_level_id == 8 && state.player.y > 140) {
        new_best_popup.text_id = IMPOSSIBLE_TIMING_ID;
    }

    // If on story madness, reroll to not say the story madness line
    if (new_best_popup.text_id == STORY_MADNESS_ID && contains(level_info.level_name, "story madness")) {
        do {
            new_best_popup.text_id = random_int(0, ARRAY_LEN(new_best_text) - 1);
        } while(new_best_popup.text_id == STORY_MADNESS_ID);
    }

    new_best_popup.timer = 0;
    new_best_popup.duration = NEW_BEST_STATE_0_DURATION;
    new_best_popup.initial_scale = 0.01;
    new_best_popup.target_scale = NEW_BEST_STATE_0_TARGET_SCALE;
}

void clear_new_best_popup() {
    new_best_popup.active = false;
}

void handle_new_best_popup(float delta) {
    if (new_best_popup.active) {
        switch (new_best_popup.state) {
            case SCALE_IN:
                if (new_best_popup.timer > NEW_BEST_STATE_0_DURATION) {
                    new_best_popup.state = WAITING;
                    new_best_popup.ease = EASE_LINEAR;

                    new_best_popup.timer = 0;
                    new_best_popup.duration = NEW_BEST_STATE_1_DURATION;
                    new_best_popup.initial_scale = new_best_popup.scale;
                    new_best_popup.target_scale = new_best_popup.scale;
                }
                break;
            case WAITING:
                if (new_best_popup.timer > NEW_BEST_STATE_1_DURATION) {
                    new_best_popup.state = SCALE_OUT;
                    new_best_popup.ease = QUAD_IN;
                    
                    new_best_popup.timer = 0;
                    new_best_popup.duration = NEW_BEST_STATE_2_DURATION;
                    new_best_popup.initial_scale = new_best_popup.scale;
                    new_best_popup.target_scale = NEW_BEST_STATE_2_TARGET_SCALE;
                }
                break;
            case SCALE_OUT:
                if (new_best_popup.timer > NEW_BEST_STATE_2_DURATION) {
                    new_best_popup.active = false;
                }
                break;
        }
        new_best_popup.scale = easeValue(new_best_popup.ease, new_best_popup.initial_scale, new_best_popup.target_scale, new_best_popup.timer, new_best_popup.duration, 1.f);

        new_best_popup.timer += delta;
    }
}

void draw_new_best_popup() {
    if (new_best_popup.active) {
        float scale = new_best_popup.scale;
        char buf[256];

        if (settingsState.doNot) {
            char *text = new_best_text[new_best_popup.text_id];

            if (new_best_popup.text_id == DIED_TO_ID) {
                text = died_to_text[state.death_reason];
            } 

            if (new_best_popup.text_id == HARDER_THAN_ID) {
                snprintf(buf, sizeof(buf), text, new_best_popup.progress - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s", text);
            }

            float text_scale = scale;

            int width = 390;
            float length = get_longest_line_length(&goldFont_fontCharset, NEW_BEST_STATE_0_TARGET_SCALE, buf);

            if (width < length && length > 0) {
                text_scale *= (width / length);
            }
            
            draw_text(&goldFont_fontCharset, &goldFont_sheet, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) - (NEW_BEST_SEPARATION * text_scale), text_scale, text_scale, 0.5f, true, buf);
        } else {
            C2D_Sprite text = { 0 };
            C2D_SpriteFromSheet(&text, ui_sheet, (NEW_BEST_IMAGE_ID));
            C3D_TexSetFilter(text.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&text, 0.5f, 0.5f);
            C2D_SpriteSetPos(&text, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) - (NEW_BEST_SEPARATION * scale));
            C2D_SpriteSetScale(&text, scale, scale);
            C2D_DrawSprite(&text);
        }

        
        draw_text(&bigFont_fontCharset, &bigFont_sheet, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) + (NEW_BEST_SEPARATION * scale), scale, scale, 0.5f, true, "%d%%", new_best_popup.progress);
    }
}