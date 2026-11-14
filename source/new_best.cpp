#include "new_best.hpp"
#include "math_helpers.hpp"
#include "main.hpp"
#include "state.hpp"
#include "easing.hpp"
#include "graphics.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/level_select.hpp"

#include "fonts/goldFont.hpp"
#include "fonts/bigFont.hpp"

#include "utils/string_helpers.hpp"

#define NOT_100_ID 0
#define IMPOSSIBLE_TIMING_ID 1
#define STORY_MADNESS_ID 2
#define DIED_TO_ID 3
#define HARDER_THAN_ID 4

namespace gd3ds
{

const char *newBestText[] = {
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

const char *diedToText[static_cast<int>(DeathReason::DeathReasonCount)] = {
    "Bro died magically",
    "Bro died to a <i646s3>spike",
    "Bro died to a <#000000><i639s3></>saw",
    "Bro died to a <i696s3>block",
    "Bro died to a <i731s3>slope",
    "Bro fell out of the level",
    "Bro died to the ground",
};

enum class NewBestState {
    ScaleIn,
    Waiting,
    ScaleOut,
};

struct NewBestPopup {
    bool active;

    int progress;
    int textID;
    
    NewBestState state;

    float scale;

    float initialScale;
    float targetScale;
    float timer;
    float duration;
    EaseTypes ease;
};

static NewBestPopup newBestPopup;

void initNewBestPopup(int progress) {
    newBestPopup.active = true;
    newBestPopup.state = NewBestState::ScaleIn;
    newBestPopup.ease = EaseTypes::ElasticOut;

    newBestPopup.textID = randomInt(0, ARRAY_LEN(newBestText) - 1);
    newBestPopup.progress = progress;
    

    // 50% chance on >95% death to say "not 100%"
    if (progress >= 95 && randomInt(0, 100) <= 50) {
        newBestPopup.textID = NOT_100_ID;
    }

    // Cycles impossible timing
    if (progress == 42 && !state.customLevel && currLevelID == 8 && state.player.y > 140) {
        newBestPopup.textID = IMPOSSIBLE_TIMING_ID;
    }

    // If on story madness, reroll to not say the story madness line
    if (newBestPopup.textID == STORY_MADNESS_ID && contains(levelInfo.levelName, "story madness")) {
        do {
            newBestPopup.textID = randomInt(0, ARRAY_LEN(newBestText) - 1);
        } while(newBestPopup.textID == STORY_MADNESS_ID);
    }

    newBestPopup.timer = 0;
    newBestPopup.duration = NEW_BEST_STATE_0_DURATION;
    newBestPopup.initialScale = 0.01;
    newBestPopup.targetScale = NEW_BEST_STATE_0_TARGET_SCALE;
}

void clearNewBestPopup() {
    newBestPopup.active = false;
}

void handleNewBestPopup(float delta) {
    if (newBestPopup.active) {
        switch (newBestPopup.state) {
            case NewBestState::ScaleIn:
                if (newBestPopup.timer > NEW_BEST_STATE_0_DURATION) {
                    newBestPopup.state = NewBestState::Waiting;
                    newBestPopup.ease = EaseTypes::EaseLinear;

                    newBestPopup.timer = 0;
                    newBestPopup.duration = NEW_BEST_STATE_1_DURATION;
                    newBestPopup.initialScale = newBestPopup.scale;
                    newBestPopup.targetScale = newBestPopup.scale;
                }
                break;
            case NewBestState::Waiting:
                if (newBestPopup.timer > NEW_BEST_STATE_1_DURATION) {
                    newBestPopup.state = NewBestState::ScaleOut;
                    newBestPopup.ease = EaseTypes::QuadIn;
                    
                    newBestPopup.timer = 0;
                    newBestPopup.duration = NEW_BEST_STATE_2_DURATION;
                    newBestPopup.initialScale = newBestPopup.scale;
                    newBestPopup.targetScale = NEW_BEST_STATE_2_TARGET_SCALE;
                }
                break;
            case NewBestState::ScaleOut:
                if (newBestPopup.timer > NEW_BEST_STATE_2_DURATION) {
                    newBestPopup.active = false;
                }
                break;
        }
        newBestPopup.scale = easeValue(newBestPopup.ease, newBestPopup.initialScale, newBestPopup.targetScale, newBestPopup.timer, newBestPopup.duration, 1.f);

        newBestPopup.timer += delta;
    }
}

void drawNewBestPopup() {
    if (newBestPopup.active) {
        float scale = newBestPopup.scale;
        char buf[256];

        if (settingsState.doNot) {
            const char *text = newBestText[newBestPopup.textID];

            if (newBestPopup.textID == DIED_TO_ID) {
                text = diedToText[static_cast<int>(state.deathReason)];
            } 

            if (newBestPopup.textID == HARDER_THAN_ID) {
                snprintf(buf, sizeof(buf), text, newBestPopup.progress - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s", text);
            }

            float textScale = scale;

            int width = 390;
            float length = getLongestLineLength(&goldFontFontCharset, NEW_BEST_STATE_0_TARGET_SCALE, buf);

            if (width < length && length > 0) {
                textScale *= (width / length);
            }
            
            drawText(&goldFontFontCharset, &goldFontSheet, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) - (NEW_BEST_SEPARATION * textScale), textScale, textScale, 0.5f, true, buf);
        } else {
            C2D_Sprite text = { 0 };
            C2D_SpriteFromSheet(&text, uiSheet, (NEW_BEST_IMAGE_ID));
            C3D_TexSetFilter(text.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&text, 0.5f, 0.5f);
            C2D_SpriteSetPos(&text, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) - (NEW_BEST_SEPARATION * scale));
            C2D_SpriteSetScale(&text, scale, scale);
            C2D_DrawSprite(&text);
        }

        
        drawText(&bigFontFontCharset, &bigFontSheet, SCREEN_WIDTH_AREA / 2, (SCREEN_HEIGHT_AREA / 2) + (NEW_BEST_SEPARATION * scale), scale, scale, 0.5f, true, "%d%%", newBestPopup.progress);
    }
}

}
