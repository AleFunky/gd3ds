#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "level_loading.hpp"
#include "menus/core/common_setters.hpp"

#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "fonts/bigFont.hpp"
#include "main.hpp"
#include "easing.hpp"
#include "triggers.hpp"
#include "mp3_player.hpp"
#include "graphics.hpp"
#include "level_select.hpp"

#include "level/main_levels.hpp"
#include "save/saving.hpp"

#include "menus/gameplay.hpp"

#include "state.hpp"

namespace gd3ds
{

const char *errorStrings[LOAD_ERROR_COUNT - 1] = {
    "Invalid gmd.",
    "Invalid level data.",
    "Level string missing sections.",
    "Out of memory.",
    "Couldn't parse objects.",
    "Invalid Base64."
};

static bool startLevel = false;

int currLevelID = 0;

s8 scrollDir = 0;

float animDuration = 1.f;
float animTime = 0;

static bool dragging;
static int dragStartX;
static int lastTouchX;
static int dragDistance;
static int dragDir;
//Prevents cards from overlapping 
//hopefully
static bool cardCorrection;

static UIImage *bgGradient = NULL;
static UIImage *bg_gradient_top = NULL;
static UIWindowButton *level_card_window = NULL;
static UILabel *level_card_title_top = NULL;

static UILabel *level_card_title = NULL;
static UILabel *level_card_stars = NULL;
static UIImage *level_card_face = NULL;

static UIWindowButton *level_card_2_window = NULL;

static UILabel *level_card_2_title = NULL;
static UILabel *level_card_2_stars = NULL;

static UIProgressBar *level_card_normal_progress = NULL;
static UILabel *level_card_normal_progress_val = NULL;
static UIProgressBar *level_card_2_normal_progress = NULL;
static UILabel *level_card_2_normal_progress_val = NULL;

static UIProgressBar *level_card_practice_progress = NULL;
static UILabel *level_card_practice_progress_val = NULL;
static UIProgressBar *level_card_2_practice_progress = NULL;
static UILabel *level_card_2_practice_progress_val = NULL;

static UIImage *level_card_coin_1 = NULL;
static UIImage *level_card_coin_2 = NULL;
static UIImage *level_card_coin_3 = NULL;
static UIImage *level_card_2_coin_1 = NULL;
static UIImage *level_card_2_coin_2 = NULL;
static UIImage *level_card_2_coin_3 = NULL;

#define ANIM_DURATION 0.8f
#define COLOR_FADE_DURATION 0.1f

#define C2D_Color32Const(r, g, b, a) (u32)(r | (g << (u32)8) | (b << (u32)16) | (a << (u32)24))

extern const u32 defaultLvlColors[] = {
    C2D_Color32Const(0, 0, 232, 255),
    C2D_Color32Const(227, 0, 229, 255), 
    C2D_Color32Const(233, 0, 115, 255),
    C2D_Color32Const(233, 0, 0, 255),
    C2D_Color32Const(231, 112, 0, 255),
    C2D_Color32Const(233, 232, 0, 255),
    C2D_Color32Const(0, 231, 0, 255),
    C2D_Color32Const(0, 227, 228, 255),
    C2D_Color32Const(0, 112, 229, 255),
};

extern const size_t numMenuColors = ARRAY_LEN(defaultLvlColors);

UIScreen *top_screen = NULL;
UIScreen *btm_screen = NULL;

#define DOTS_SCALE 0.75f

#define DOTS_X (SCREEN_WIDTH / 2)
#define DOTS_Y 232
#define DOTS_WIDTH 8
#define DOTS_MARGIN 8

static void updateCurrentLevel(int level) {
    char key[18];
    snprintf(key, sizeof(key), "main_%d", level);
    currentLevelEntry = getOrAddLevelToServerFile(currentServerFile, key, LEVEL_LIST_MAIN_LEVELS);
    currentLevelList = LEVEL_LIST_MAIN_LEVELS;
    currentLevelEntry->data.stars = currentMainLevelPack->levels[level].stars;
}

static void drawDots(int levelID) {
    C2D_Sprite spr = { 0 };
    C2D_SpriteFromSheet(&spr, uiSheet, 421);
    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetScale(&spr, DOTS_SCALE, DOTS_SCALE);
    
    C2D_ImageTint tint = {0};

    C2D_PlainImageTint(&tint, C2D_Color32(125, 125, 125, 255), 1.f);

    int leftSide = DOTS_X - ((DOTS_WIDTH * (int)currentMainLevelPack->count) + (DOTS_MARGIN * ((int)currentMainLevelPack->count - 1))) * DOTS_SCALE / 2;

    for (int i = 0; i < (int)currentMainLevelPack->count; i++) {
        C2D_SpriteSetPos(&spr, leftSide + i * (DOTS_WIDTH + DOTS_MARGIN) * DOTS_SCALE, DOTS_Y);
        
        // Do not tint if current level
        if (i == levelID) {
            C2D_DrawSprite(&spr);
        } else {
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
}

void updateLevelName(int level, int card);
void updateLevelStars(int level, int card);

void disableCard2(UIElement *e) {
    e->enabled = false;
}

void enableCard2(UIElement *e) {
    e->enabled = true;
}

void levelCardMoveRight(UIElement *e) {
    e->x += 320;
}

void levelCardMoveLeft(UIElement *e) {
    e->x -= 320;
}

void updateLevelProgress(int level, int card) {
    if (level < 0) level = (int)currentMainLevelPack->count-1;
    if (level >= (int)currentMainLevelPack->count) level = 0;

    updateCurrentLevel(currentMainLevelPack->levels[level].levelID);

    LevelData data = { 0 }; 
    if (currentLevelEntry) {
        data = currentLevelEntry->data;
    }

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data.normalProgress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data.practiceProgress);

    UIProgressBar *normalProg = (card) ? level_card_2_normal_progress : level_card_normal_progress;
    UIProgressBar *practiceProg = (card) ? level_card_2_practice_progress : level_card_practice_progress;
    UILabel *normalProgval = (card) ? level_card_2_normal_progress_val : level_card_normal_progress_val;
    UILabel *practiceProgval = (card) ? level_card_2_practice_progress_val : level_card_practice_progress_val;

    UIImage *coin1 = (card) ? level_card_2_coin_1 : level_card_coin_1;
    UIImage *coin2 = (card) ? level_card_2_coin_2 : level_card_coin_2;
    UIImage *coin3 = (card) ? level_card_2_coin_3 : level_card_coin_3;

    normalProg->value = data.normalProgress;
    practiceProg->value = data.practiceProgress;

    snprintf(normal, sizeof(normal), "%d%%", data.normalProgress);
    snprintf(practice, sizeof(practice), "%d%%", data.practiceProgress);

    uiLabelSetText(normalProgval, normal);
    uiLabelSetText(practiceProgval, practice);
    
    uiImageSetImage(coin1, (data.coin1 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
    uiImageSetImage(coin2, (data.coin2 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
    uiImageSetImage(coin3, (data.coin3 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
}

void updateLevelName(int level, int card) {
    if (level < 0) level = (int)currentMainLevelPack->count-1;
    if (level >= (int)currentMainLevelPack->count) level = 0;

    UILabel *e = (card) ? level_card_2_title : level_card_title;
    level_card_title_top = (UILabel *) uiGetElementByTag(top_screen, "levelname");

    uiElementSetScale((UIElement *) e, 1.f);

    uiLabelSetText(e, currentMainLevelPack->levels[level].levelName);
}

void updateLevelStars(int level, int card) {
    if (level < 0) level = (int)currentMainLevelPack->count-1;
    if (level >= (int)currentMainLevelPack->count) level = 0;

    UILabel *e = (card) ? level_card_2_stars : level_card_stars;
    char stars[10] = { 0 };
    snprintf(stars, 9, "%d", currentMainLevelPack->levels[level].stars);
    uiLabelSetText(e, stars);
}

void updateLevelFace(int level) {
    if (level < 0) level = (int)currentMainLevelPack->count-1;
    if (level >= (int)currentMainLevelPack->count) level = 0;

    uiImageSetImage(level_card_face, 239 + currentMainLevelPack->levels[level].difficulty, 0);
}
void updateLevelTop(int level){
    updateCurrentLevel(currentMainLevelPack->levels[level].levelID);

    LevelData *data = &currentLevelEntry->data;

    char attempts[256];
    snprintf(attempts, sizeof(attempts), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumps[256];
    snprintf(jumps, sizeof(jumps), "<#60abef>Total Jumps</>: %d", data->jumps);

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normalProgress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practiceProgress);

    uiLabelSetText(level_card_title_top, currentMainLevelPack->levels[level].levelName);
    uiLabelSetText((UILabel *) uiGetElementByTag(top_screen, "totalattempts"), attempts);
    uiLabelSetText((UILabel *) uiGetElementByTag(top_screen, "totaljumps"), jumps);
    uiLabelSetText((UILabel *) uiGetElementByTag(top_screen, "normalprogress"), normal);
    uiLabelSetText((UILabel *) uiGetElementByTag(top_screen, "practiceprogress"), practice);
}

void recenter(){
    updateLevelName(currLevelID, 0);
    updateLevelStars(currLevelID, 0);
    updateLevelProgress(currLevelID, 0);
}

void handleCardMovement() {
    if (!dragging) {
        if (animTime > animDuration) {
            updateLevelName(currLevelID, 0);
            updateLevelStars(currLevelID, 0);
            updateLevelProgress(currLevelID, 0);

            uiRunFuncOnTag(btm_screen, "level_card_2", disableCard2);
            uiSetPosOnTag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
            uiSetPosOnTag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card_2");
            scrollDir = 0;
            dragDistance = 0;

            return;
        }

        float start = (scrollDir == 0) ? dragDistance : dragDistance * scrollDir;
        float end = (scrollDir == 0) ? 0 : 320;

        float fadeValue = easeValue(ELASTIC_OUT, start, end, animTime, animDuration, 0.6f);

        float value = (scrollDir == 0) ? 160 + fadeValue : 160 + fadeValue * scrollDir;
        animTime += 0.016666f;

        uiSetPosOnTag(btm_screen, value, LEVEL_CARD_Y_POS, "level_card");
        uiSetPosOnTag(btm_screen, value - 320 * scrollDir, LEVEL_CARD_Y_POS, "level_card_2");
    }
}

void lerpLevelColors(u32 color1, u32 color2){
    float frac = abs(dragDistance) / 320.f;
    frac = clampf(frac, 0.f, 1.f);
    u32 lerpCol = colorLerpU32(color1, color2, frac);
    uploadColorToBuffer(CHANNEL_BG, lerpCol, 0);
}

void peekRight(){
    uiRunFuncOnTag(btm_screen, "level_card_2", enableCard2);

    int card2id = currLevelID + 1;
    if (card2id >= (int)currentMainLevelPack->count) card2id = 0;

    updateLevelName(card2id, 1);
    updateLevelStars(card2id, 1);
    updateLevelProgress(card2id, 1);

    u32 col1 = defaultLvlColors[currLevelID % numMenuColors];
    u32 col2 = defaultLvlColors[card2id % numMenuColors];
    lerpLevelColors(col1, col2);
}

void peekLeft(){
    uiRunFuncOnTag(btm_screen, "level_card_2", enableCard2);

    int card2id = currLevelID - 1;
    if (card2id < 0) card2id = (int)currentMainLevelPack->count-1;

    updateLevelName(card2id, 1);
    updateLevelStars(card2id, 1);
    updateLevelProgress(card2id, 1);

    u32 col1 = defaultLvlColors[currLevelID % numMenuColors];
    u32 col2 = defaultLvlColors[card2id % numMenuColors];
    lerpLevelColors(col1, col2);
}

void tintGround(UIElement *e) {
    ColorChannel channel = channels[CHANNEL_BG];
    uiImageSetTint((UIImage *) e, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
}

void actionOpenLevel(UIElement* e, const UIPropertyList *args) { 
    recenter();
    stopMP3();
    playingMenuLoop = false;
    uiStackPushGameState(STATE_GAME);
}

void actionMoveRight(UIElement* e, const UIPropertyList *args) { 
    currLevelID++;
    scrollDir = -1;
    animTime = 0;
    animDuration = 1.f;
    
    if (currLevelID >= (int)currentMainLevelPack->count) currLevelID = 0;
    
    uiSetPosOnTag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
    uiRunFuncOnTag(btm_screen, "level_card_2", enableCard2);
    uiRunFuncOnTag(btm_screen, "level_card_2", levelCardMoveRight);
    
    uploadColorToBuffer(CHANNEL_BG, defaultLvlColors[currLevelID % numMenuColors], COLOR_FADE_DURATION);

    updateLevelName(currLevelID - 1, 0);
    updateLevelStars(currLevelID - 1, 0);
    updateLevelProgress(currLevelID - 1, 0);
    
    updateLevelFace(currLevelID);
    updateLevelTop(currLevelID);

    updateLevelName(currLevelID, 1);
    updateLevelStars(currLevelID, 1);
    updateLevelProgress(currLevelID, 1);
};

void actionMoveLeft(UIElement* e, const UIPropertyList *args) { 
    currLevelID--;
    scrollDir = 1;
    animTime = 0;
    animDuration = 1.f;

    if (currLevelID < 0) currLevelID = (int)currentMainLevelPack->count-1;

    uiSetPosOnTag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
    uiRunFuncOnTag(btm_screen, "level_card_2", enableCard2);
    uiRunFuncOnTag(btm_screen, "level_card_2", levelCardMoveLeft);

    uploadColorToBuffer(CHANNEL_BG, defaultLvlColors[currLevelID % numMenuColors], COLOR_FADE_DURATION);

    updateLevelName(currLevelID + 1, 0);
    updateLevelStars(currLevelID + 1, 0);
    updateLevelProgress(currLevelID + 1, 0);
    
    updateLevelFace(currLevelID);
    updateLevelTop(currLevelID);

    updateLevelName(currLevelID, 1);
    updateLevelStars(currLevelID, 1);
    updateLevelProgress(currLevelID, 1);
}

static UIActionDef actions[] = {
    {"open_level", actionOpenLevel},
    {"move_right", actionMoveRight},
    {"move_left", actionMoveLeft}
};

int mode = 0;

void levelSelectInit(UIScreen *s){
    initColChannels();
    btm_screen = s;
    
    currLevelID = 0;
    startLevel = false;
    state.customLevel = false;
    state.onlineLevel = false;
    dragDistance = 0;

    // Set window color
    level_card_window = (UIWindowButton *) uiGetElementByTag(s, "card_window");
    uiWindowButtonSetTint(level_card_window, C2D_Color32(0, 0, 0, 127));

    level_card_2_window = (UIWindowButton *) uiGetElementByTag(s, "card_window_2");
    uiWindowButtonSetTint(level_card_2_window, C2D_Color32(0, 0, 0, 127));

    // Get level card components
    level_card_title = (UILabel *) uiGetElementByTag(s, "level_title");
    level_card_stars = (UILabel *) uiGetElementByTag(s, "level_stars");

    level_card_2_title = (UILabel *) uiGetElementByTag(s, "level_title_2");
    level_card_2_stars = (UILabel *) uiGetElementByTag(s, "level_stars_2");

    level_card_normal_progress = (UIProgressBar *) uiGetElementByTag(s, "normalprogress");
    level_card_normal_progress_val = (UILabel *) uiGetElementByTag(s, "normalprogressvalue");
    level_card_2_normal_progress = (UIProgressBar *) uiGetElementByTag(s, "normalprogress_2");
    level_card_2_normal_progress_val = (UILabel *) uiGetElementByTag(s, "normalprogressvalue_2");
    
    level_card_practice_progress = (UIProgressBar *) uiGetElementByTag(s, "practiceprogress");
    level_card_practice_progress_val = (UILabel *) uiGetElementByTag(s, "practiceprogressvalue");
    level_card_2_practice_progress = (UIProgressBar *) uiGetElementByTag(s, "practiceprogress_2");
    level_card_2_practice_progress_val = (UILabel *) uiGetElementByTag(s, "practiceprogressvalue_2");

    level_card_coin_1 = (UIImage *) uiGetElementByTag(s, "coin_1");
    level_card_coin_2 = (UIImage *) uiGetElementByTag(s, "coin_2");
    level_card_coin_3 = (UIImage *) uiGetElementByTag(s, "coin_3");
    level_card_2_coin_1 = (UIImage *) uiGetElementByTag(s, "coin_1_2");
    level_card_2_coin_2 = (UIImage *) uiGetElementByTag(s, "coin_2_2");
    level_card_2_coin_3 = (UIImage *) uiGetElementByTag(s, "coin_3_2");

    updateCurrentLevel(currentMainLevelPack->levels[currLevelID].levelID);

    uiProgressBarSetTint(level_card_normal_progress, C2D_Color32(0, 255, 0, 255));
    uiProgressBarSetTint(level_card_2_normal_progress, C2D_Color32(0, 255, 0, 255));

    uiProgressBarSetTint(level_card_practice_progress, C2D_Color32(0, 255, 255, 255));
    uiProgressBarSetTint(level_card_2_practice_progress, C2D_Color32(0, 255, 255, 255));

    updateLevelName(currLevelID, 0);
    updateLevelStars(currLevelID, 0);
    updateLevelFace(currLevelID);
    updateLevelTop(currLevelID);
    updateLevelProgress(currLevelID, 0);

    uiRunFuncOnTag(s, "level_card_2", disableCard2);

    u32 color = defaultLvlColors[currLevelID % numMenuColors];
    uploadColorToBuffer(CHANNEL_BG, color, 0);

    dragging = false;
    dragDistance = 0;
    dragDir = 0;
    cardCorrection = false;

    scrollDir = 0;
        
    // Set bg color
    bgGradient = (UIImage *) uiGetElementByTag(s, "gradient");

    playMenuSong();
}

void levelSelectInitTop(UIScreen *s){
    top_screen = s;

    level_card_face  = (UIImage *) uiGetElementByTag(s, "level_face");

    bg_gradient_top = (UIImage *) uiGetElementByTag(s, "gradient");
}

void levelSelectUpdate(UIScreen *s, UIInput *input){
    if (exitingLevel) {
        //make sure the color immediately sets when the level is exited
        scrollDir = 0;
        dragging = false;
        uiRunFuncOnTag(s, "level_card_2", disableCard2);
        recenter();
        uploadColorToBuffer(CHANNEL_BG, defaultLvlColors[currLevelID % numMenuColors], 0);
        updateLevelTop(currLevelID);
    }
    exitingLevel = false;
    
    if(!scrollDir && !dragging){
        uploadColorToBuffer(CHANNEL_BG, defaultLvlColors[currLevelID % numMenuColors], 0);
    }

    handleColChannel(CHANNEL_BG);

    ColorChannel channel = channels[CHANNEL_BG];

    uiImageSetTint(bgGradient, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
    uiImageSetTint(bg_gradient_top, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
    uiRunFuncOnTag(s, "ground", tintGround);

    if(cardCorrection){
        uiRunFuncOnTag(s, "level_card_2", disableCard2);
        uiSetPosOnTag(s, 160, LEVEL_CARD_Y_POS, "level_card_2");
        cardCorrection = false;
    }

    if((input->down & KEY_TOUCH)){
        dragStartX = input->touchPos.px;
        dragging = false;
        dragDir = 0;
        dragDistance = 0;

        recenter();
    }
    if((input->held & KEY_TOUCH)){
        if(dragging){
            int delta = input->touchPos.px - lastTouchX;
            dragDistance += delta;

            if(dragDistance > 15){
                dragDir = -1;
                peekLeft();
            } else if(dragDistance < -15){
                dragDir = 1;
                peekRight();
            } else{
                dragDir = 0;
                uiRunFuncOnTag(s, "level_card_2", disableCard2);
            }

            uiSetPosOnTag(s, 160 + dragDistance, LEVEL_CARD_Y_POS, "level_card");
            uiSetPosOnTag(s, 160 + dragDistance + (dragDir * 320), LEVEL_CARD_Y_POS, "level_card_2");
        } else {
            dragDir = 0;

            int dragXDiff = input->touchPos.px - dragStartX;
            if(abs(dragXDiff) > 10){
                dragging = true;
                scrollDir = 0;
                animDuration = 1.f;
                animTime = animDuration;
                dragDistance = 0;
                dragDir = 0;
                cardCorrection = true;
                recenter();
            }
        }
        lastTouchX = input->touchPos.px;
    }
    if((input->up & KEY_TOUCH)){
        if(dragging){
            if(dragDir == -1){
                actionMoveLeft(NULL, NULL);
                animDuration = 0.5f;
            } else if(dragDir == 1){
                actionMoveRight(NULL, NULL);
                animDuration = 0.5f;
            } else{
                scrollDir = 0;
                animTime = 0.f;
            }
        }
        dragging = false;
        dragDir = 0;
    }
    
    handleCardMovement();

    //Buttons can't be tapped while dragging, left/right buttons cannot be pressed with keybinds while dragging
    if(dragging){
        input->touchPos.px = -99;
        input->touchPos.py = -99;
        ((UIButton *)uiGetElementByTag(s, "left"))->keyBinds = 0;
        ((UIButton *)uiGetElementByTag(s, "right"))->keyBinds = 0;
    } else{
        ((UIButton *)uiGetElementByTag(s, "left"))->keyBinds = (KEY_DLEFT | KEY_L | KEY_ZL | KEY_CPAD_LEFT | KEY_CSTICK_LEFT);
        ((UIButton *)uiGetElementByTag(s, "right"))->keyBinds = (KEY_DRIGHT | KEY_R | KEY_ZR | KEY_CPAD_RIGHT | KEY_CSTICK_RIGHT);
    }
}

void levelSelectDrawTop(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_BEFORE) return;

    drawDots(currLevelID);
}

extern const UIScreenDefPair levelSelectDef = {
    .name = "level_select",
    .top = {
        .path = "romfs:/menus/level_select_top.txt",
        .init = levelSelectInitTop,
        .draw = levelSelectDrawTop
    },
    .btm = {
        .path = "romfs:/menus/level_select.txt",
        .init = levelSelectInit,
        .update = levelSelectUpdate,
        .actionList = {
            .actions = actions,
            .actionCount = ARRAY_LEN(actions)
        }
    }
};

}