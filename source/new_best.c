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

char *new_best_text[] = {
    "Not 100%",
    "Impossible timing",
    "Have you tried Story Madness yet?",
    "<DIED TO X>",
    "Not GG",
    "GGWP",
    "Git gud nub",
    "Skill issue",
    "New % when",
    "Do a practice run next time",
    "Did you reach here in the real GD?",
    "Not 0%",
    "Buff it",
    "Nerf it",
    "Level Complete... just kidding!",
    "Its easy bro, just click",
    "Never gonna give you up",
    "Wrong click pattern",
    "Cry if you want to",
    "Please scream loudly",
    "Just beat it!",
    "Progress alert!",
    "Try it in Famidash",
    "Blame it on RobTop",
    "Do you have muscle dementia",
    "New worst!"
};

char *died_to_text[DEATH_REASON_COUNT] = {
    "Bro died magically",
    "Bro died to a <i646s3>spike",
    "Bro died to a <#000000><i639s3></>saw",
    "Bro died to a <i650s3>block",
    "Bro died to a <i758s3>slope",
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

        if (settingsState.doNot) {
            char *text = new_best_text[new_best_popup.text_id];

            if (new_best_popup.text_id == DIED_TO_ID) {
                text = died_to_text[state.death_reason];
            }

            draw_text(&goldFont_fontCharset, &goldFont_sheet, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) - (NEW_BEST_SEPARATION * scale), scale, scale, 0.5f, true, "%s", text);
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