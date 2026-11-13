#include <3ds.h>
#include <citro2d.h>
#include "math_helpers.hpp"
#include "menus/core/ui_element.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_stack.hpp"
#include "utils/precise_input.hpp"

#include "main.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "level_loading.hpp"
#include "triggers.hpp"
#include "easing.hpp"
#include "mp3_player.hpp"
#include "particles/circles.hpp"

#include "menus/core/common_setters.hpp"
#include "menus/components/ui_use_effect.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/gameplay.hpp"

#include "practice.hpp"

#include "save/saving.hpp"

bool gamePaused, inLevelComplete = false;
static float animTime, targetPos, currentPos;
static bool hidePauseMenu = false;

static UILabel *fpsLabel;
static UIProgressBar *progressBar;
static UILabel *percent;
static UILabel *levelName;

static UIWindow *pauseWindow;

#define MAX_PAUSE_MENU_ELEMENTS 16
static UIElement *pauseMenuElements[MAX_PAUSE_MENU_ELEMENTS];
static float pauseMenuBaseY[MAX_PAUSE_MENU_ELEMENTS];
static int pauseMenuCount;
static UIElement *pauseHideBtn;
static float pauseHideBtnY;
static float pauseHideOffset;

static UIProgressBar *normalProgress;
static UILabel *normalProgressVal;
static UIProgressBar *practiceProgress;
static UILabel *practiceProgressVal;

static UISlider *musicSliderBar;
static UISlider *soundSliderBar;

static UIImage *coin1;
static UIImage *coin2;
static UIImage *coin3;

static UIImage *coinsFull[3];
static bool coinsGot[3];
static float coinAnims[3];
static bool coinsCirclesSpawned[3];

static UIImage *coin1Top;
static UIImage *coin2Top;
static UIImage *coin3Top;

static UIScreen *screenTop;
static UIScreen *screen;

int decimal;
float fps;

static void updateProgressBars() {
    LevelData *levelDataSel = &currentLevelEntry->data;

    normalProgress->value = levelDataSel->normalProgress;
    practiceProgress->value = levelDataSel->practiceProgress;

    char normal[32];
    char practice[32];
    snprintf(normal, sizeof(normal), "%d%%", levelDataSel->normalProgress);
    snprintf(practice, sizeof(practice), "%d%%", levelDataSel->practiceProgress);

    uiLabelSetText(normalProgressVal, normal);
    uiLabelSetText(practiceProgressVal, practice);

    if (state.customLevel) {
        bool unrated = levelHasUnverifiedCoins();
        UIImage *tops[3] = { coin1Top, coin2Top, coin3Top };
        bool got[3] = { levelDataSel->coin1, levelDataSel->coin2, levelDataSel->coin3 };

        for (int i = 0; i < 3; i++) {
            uiImageSetImage(tops[i], COIN_BIG_USER_ID, 0);
            uiImageSetTint(tops[i], got[i]
                ? (unrated ? USER_COIN_UNRATED_TINT : C2D_Color32(255, 255, 255, 255))
                : (unrated ? USER_COIN_UNRATED_EMPTY_TINT : C2D_Color32(165, 165, 165, 255)));
        }
    } else {
        uiImageSetImage(coin1Top, (levelDataSel->coin1 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
        uiImageSetImage(coin2Top, (levelDataSel->coin2 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
        uiImageSetImage(coin3Top, (levelDataSel->coin3 ? PAUSE_COIN_FILLED_ID : PAUSE_COIN_UNFILLED_ID), 0);
    }
}

static void resetCoin(LevelData *levelDataSel, int i){
    if((!levelDataSel->coin1 && i == 0)
        || (!levelDataSel->coin2 && i == 1)
        || (!levelDataSel->coin3 && i == 2)) { 
            coinsFull[i]->base.enabled = false;
        } else {
            coinsFull[i]->base.enabled = true;
    }
}

void resetCoins(){
    LevelData *levelDataSel = &currentLevelEntry->data;

    uiUseEffectClear((UIUseEffect *) uiGetElementByTag(screen, "coin_circle"));

    for(int i = 0; i < 3; i++){
        resetCoin(levelDataSel, i);
        coinsGot[i] = false;
        coinAnims[i] = 0.f;
        coinsCirclesSpawned[i] = false;
    }
}

static void applyCoinSlotCount(){
    int count = getLevelCoinCount();
    if (count > 3) count = 3;

    UIImage *slots[3] = { coin1, coin2, coin3 };
    for(int i = 0; i < 3; i++){
        if (slots[i]) slots[i]->base.enabled = (i < count);
        if (i >= count && coinsFull[i]) coinsFull[i]->base.enabled = false;
    }

    if (count == 0) {
        uiRunFuncOnTag(screen, "coin_circle", uiDisableElement);
    }
}

void pauseGame() {
    if (state.endWallAnimPlaying) return;

    updateProgressBars();
    gamePaused = true;
    if (songLoaded || state.practiceMode) pausePlaybackMP3();
    int coinCount = getLevelCoinCount();
    if (coinCount > 3) coinCount = 3;

    UIImage *coinTopSlots[3] = { coin1Top, coin2Top, coin3Top };
    for (int i = 0; i < 3; i++) {
        coinTopSlots[i]->base.x = (i - (coinCount - 1) / 2.f) * 50.f;
        coinTopSlots[i]->base.enabled = (i < coinCount);
    }
    uiRunFuncOnTag(screen, "coin_1", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_2", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_3", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_1_full", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_2_full", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_3_full", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_circle", uiDisableElement);
    uiRunFuncOnTag(screenTop, "pause_menu", uiEnableElement);
    uiRunFuncOnTag(screen, "paused", uiEnableElement);
    uiRunFuncOnTag(screen, "not_paused", uiDisableElement);
}

static void collectPauseMenuElement(UIElement *e) {
    if (e == pauseHideBtn || pauseMenuCount >= MAX_PAUSE_MENU_ELEMENTS) return;

    pauseMenuElements[pauseMenuCount] = e;
    pauseMenuBaseY[pauseMenuCount] = e->y;
    pauseMenuCount++;
}

static void applyPauseMenuOffset(float offset) {
    for (int i = 0; i < pauseMenuCount; i++) pauseMenuElements[i]->y = pauseMenuBaseY[i] + offset;

    if (pauseHideBtn) {
        float y = pauseHideBtnY + offset;

        pauseHideBtn->y = (y > SCREEN_HEIGHT - 20.f) ? SCREEN_HEIGHT - 20.f : y;
    }
}

static void handleHideAnimation(float delta) {
    if (settingsState.swapGameplayScreens) {
        float targetOffset = hidePauseMenu ? SCREEN_HEIGHT : 0.f;

        if (pauseHideOffset == targetOffset) return;

        pauseHideOffset = easeValue(QUAD_IN, pauseHideOffset, targetOffset, animTime, 4.f, 1.f);
        applyPauseMenuOffset(pauseHideOffset);

        animTime += delta;
        return;
    }

    if (hidePauseMenu) {
        targetPos = 0.f - 120.f;
    } else {
        targetPos = 0.f + 120.f;
    }

    if (pauseWindow->base.y == targetPos) return;
    
    currentPos = easeValue(QUAD_IN, pauseWindow->base.y, targetPos, animTime, 4.f, 1.f);
    uiElementSetPosition((UIElement *) pauseWindow, (float)SCREEN_WIDTH / 2.f, currentPos);
    
    animTime += delta;
}

void unpauseGame() {
    gamePaused = false;
    hidePauseMenu = false;
    uiImageSetTint((UIImage *) uiGetElementByTag(screen, "hide_btn"), C2D_Color32(255, 255, 255, 170));

    uiElementSetPosition((UIElement *) pauseWindow, (float)SCREEN_WIDTH / 2.f, (float)SCREEN_HEIGHT / 2.f);

    pauseHideOffset = 0.f;
    applyPauseMenuOffset(0.f);

    if (piEnabled) {
        syncPreciseInput(true);
    }
    if (state.deathTimer <= 0 && (songLoaded || state.practiceMode)) {
        unpausePlaybackMP3();
    }
    uiRunFuncOnTag(screenTop, "coin_1", uiDisableElement);
    uiRunFuncOnTag(screenTop, "coin_2", uiDisableElement);
    uiRunFuncOnTag(screenTop, "coin_3", uiDisableElement);
    uiRunFuncOnTag(screen, "coin_1", uiEnableElement);
    uiRunFuncOnTag(screen, "coin_2", uiEnableElement);
    uiRunFuncOnTag(screen, "coin_3", uiEnableElement);
    for(int i = 0; i < 3; i++){
        resetCoin(&currentLevelEntry->data, i);
    }
    uiRunFuncOnTag(screen, "coin_circle", uiEnableElement);
    applyCoinSlotCount();
    uiRunFuncOnTag(screenTop, "pause_menu", uiDisableElement);
    uiRunFuncOnTag(screen, "paused", uiDisableElement);
    uiRunFuncOnTag(screen, "not_paused", uiEnableElement);
    
    if (!state.practiceMode || settingsState.hidePracticeButtons) {
        uiRunFuncOnTag(screen, "practice_buttons", uiDisableElement);
    }
}

static void exitLevel() {
    if (gamePaused){
        exitingLevel = true;
        uiStackPushGameState(STATE_MENU);
    }
}

static void restartLevel() {
    initVariables();
    reloadLevel();
    if (state.practiceMode) {
        if (settingsState.autoCheckpoints && playerGamemodeIsFlying(&state.deathPlayer) && pseudoCheckpointExists) {
            deleteLastCheckpoint();
            pseudoCheckpointExists = false;
        }
        
        if (getCheckpointCount() > 0) {
            restoreCheckpoint();
        } else if (settingsState.practiceMusicSync) {
            seekMP3(levelInfo.songOffset);
        }
    } else if (songLoaded) seekMP3(levelInfo.songOffset);

    resetCoins();

    unpauseGame();
}

static void hideMenu() {
    hidePauseMenu = !hidePauseMenu;
    uiImageSetTint((UIImage *) uiGetElementByTag(screen, "hide_btn"), C2D_Color32(255, 255, 255, hidePauseMenu ? 255 : 170));

    animTime = 0.f;
}

static void actionPause(UIElement *e, const UIPropertyList *args) { 
    pauseGame();
}

static void actionUnpause(UIElement *e, const UIPropertyList *args) {
    unpauseGame();
}

static void actionExit(UIElement *e, const UIPropertyList *args) {
    exitLevel();
}

static void actionRestart(UIElement *e, const UIPropertyList *args) {
    restartLevel();
}

static void actionPracticeMode(UIElement *e, const UIPropertyList *args) {
    if (!state.practiceMode) {
        startPracticeMode();
    } else {
        exitPracticeMode();
    }

    actionUnpause(e, NULL);
}

static void actionAddCheckpoint(UIElement *e, const UIPropertyList *args) {
    newCheckpoint();
}
static void actionRemoveCheckpoint(UIElement *e, const UIPropertyList *args) {
    deleteLastCheckpoint();
}
static void actionHideMenu(UIElement *e, const UIPropertyList *args) {
    hideMenu();
}

static UIActionDef gameplayActions[] = {
    {"pause", actionPause },
    {"unpause", actionUnpause },
    {"exit", actionExit },
    {"restart", actionRestart },
    {"practice", actionPracticeMode },
    {"add_check", actionAddCheckpoint },
    {"remove_check", actionRemoveCheckpoint },
    {"hide_menu", actionHideMenu}
};

void gameplayInitTop(UIScreen *s){
    screenTop = s;

    fpsLabel = (UILabel *) uiGetElementByTag(screenTop, "fps");
    progressBar = (UIProgressBar *) uiGetElementByTag(screenTop, "progressalert");
    percent = (UILabel *) uiGetElementByTag(screenTop, "percent");

    pauseWindow = (UIWindow *) uiGetElementByTag(s, "bgwindow");
    levelName = (UILabel *) uiGetElementByTag(screenTop, "level_title");

    Color color = getWhiteIfBlack(p1Color);

    uiProgressBarSetTint(progressBar, C2D_Color32(color.r, color.g, color.b, 255));

    uiWindowSetTint(pauseWindow, C2D_Color32(0, 0, 0, 150));

    uiLabelSetText(levelName, levelInfo.levelName);

    normalProgress = (UIProgressBar *) uiGetElementByTag(screenTop, "normalprogress");
    normalProgressVal = (UILabel *) uiGetElementByTag(screenTop, "normalprogressvalue");
    practiceProgress = (UIProgressBar *) uiGetElementByTag(screenTop, "practiceprogress");
    practiceProgressVal = (UILabel *) uiGetElementByTag(screenTop, "practiceprogressvalue");
    
    uiProgressBarSetTint(normalProgress, C2D_Color32(0, 255, 0, 255));
    uiProgressBarSetTint(practiceProgress, C2D_Color32(0, 255, 255, 255));

    // Hide pause menu
    uiRunFuncOnTag(screenTop, "coin_1", uiDisableElement);
    uiRunFuncOnTag(screenTop, "coin_2", uiDisableElement);
    uiRunFuncOnTag(screenTop, "coin_3", uiDisableElement);
    uiRunFuncOnTag(screenTop, "pause_menu", uiDisableElement);

    coin1Top = (UIImage *) uiGetElementByTag(screenTop, "coin_1");
    coin2Top = (UIImage *) uiGetElementByTag(screenTop, "coin_2");
    coin3Top = (UIImage *) uiGetElementByTag(screenTop, "coin_3");
}

void gameplayInit(UIScreen *s) {
    screen = s;
    hidePauseMenu = false;

    levelUnratedOnlineRefresh();
    
    coin1 = (UIImage *) uiGetElementByTag(s, "coin_1");
    coin2 = (UIImage *) uiGetElementByTag(s, "coin_2");
    coin3 = (UIImage *) uiGetElementByTag(s, "coin_3");
    
    coinsFull[0] = (UIImage *) uiGetElementByTag(s, "coin_1_full");
    coinsFull[1] = (UIImage *) uiGetElementByTag(s, "coin_2_full");
    coinsFull[2] = (UIImage *) uiGetElementByTag(s, "coin_3_full");
    
    if (state.customLevel) {
        bool unrated = levelHasUnverifiedCoins();
        UIImage *coinSlots[3] = { coin1, coin2, coin3 };
        for (int i = 0; i < 3; i++) {
            uiImageSetImage(coinSlots[i], COIN_BIG_USER_ID, 0);
            uiElementSetScale((UIElement *) coinSlots[i], COIN_BIG_USER_SCALE);
            uiImageSetTint(coinSlots[i], unrated ? USER_COIN_UNRATED_EMPTY_TINT : C2D_Color32(165, 165, 165, 255));
            uiImageSetImage(coinsFull[i], COIN_BIG_USER_ID, 0);
            uiElementSetScale((UIElement *) coinsFull[i], COIN_BIG_USER_SCALE);
            if (unrated) uiImageSetTint(coinsFull[i], USER_COIN_UNRATED_TINT);
        }
    }

    uiWindowSetTint((UIWindow *) uiGetElementByTag(s, "bgwindow"), C2D_Color32(0, 0, 0, 150));
    uiImageSetTint((UIImage *) uiGetElementByTag(s, "hide_btn"), C2D_Color32(255, 255, 255, 150));

    pauseHideBtn = (UIElement *) uiGetElementByTag(s, "hide_btn");
    pauseHideBtnY = pauseHideBtn ? pauseHideBtn->y : 0.f;
    pauseMenuCount = 0;
    pauseHideOffset = 0.f;
    uiRunFuncOnTag(s, "paused", collectPauseMenuElement);

    uiRunFuncOnTag(s, "paused", uiDisableElement);
    uiRunFuncOnTag(s, "practice_buttons", uiDisableElement);
    
    resetCoins();

    applyCoinSlotCount();

    musicSliderBar = (UISlider*) uiGetElementByTag(s, "music_slider");
    soundSliderBar = (UISlider*) uiGetElementByTag(s, "sound_slider");

    if (musicSliderBar) musicSliderBar->value = musicVolume;
    if (soundSliderBar) soundSliderBar->value = soundVolume;
}

void gameplayUpdateTop(UIScreen *s, UIInput *touch) { 
    decimal = 0;
    float cpuFPS = 1000.f / (delta * 1000);
    float gpuFPS = (16.667f / C3D_GetDrawingTime()) * 60;
    // float gpu_fps = 351.f;
    fps = settingsState.vsync ? cpuFPS : MIN(cpuFPS, gpuFPS);
    if (settingsState.decimalPercent) decimal = 2;
    if (settingsState.ultraDecimalPercent) decimal = MAX_DECIMAL_PERCENT;

    bool isGolden = (int)state.levelProgress > state.currentData.maxNormal;
    if (settingsState.goldenBest && isGolden && !state.practiceMode) {
        percent->font = 2;
        percent->base.y = 7.f;
        percent->base.scaleX = 0.57f;
        percent->base.scaleY = 0.57f;
    } else {
        percent->font = 0;
        percent->base.y = 8.f;
        percent->base.scaleX = 0.45f;
        percent->base.scaleY = 0.45f;
    };
    // update fps counter but only when game isnt paused cuz otherwise it shows like a bajillion frame per sec ond
    if(!gamePaused) snprintf(fpsLabel->text, 32, "<#FFFFFF55>FPS: %.*f</>", 0, fps);

    progressBar->value = state.levelProgress;
    snprintf(percent->text, 32, "%.*f%%", decimal, state.levelProgress);

    uiRunFuncOnTag(s, "progressalert", uiDisableElement);
    uiRunFuncOnTag(s, "percent", uiDisableElement);
    uiRunFuncOnTag(s, "fps", uiDisableElement);
    percent->base.x = 200.f;
    percent->alignment = 0.5;

    if (settingsState.showProgressBar) {
        uiRunFuncOnTag(s, "progressalert", uiEnableElement);
        percent->base.x = 282.f;
        percent->alignment = 0;
    }

    if (settingsState.showProgressPercent) {
        uiRunFuncOnTag(s, "percent", uiEnableElement);
    }

    if (settingsState.showFpsCounter) {
        uiRunFuncOnTag(s, "fps", uiEnableElement);
    }

    handleHideAnimation(delta);
}

void gameplayUpdate(UIScreen *s, UIInput *touch) {
    LevelData *levelDataSel = &currentLevelEntry->data;

    coinsGot[0] = state.currentData.coin1 && !levelDataSel->coin1;
    coinsGot[1] = state.currentData.coin2 && !levelDataSel->coin2;
    coinsGot[2] = state.currentData.coin3 && !levelDataSel->coin3;

    for(int i = 0; i < 3; i++){
        if(coinsGot[i] && !gamePaused){
            float scale = easeValue(BOUNCE_OUT, 2.f, state.customLevel ? COIN_BIG_USER_SCALE : 0.65f, coinAnims[i], 0.35f, 1.f);
            float opacity = easeValue(EASE_LINEAR, 0.f, 1.f, coinAnims[i], 0.1f, 1.f);

            coinsFull[i]->base.enabled = true;
            uiElementSetScale((UIElement *) coinsFull[i], scale);
            uiImageSetTint(coinsFull[i], levelHasUnverifiedCoins()
                ? C2D_Color32(USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B, (u8)(opacity * 255.f))
                : C2D_Color32f(1, 1, 1, opacity));

            if(coinAnims[i] >= 0.05f){
                if(!coinsCirclesSpawned[i]){
                    uiSetUseEffectCol(
                        uiAddUseEffect(
                            (UIUseEffect *) uiGetElementByTag(s, "coin_circle"), 
                        coinsFull[i]->base.x, coinsFull[i]->base.y, &endWallFireworkCircle),
                    1.f,
                    levelHasUnverifiedCoins() ? USER_COIN_UNRATED_G / 255.f : (state.customLevel ? 1.f : 0.75f),
                    levelHasUnverifiedCoins() ? USER_COIN_UNRATED_B / 255.f : (state.customLevel ? 1.f : 0.f));
                    
                    coinsCirclesSpawned[i] = true;
                }
            }

            coinAnims[i] += delta;
        }
    }

    if (gamePaused) {
        if (musicSliderBar) musicVolume = musicSliderBar->value;
        if (soundSliderBar) soundVolume = soundSliderBar->value;

        applyVolumeSettings();
    }

    //level end buttons retraction animation
    float cutsceneTimer = state.player.cutsceneTimer;
    float completeYOffset = easeValue(EASE_IN, 20.f, -20.f, cutsceneTimer, 0.3f, 1.3f);
    float completeYOffsetPractice = easeValue(EASE_IN, 200.f, 270.f, cutsceneTimer, 0.3f, 1.8f);

    bool swapCoins = settingsState.swapGameplayScreens && !inLevelComplete;
    float coinX = swapCoins ? 20.f - SWAP_SIDE_OFFSET_X : 20.f;
    float coinY = swapCoins ? SCREEN_HEIGHT - completeYOffset : completeYOffset;
    UIImage *coinSlots[3] = { coin1, coin2, coin3 };

    for (int i = 0; i < 3; i++) {
        coinSlots[i]->base.x = coinX + 35.f * i;
        coinSlots[i]->base.y = coinY;
        coinsFull[i]->base.x = coinX + 35.f * i;
        coinsFull[i]->base.y = coinY;
    }

    UIElement *pauseBtn = uiGetElementByTag(s, "pause_btn");
    pauseBtn->y = completeYOffset;
    
    UIElement *addCheckpoint = uiGetElementByTag(s, "add_checkpoint");
    UIElement *removeCheckpoint = uiGetElementByTag(s, "remove_checkpoint");
    addCheckpoint->y = completeYOffsetPractice;
    removeCheckpoint->y = completeYOffsetPractice;

    if (state.practiceMode) {
        uiButtonSetImage((UIButton *) uiGetElementByTag(s, "practice_mode"), 124, 0);
    } else {
        uiRunFuncOnTag(s, "practice_buttons", uiDisableElement);
        uiButtonSetImage((UIButton *) uiGetElementByTag(s, "practice_mode"), 146, 0);
    }
}

extern const UIScreenDefPair gameplayDef = {
    .name = "gameplay",
    .top = {
        .path = "romfs:/menus/gameplay_top.txt",
        .init = gameplayInitTop,
        .update = gameplayUpdateTop
    },
    .btm = {
        .path = "romfs:/menus/gameplay.txt",
        .init = gameplayInit,
        .update = gameplayUpdate,
        .actionList = {
            .actions = gameplayActions,
            .actionCount = ARRAY_LEN(gameplayActions)
        }
    }
};