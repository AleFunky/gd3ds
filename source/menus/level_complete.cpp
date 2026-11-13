#include <3ds.h>
#include <citro2d.h>

#include "level_complete.hpp"

#include "menus/core/common_setters.hpp"

#include "math_helpers.hpp"
#include "main.hpp"
#include "easing.hpp"
#include "mp3_player.hpp"
#include "sounds.hpp"
#include "state.hpp"
#include "level_loading.hpp"
#include "particles/circles.hpp"

#include "menus/components/ui_darken.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_particle.hpp"
#include "menus/components/ui_use_effect.hpp"
#include "menus/settings_hub/settings.hpp"

#include "utils/string_helpers.hpp"

#include "save/saving.hpp"
#include "wav_player.hpp"

#define ANIM_DURATION 1.f
#define RESTART_ANIM_DURATION 0.5f

#define SECRET_COIN_UI2_ID 401

#define COIN_UNCOLLECTED_CUSTOM_ID 64
#define COIN_UNCOLLECTED_CUSTOM_SHEET 4
#define COIN_UNCOLLECTED_CUSTOM_SCALE 0.968f

namespace gd3ds
{

static bool yesExit = false;
static bool restart = false;
static bool init = false;

static bool animatingDown = false;
static bool animatingReward = false;
static bool animatingUp = false;

static float animTime = 0;

static float upYStart = 0;
static float windowYPos = 0;

static int rewardAnimPhase = 0;
static float rewardAnimTime = 0.f;
static bool playedRewardSFX;

static bool showStars;

static UIScreen screenTop;
static UIScreen screen = {
    .isBottom = true
};

static UILabel *attemptText;
static UILabel *jumpsText;
static UILabel *timeText;

static UILabel *completionText;

static UIImage *coinsFull[3];
static UIImage *coinsBase[3];
static UIParticle *particles[4];

const char *practiceCompletionText = "Well done... Now try to complete it\nwithout any checkpoints!";

const char *doNotCompletionTexts[] = {
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
    "we really doing anything now",
    "+100 dollars",
    "360 FPS detected",
    "A swing and a hit!",
    "A swing and a home run!",
    "Absolute Geometry Dash",
    "After all this time...",
    "Astounded. Flabbergasted really.",
    "Auto-Safe Mode Disabled!",
    "Back to back?",
    "Believe in magic",
    "But is it possible with accurate hitboxes?",
    "Calm down!",
    "CBF Detected, loser! Click Between Frames\nis illegitimate and will not be allowed\nfor use in GD3DS. Please disable the mod to\ncontinue playing.",
    "Cheats Not Detected! Verification not voided.",
    "Cheats used: Skill",
    "Clap Clap Clap",
    "Congragurlationals!",
    "dear god they did it...",
    "Do it again, I wasn't paying attention",
    "engineer gaming",
    "epic sauce",
    "Error 404: Completion not found",
    "exit(0)",
    "famidash ReTraY 100% complete",
    "First try!!",
    "GAMING",
    "gg freakin ez",
    "go outside",
    "Good enough.",
    "good j*b ig",
    "Good job, now beat Grief.",
    "good yob",
    "Goop job!",
    "Have a cookie",
    "hi you did it bye",
    "HOLY SHIT YOU JUST VERIFIED... oh wait.",
    "horse",
    "How",
    "I could do this in half the attempts",
    "If it's not a demon I don't care",
    "ITS BLUE ITS BLUE",
    "John GD3DS congratulates you",
    "Like Doggie Jr.!",
    "Nah this is pay to win",
    "Nice! Now try to complete it with 3D on!",
    "Now beat it while playing piano",
    "Now do it again",
    "Now do it blindfolded",
    "Now you can beat Grief!",
    "One Level, Two Level, Beat Level, New Level",
    "Please touch some grass.",
    "Possible top 1 - Said no one ever",
    "Project zoink",
    "Put me in the screenshot",
    "RubRub is always watching",
    "Scream louder!",
    "slow clap",
    "So what now?",
    "Son im crine",
    "Stop screaming!",
    "That doesn't count",
    "That's just dumb luck",
    "That's not Aeternus",
    "The End.",
    "The servers are currently experiencing a\ntemporary period of heavy load,\nplease try again tomorrow.",
    "They cleared it for an amazing reason!",
    "Time to rest...",
    "To celebrate, they became purple",
    "Took you long enough...",
    "Unc still got it",
    "W bro",
    "W larp",
    "W speed (heart emoji)",
    "Was it worth it?",
    "WE beat it, yes BOTH of us.",
    "Well done! Now try to complete it withou- oh.",
    "Well played!",
    "Why didn't anyone wake me up?",
    "why u hacker",
    "Winner winner chicken dinner!!!",
    "Yea whatever, that wasn't even difficult...",
    "YIPPEE!!!!",
    "You are peak!",
    "You are the Geometry Dash",
    "You cannot beat a practice in normal mode.",
    "you di dit!",
    "You took THAT many attempts? Pathetic.",
    "You verified The Golden, let's go",
    "YOU WIN!",
    "You've just beaten Stereo Madness",
    "Your dids it!",
    "your winner!"
};

const char *completionTexts[] = {
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


static void exitLevelComplete(UIElement* e, const UIPropertyList *args) {
    if (!animatingUp) {
        playSfx(&quitSound, 1);
        yesExit = true;
        animatingUp = true;
        animatingDown = false;
        animTime = 0;
    }
}

static void restartLevel(UIElement* e, const UIPropertyList *args) {
    if (!animatingUp) {
        playSfx(&playSound, 1);
        animatingUp = true;
        animatingDown = false;
        animTime = 0;
        pausePlaybackMP3();
    }
}

static void scaleBottomButtonsAnim(UIElement* e){
    UIButton *button = (UIButton *) e;

    float fadeValueScale = easeValue(ELASTIC_OUT, 0, 1, animTime, ANIM_DURATION, 1.f);
    
    uiElementSetScale((UIElement *) button, fadeValueScale);
}

static UIActionDef actions[] = {
    { "restart", restartLevel },
    { "exit", exitLevelComplete },
};

// This runs the initial animation of the menu coming off screen
static void runStartAnimation(float delta) {
    float fadeValue = easeValue(BOUNCE_OUT, 0, 240, animTime, ANIM_DURATION, 1.f);\
    windowYPos = -120 + fadeValue;
    upYStart = fadeValue;

    UIDarken *darken = (UIDarken *) uiGetElementByTag(&screen, "endDarken");
    darken->base.opacity = clampf(animTime, 0.f, 0.6f);
    uiDarkenResetOpacity(darken);

    uiRunFuncOnTag(&screen, "bottomWindow", scaleBottomButtonsAnim);
    
    uiSetPosOnTag(&screen, (float)SCREEN_BOT_WIDTH / 2, windowYPos, "window");
    uiSetPosOnTag(&screenTop, (float)SCREEN_WIDTH / 2, windowYPos, "window");

    if(animTime > 0.6f){
        animatingReward = true;
    }

    // Animation end
    if (animTime >= ANIM_DURATION) {
        animatingDown = false;
        animTime = 0;
    }
    animTime += delta;
}

static void spawnRewardFirework(UIImage* e){
    float x = e->base.x;
    float y = e->base.y;

    UIParticle *particle = particles[rewardAnimPhase];
    uiParticleEmit(particle, 60);

    uiSetUseEffectCol(
        uiAddUseEffect(
            (UIUseEffect *) uiGetElementByTag(&screenTop, "rewardCircle"),
        x, y, &deathEffect),
    1.f,
    levelHasUnverifiedCoins() ? USER_COIN_UNRATED_G / 255.f : (state.customLevel ? 1.f : 0.75f),
    levelHasUnverifiedCoins() ? USER_COIN_UNRATED_B / 255.f : (state.customLevel ? 1.f : 0.f));
}

// This plays the animation of the coins popping into place and the stars
static void runRewardsAnimation(float delta){
    if(rewardAnimPhase > 3){
        animatingReward = false;
        return;
    }

    //move particles and use effects to align with coin when still animating the window coming onscreen
    if(animatingDown || animatingUp){
        uiParticleUpdatePos(particles[rewardAnimPhase]);
        uiUseEffectUpdatePos((UIUseEffect *) uiGetElementByTag(&screenTop, "rewardCircle"));
    }

    LevelData *levelDataSel = &currentLevelEntry->data;
    UIImage *rewardCenter = NULL;

    float scaleValue = easeValue(BOUNCE_OUT, 3.f, 1.f, rewardAnimTime, 0.35f, 1.f);
    float opacityValue = easeValue(EASE_LINEAR, 0.f, 1.f, rewardAnimTime, 0.1f, 1.f);
    //coins
    if(rewardAnimPhase < 3){
        //Skip uncollected/already collected coins
        if((rewardAnimPhase == 0 && (!state.currentData.coin1 || levelDataSel->coin1))
        || (rewardAnimPhase == 1 && (!state.currentData.coin2 || levelDataSel->coin2))
        || (rewardAnimPhase == 2 && (!state.currentData.coin3 || levelDataSel->coin3))){
            rewardAnimPhase++;
            rewardAnimTime = 0;
            return;
        }

        UIImage* coin = coinsFull[rewardAnimPhase];
        rewardCenter = coin;
        
        coin->base.enabled = true;

        uiElementSetScale((UIElement *) coin, scaleValue * 0.88f);

        uiImageSetTint(coin, levelHasUnverifiedCoins()
            ? C2D_Color32(USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B, (u8)(opacityValue * 255.f))
            : C2D_Color32f(1, 1, 1, opacityValue));
    } else{
        if(showStars) {
            UIImage* star = (UIImage *) uiGetElementByTag(&screenTop, "star");
            UILabel* starText = (UILabel *) uiGetElementByTag(&screenTop, "startext");
            rewardCenter = star;

            star->base.enabled = true;
            starText->base.enabled = true;

            //scale around y = 128 (relative to window)
            int center = upYStart - 128;
            star->base.y = ((scaleValue * 0.7f) * (upYStart - 127 - center) + center);
            starText->base.y = ((scaleValue * 0.55f) * (upYStart - 83 - center) + center);

            uiElementSetScale((UIElement *) star, scaleValue * 0.7f);
            uiElementSetScaleXy((UIElement *) starText, scaleValue * 0.55f, scaleValue * 0.55f);

            uiImageSetTint(star, C2D_Color32f(1, 1, 1, opacityValue));
            starText->base.opacity = opacityValue;
        } else {
            return;
        }
    }

    if(rewardAnimTime >= 0.1f){
        if(!playedRewardSFX){
            playSfx(&coinSound, 1);
            spawnRewardFirework(rewardCenter);
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
static void runEndAnimation(float delta) {
    float fadeValue = easeValue(QUAD_IN, upYStart, 0, animTime, RESTART_ANIM_DURATION, 1.f);
    windowYPos = -120 + fadeValue;

    uiSetPosOnTag(&screen, (float)SCREEN_BOT_WIDTH / 2.f, windowYPos, "window");
    uiSetPosOnTag(&screenTop, (float)SCREEN_WIDTH / 2.f, windowYPos, "window");
    
    // Animation end
    if (animTime >= RESTART_ANIM_DURATION) {
        animatingUp = false;
        restart = true;
        animTime = 0;
    }
    animTime += delta;
}

#define COMPLETION_TEXT_MAX_WIDTH 250.f

static const UIScreenDefinition levelCompleteDef = {
    .actionList = {
        .actions = actions,
        .actionCount = ARRAY_LEN(actions)
    }
};

static void enableSavedCoins(LevelData *levelDataSel) {
    for (int i = 0; i < 3; i++) {
        bool alreadyCollectedCoin = false;
        if ((i == 0 && levelDataSel->coin1)
        || (i == 1 && levelDataSel->coin2)
        || (i == 2 && levelDataSel->coin3)) {
            alreadyCollectedCoin = true;
        }

        UIImage *coin = coinsFull[i];

        if (alreadyCollectedCoin) {
            coin->base.enabled = true;
            coin->base.opacity = 1.f;

            uiElementSetScale((UIElement *) coin, 0.88f);
        }
    }
}

void levelCompleteInit() {
    init = true;
    inLevelComplete = true;

    levelUnratedOnlineRefresh();
    
    uiUnloadScreen(&screen);
    uiUnloadScreen(&screenTop);
    
    screen.def = &levelCompleteDef;

    uiLoadScreenOld(&screenTop, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete_top.txt");
    uiLoadScreenOld(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_complete.txt");

    state.currentData.timeEnd = svcGetSystemTick() / (CPU_TICKS_PER_MSEC * 1000);

    char attempts[64];
    char jumps[64];
    char time[64];

    snprintf(attempts, sizeof(attempts), "Attempts: %d", state.currentData.attempts);
    snprintf(jumps, sizeof(jumps), "Jumps: %d", state.currentData.jumps);

    float timer = state.currentData.timeEnd - state.currentData.timeStart;

    int hours   = (int)(timer) / (60 * 60);
    int minutes = (int)(timer / 60) % 60;
    int seconds = (int)(timer) % 60;

    if (hours) {
        snprintf(time, sizeof(time), "Time: %d:%02d:%02d", hours, minutes, seconds);
    } else {
        snprintf(time, sizeof(time), "Time: %02d:%02d", minutes, seconds);
    }

    attemptText = (UILabel *) uiGetElementByTag(&screenTop, "attempts");
    if (attemptText) uiLabelSetText(attemptText, attempts);

    jumpsText = (UILabel *) uiGetElementByTag(&screenTop, "jumps");
    if (jumpsText) uiLabelSetText(jumpsText, jumps);

    timeText = (UILabel *) uiGetElementByTag(&screenTop, "time");
    if (timeText) uiLabelSetText(timeText, time);

    yesExit = false;
    restart = false;
    animatingDown = true;
    animatingReward = false;
    animatingUp = false;
    animTime = 0;
    windowYPos = 0;

    rewardAnimPhase = 0;
    rewardAnimTime = 0.f;
    playedRewardSFX = false;

    coinsFull[0] = (UIImage *) uiGetElementByTag(&screenTop, "coin1full");
    coinsFull[1] = (UIImage *) uiGetElementByTag(&screenTop, "coin2full");
    coinsFull[2] = (UIImage *) uiGetElementByTag(&screenTop, "coin3full");

    coinsBase[0] = (UIImage *) uiGetElementByTag(&screenTop, "coin1");
    coinsBase[1] = (UIImage *) uiGetElementByTag(&screenTop, "coin2");
    coinsBase[2] = (UIImage *) uiGetElementByTag(&screenTop, "coin3");

    int coinDisplayCount = getLevelCoinCount();
    if (coinDisplayCount > 3) coinDisplayCount = 3;

    if (state.customLevel) {
        for (int i = 0; i < 3; i++) {
            uiImageSetImage(coinsFull[i], SECRET_COIN_UI2_ID, 0);
            if (levelHasUnverifiedCoins()) uiImageSetTint(coinsFull[i], USER_COIN_UNRATED_TINT);
        }
    }

    uiRunFuncOnTag(&screenTop, "coinfull", uiDisableElement);

    particles[0] = (UIParticle *) uiGetElementByTag(&screenTop, "coinParticle1");
    particles[1] = (UIParticle *) uiGetElementByTag(&screenTop, "coinParticle2");
    particles[2] = (UIParticle *) uiGetElementByTag(&screenTop, "coinParticle3");
    particles[3] = (UIParticle *) uiGetElementByTag(&screenTop, "starParticle");

    if (levelHasUnverifiedCoins()) {
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
    completionText = (UILabel *) uiGetElementByTag(&screenTop, "funnytext");
    
    LevelData *levelDataSel = &currentLevelEntry->data;

    UILabel *starText = (UILabel *) uiGetElementByTag(&screenTop, "startext");

    char starCount[4];
    int stars = levelDataSel->stars;
    snprintf(starCount, sizeof(starCount), "+%d", stars);
    uiLabelSetText(starText, starCount);

    uiDisableElement(uiGetElementByTag(&screenTop, "star"));
    uiDisableElement((UIElement *) starText);

    showStars = stars > 0 && levelDataSel->normalProgress < 100;

    if(state.customLevel == true || state.practiceMode || cheated) {
        if (state.customLevel && !state.practiceMode && !cheated) {
            for (int k = 0; k < 3; k++) {
                bool saved = (k == 0 && levelDataSel->coin1)
                          || (k == 1 && levelDataSel->coin2)
                          || (k == 2 && levelDataSel->coin3);
                if (k < coinDisplayCount && !saved) {
                    uiImageSetImage(coinsBase[k], COIN_UNCOLLECTED_CUSTOM_ID, COIN_UNCOLLECTED_CUSTOM_SHEET);
                    uiElementSetScale((UIElement *) coinsBase[k], COIN_UNCOLLECTED_CUSTOM_SCALE);
                    if (levelHasUnverifiedCoins()) uiImageSetTint(coinsBase[k], USER_COIN_UNRATED_TINT);
                    coinsBase[k]->base.enabled = true;
                } else {
                    coinsBase[k]->base.enabled = false;
                }
            }
            enableSavedCoins(levelDataSel);
        } else {
            uiRunFuncOnTag(&screenTop, "coin1", uiDisableElement);
            uiRunFuncOnTag(&screenTop, "coin2", uiDisableElement);
            uiRunFuncOnTag(&screenTop, "coin3", uiDisableElement);
        }
    
        int startIndex = 0;

        // Skip the "Not 1 attempt" line
        if (settingsState.doNot) {
            if (state.currentData.attempts == 1) startIndex++;
        }

        int textIndex = randomInt(startIndex, (settingsState.doNot ? ARRAY_LEN(doNotCompletionTexts) : ARRAY_LEN(completionTexts))- 1);

        if (settingsState.doNot) {
            // If exactly 2 attempts, say "Not 1 attempt"
            if (state.currentData.attempts == 2) textIndex = 0;

            // If on story madness, reroll to not say the story madness line
            if (textIndex == 1 && contains(levelInfo.levelName, "story madness")) {
                do {
                    textIndex = randomInt(startIndex, (settingsState.doNot ? ARRAY_LEN(doNotCompletionTexts) : ARRAY_LEN(completionTexts))- 1);
                } while(textIndex == 1);
            }
        }

        const char *text = (settingsState.doNot ? doNotCompletionTexts[textIndex] : completionTexts[textIndex]);

        char tmp[512];

        if (state.practiceMode) {
            text = practiceCompletionText;
        }
        
        if (cheated) {
            char enabledCheats[256] = "";
            size_t len = 0;
            
            for (int i = 0; i < CHEAT_COUNT; i++) {
                if (cheatsUsed[i]) {
                    if (len > 0) {
                        len += snprintf(enabledCheats + len,
                                        sizeof(enabledCheats) - len,
                                        ", ");
                    }

                    len += snprintf(enabledCheats + len,
                                    sizeof(enabledCheats) - len,
                                    "%s",
                                    cheatNames[i]);
                }
            }
            snprintf(tmp, sizeof(tmp), "Safe mode - %s", enabledCheats);
            text = tmp;
        }
        
        uiLabelSetText(completionText, text);
    } else {
        uiRunFuncOnTag(&screenTop, "funnytext", uiDisableElement);

        enableSavedCoins(levelDataSel);
    }

    for (int k = coinDisplayCount; k < 3; k++) {
        if (coinsBase[k]) coinsBase[k]->base.enabled = false;
    }

    for (int k = 0; k < 3; k++) {
        float coinX = (SCREEN_WIDTH / 2.f) + (k - (coinDisplayCount - 1) / 2.f) * 50.f;
        if (coinsBase[k]) uiElementSetPosition((UIElement *) coinsBase[k], coinX, coinsBase[k]->base.y);
        if (coinsFull[k]) uiElementSetPosition((UIElement *) coinsFull[k], coinX, coinsFull[k]->base.y);
        if (particles[k]) {
            particles[k]->base.x = coinX;
            uiParticleUpdatePos(particles[k]);
        }
    }

    if (getLevelCoinCount() > 0 && !state.practiceMode && !cheated) {
        uiRunFuncOnTag(&screenTop, "funnytext", uiDisableElement);
    }

    if (state.practiceMode) {
        uiRunFuncOnTag(&screenTop, "levelcomplete", uiDisableElement);
        if (settingsState.doNot) {
            UIImage *levelCompleteText = (UIImage *) uiGetElementByTag(&screenTop, "practicecomplete");
            if (levelCompleteText) {
                uiElementSetScaleX((UIElement *) levelCompleteText, levelCompleteText->base.scaleX * -1);
            }
        }
    } else {
        uiRunFuncOnTag(&screenTop, "practicecomplete", uiDisableElement);
        if (settingsState.doNot) {
            UIImage *levelCompleteText = (UIImage *) uiGetElementByTag(&screenTop, "levelcomplete");
            if (levelCompleteText) {
                uiElementSetScaleX((UIElement *) levelCompleteText, levelCompleteText->base.scaleX * -1);
            }
        }
    }

    uiGetElementByTag(&screen, "endDarken")->opacity = 0.f;
}

int levelCompleteLoop(UIInput *touch) {
    if (!init) return 0;

    if (animatingDown) runStartAnimation(delta);
    if (animatingReward && !state.practiceMode && !cheated) runRewardsAnimation(delta);
    if (animatingUp) runEndAnimation(delta);

    if (yesExit) {
        return 1;
    }

    if (restart) {
        return 2;
    }

    uiScreenUpdate(&screen, touch);
    uiScreenUpdate(&screenTop, touch);

    return 0;
}

void levelCompleteDestroy() {
    init = false;
}

void drawLevelComplete() {
    if (init) {
        uiScreenDraw(&screen);
    }
}
void drawLevelCompleteTop() {
    if (init) {
        uiScreenDraw(&screenTop);
    }
}

}