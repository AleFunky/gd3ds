#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "icons.hpp"
#include "math_helpers.hpp"
#include "main.hpp"
#include "graphics.hpp"
#include "utils/gfx.hpp"
#include "state.hpp"
#include "triggers.hpp"
#include "utils/utils.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"

#include "menus/palette_kit.hpp"
#include "menus/main_menu.hpp"
#include "menus/level_select.hpp"
#include "menus/updater.hpp"
#include "menus/updater_settings.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/settings_hub/info_card.hpp"

#include "particles/object_particles.hpp"
#include "particles/circles.hpp"

#include "save/saving.hpp"

#define DEATH_WAITING_TIME 0.5f
#define OFFSCREEN_BUFFER 240

namespace gd3ds
{

static UIButton *quitGameButton;

static Player titleScreenPlayer;
static bool titleScreenPlayerHold = false;

static bool pressing = false;
static bool oldPressing = false;

static bool started = false;
static bool holding = false;

static float deathWaitTimer = 0;

static int mainMenuColorIndex = 0;

static float bgScroll = 0;

bool oldWide;
bool oldStereo;

static void actionOpenQuitGamePopUp(UIElement *e, const UIPropertyList *args){
    TwoOptionPopupData *quitData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
    if(!quitData) return;

    quitData->text = strdup("Are you sure you want to quit?");
    quitData->title = strdup("Quit Game");
    quitData->proceedText = strdup("Quit");
    quitData->cancelText = strdup("Cancel");

    uiStackPush(&quitPopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    uiStackPushData(quitData);
}

void handleTitleScreenPlayer(Player *player) {
    if (state.input.holdJump) {
        if (player->bufferingState == BUFFER_NONE) {
            player->bufferingState = BUFFER_READY;
        }
    } else {
        player->bufferingState = BUFFER_NONE;
    }

    player->onGround = false;
    player->onCeiling = false;

    player->velocityOverride = false;

    player->gravObjID = -1;
    player->potentialSlopeID = -1;
    
    player->timeElapsed += STEPS_DT;

    player->upsideDown = false;

    player->velX = playerSpeeds[state.speed]; 
    player->x += player->velX * STEPS_DT;
    player->y += player->velY * STEPS_DT;

    runPlayer(player);
}

void resetPlayers() {
    deathWaitTimer = 0;

    initState();
    levelInfo.wallX = 9999999999999999999.f;
    levelInfo.wallY = 0;
    initPlayer(&titleScreenPlayer);
    titleScreenPlayer.x = -120;
    titleScreenPlayer.upsideDown = false;
    state.speed = randomInt(0, SPEED_COUNT - 1);
    titleScreenPlayer.rotation = 0;
    setGamemode(&titleScreenPlayer, randomInt(0, GAMEMODE_COUNT - 1));
    setMini(&titleScreenPlayer, randomInt(0,1));

    titleScreenPlayer.playerIcons.cube = randomInt(1, ICON_COUNT_PLAYER - 1);
    titleScreenPlayer.playerIcons.ship = randomInt(1, ICON_COUNT_SHIP - 1);
    titleScreenPlayer.playerIcons.ball = randomInt(1, ICON_COUNT_PLAYER_BALL - 1);
    titleScreenPlayer.playerIcons.ufo  = randomInt(1, ICON_COUNT_BIRD - 1);
    titleScreenPlayer.playerIcons.wave = randomInt(1, ICON_COUNT_DART - 1);
    titleScreenPlayer.playerIcons.robot = randomInt(1, ICON_COUNT_ROBOT - 1);
    titleScreenPlayer.playerIcons.spider = randomInt(1, ICON_COUNT_SPIDER - 1);
    titleScreenPlayer.playerIcons.glow = false;

    titleScreenPlayerHold = randomInt(0,1);
    titleScreenPlayer.y = titleScreenPlayer.height / 2;
    
    Color p1 = getColorAbgr8(colors[randomInt(0, numColors - 1)]);
    Color p2 = getColorAbgr8(colors[randomInt(0, numColors - 1)]);

    titleScreenPlayer.playerIcons.p1Color = p1;
    titleScreenPlayer.playerIcons.p2Color = p2;
    
    initParticles(p1, p2);
    initTrails(0);
    
    trailP1.color = (randomInt(0, 1) ? p1 : p2);
    
    waveTrailP1.color = (randomInt(0, 1) ? p1 : p2);
    waveTrailP1.blending = true;
    waveTrailP1.opacity = 1.f;
}

static void handleInput() {
    oldPressing = pressing;
    pressing = false;
    switch (titleScreenPlayer.gamemode) {
        case GAMEMODE_PLAYER:
            // Press A 1/16 of time
            if (!(rand() & 0b1111)) pressing = true;
            break;
        case GAMEMODE_UFO:
            // Press A 1/16 of time
            if (!(rand() & 0b1111)) pressing = true;
            break;
        case GAMEMODE_WAVE:
            // Switch holding A 1/16 of time
            if (!(rand() & 0b1111)) titleScreenPlayerHold ^= 1;

            if (titleScreenPlayerHold) {
                pressing = true;
            }
            break;
        case GAMEMODE_SHIP:
            // Switch holding A 1/16 of time
            if (!(rand() & 0b1111)) titleScreenPlayerHold ^= 1;

            if (titleScreenPlayerHold) {
                pressing = true;
            }
            break;
        case GAMEMODE_ROBOT:
            if (titleScreenPlayerHold) {
                // If holding, 1/8 change to stop holding
                if (!(rand() & 0b111)) titleScreenPlayerHold = 0;
                pressing = true;
            } else {
                // Else 1/16 change to start holding
                if (!(rand() & 0b1111)) titleScreenPlayerHold = 0;
            }
            break;
        case GAMEMODE_SPIDER:
            pressing = false;
            break;
    }

    started = !oldPressing && pressing;
    holding = pressing;
}

static void handlePlayers() {
    state.oldPlayer = titleScreenPlayer;
    handleTitleScreenPlayer(&titleScreenPlayer);

    bool wtrailOpacityIsZero = (titleScreenPlayer.gamemode == GAMEMODE_WAVE ? waveTrailP1.opacity <= 0.f : true);
    
    // Wave trail needs to be invisible in order to despawn this player
    if (wtrailOpacityIsZero && titleScreenPlayer.x >= SCREEN_WIDTH_AREA + OFFSCREEN_BUFFER) {
        motionTrailUpdateWaveTrail(&waveTrailP1, 1.f/60);
        resetPlayers();
    }

    // Force the player go down by disabling control
    if (titleScreenPlayer.y >= SCREEN_HEIGHT_AREA * 2 - 90) {
        pressing = false;
        titleScreenPlayerHold = false;
    }
}

static void mainMenuInit(UIScreen *s){
    initColChannels();
    playMenuSong();

    quitGameButton = (UIButton *)uiGetElementByTag(s, "quitbutton");
    if (!is3Dsx) uiDisableElement((UIElement *)quitGameButton);

    mainMenuColorIndex = 0;
    u32 color = defaultLvlColors[mainMenuColorIndex % numMenuColors];
    mainMenuColorIndex++;

    Color col;
    col.r = GET_R(color);
    col.g = GET_G(color);
    col.b = GET_B(color);

    int chanBg = getColChannelIndex(CHANNEL_BG);
    int chanGround = getColChannelIndex(CHANNEL_GROUND);
    int chanGround2 = getColChannelIndex(CHANNEL_GROUND_2);
    int chanLine = getColChannelIndex(CHANNEL_LINE);

    channels[chanBg].color = col;
    channels[chanGround].color = col;
    channels[chanGround2].color = col;
    channels[chanLine].color = white;

    getBuffer(CHANNEL_BG)->active = false;
    getBuffer(CHANNEL_GROUND)->active = false;
    getBuffer(CHANNEL_GROUND_2)->active = false;
    getBuffer(CHANNEL_LINE)->active = false;

    allocateParticles();
    initVariables();
    resetPlayers();

    state.currentPlayer = 0;
    trail = &trailP1;
    waveTrail = &waveTrailP1;

    oldWide = settingsState.wideEnabled;
    oldStereo = settingsState.stereoEnabled;

    if (!isN3Ds && is3Dsx) {
      InfoCardData *warningData = (InfoCardData *)malloc(sizeof(InfoCardData));
      if (!warningData)
        return;

      warningData->text = strdup("The .3dsx release lacks RAM\non non-New 3DS hardware.\nPlease install the .cia.");
      warningData->title = strdup("Warning");
      warningData->copied = true;
      uiStackPush(&infoCardDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
      uiStackPushData(warningData);
    }

    if (doAutoUpdates && !checkedForUpdate) updaterThread = createGenericThread(&updaterTask);
}

static void mainMenuInitTop(UIScreen *s){
    UIImage *title = (UIImage *) uiGetElementByTag(s, "title");

    if (title && altTitleScreen) {
        uiImageSetImage(title, 3, 1);
    }
}

static void mainMenuUpdate(UIScreen *s, UIInput *input){

    if (settingsState.wideEnabled != oldWide || settingsState.stereoEnabled != oldStereo) {
        gspWaitForVBlank();
        applyScreenModes();
        gspWaitForVBlank();
        reinitializeScreens();
        oldWide = settingsState.wideEnabled;
        oldStereo = settingsState.stereoEnabled;
    }

    state.oldInput = state.input;
    state.input.pressedJump = (started) == true;
    state.input.holdJump = (state.input.pressedJump || holding) == true;
    
    currInput = state.input;
    currOldInput = state.oldInput;

    for (int i = 0; i < 2; i++) {
        dragParticles[i].emitting = false;
        dragParticles2[i].stationary = true;
        dragParticles2[i].emitting = false;
        shipFireParticles[i].emitting = false;
        secondaryParticles[i].emitting = false;
        shipSecondaryParticles[i].emitting = false;
        burstParticles[i].emitting = false;
        landParticles[i].emitting = false;
    }
    
    brickDestroyParticles.emitting = false;
    slowSpeedParticlesBottom.emitting = false;
    normalSpeedParticlesBottom.emitting = false;
    fastSpeedParticlesBottom.emitting = false;
    fasterSpeedParticlesBottom.emitting = false;
    fastestSpeedParticlesBottom.emitting = false;

    p1Trail = false;
    clearP1Trail();

    handleInput();

    if (!state.dead) {
        for (int i = 0; i < 4; i++) {
            handlePlayers();
        }
    }
    
    glitterParticles.emitting = false;
    glitterParticlesBottom.emitting = false;

    // Fade wave trail
    if (titleScreenPlayer.gamemode == GAMEMODE_WAVE && (state.dead || titleScreenPlayer.x >= SCREEN_WIDTH_AREA)) {
        if (waveTrail->opacity > 0) waveTrail->opacity -= 0.08f;
        
        if (waveTrail->opacity <= 0) {
            waveTrail->opacity = 0;
            waveTrail->nuPoints = 0;
        }
    }

    motionTrailUpdate(&trailP1, delta);
    motionTrailUpdateWaveTrail(&waveTrailP1, delta);
    updatePlayerEffects(delta);
    updateUseEffects(delta, getUseEffectArrayPtr(GFX_TOP));

    float touch_x = input->touchPos.px/SCALE;
    float touch_y = SCREEN_HEIGHT - input->touchPos.py/SCALE;

    bool kill = (hidKeysDown() & KEY_TOUCH) && intersect(
        titleScreenPlayer.x, titleScreenPlayer.y, titleScreenPlayer.width, titleScreenPlayer.height, 0,
        touch_x, touch_y, 9, 9, 0
    );

    if (kill && !state.dead) {
        killPlayer(DEATH_TITLE_SCREEN_KILL);
        playersDestroyed++;
        handleDeath(&titleScreenPlayer, false);
        deathWaitTimer = DEATH_WAITING_TIME;
    } 

    // Wait to reset the player
    if (deathWaitTimer) {
        deathWaitTimer -= delta;
        if (deathWaitTimer <= 0) {
            resetPlayers();
            deathWaitTimer = 0;
            state.dead = false;
        }
    }

    handleColChannel(CHANNEL_BG);
    handleColChannel(CHANNEL_GROUND);
    handleColChannel(CHANNEL_GROUND_2);

    ColTriggerBuffer *trig = getBuffer(CHANNEL_BG);
    if (!trig->active) {
        uploadColorToBuffer(CHANNEL_BG, defaultLvlColors[mainMenuColorIndex % numMenuColors], 4.f);
        uploadColorToBuffer(CHANNEL_GROUND, defaultLvlColors[mainMenuColorIndex % numMenuColors], 4.f);
        uploadColorToBuffer(CHANNEL_GROUND_2, defaultLvlColors[mainMenuColorIndex % numMenuColors], 4.f);
        mainMenuColorIndex++;
    }
    
    bgScroll += 5.19300155f;
}

//bottom screen
static void mainMenuDraw(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_AFTER) return;

    drawBackground(bgScroll / 8, SCREEN_HEIGHT);

    C2D_ViewScale(SCALE, SCALE);
    state.cameraX = 0;
    state.cameraY = 0;

    drawPlayerEffects();

    changeBlending(true);

    drawUseEffects(getUseEffectArrayPtr(GFX_TOP));

    changeBlending(false);
    drawPlayer(&titleScreenPlayer);
    drawPostPlayerEffects();

    drawGround(bgScroll, 0, 0, false, 320);

    C2D_ViewScale(1/SCALE, 1/SCALE);
}

static void mainMenuDrawTop(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_AFTER) return;

    beginEyeLayer(DEPTH_BACKGROUND);
    drawBackground(-40 + (bgScroll / 8), 0);
    endEyeLayer();

    C2D_ViewScale(SCALE, SCALE);
    state.cameraX = -((SCREEN_WIDTH_AREA - SCREEN_WIDTH_AREA_BOT)/2);
    state.cameraY = SCREEN_HEIGHT_AREA;

    // Same trick as in game, the player floats in front of the screen
    beginEyeLayer(DEPTH_LEVEL);
    drawPlayerEffects();
    changeBlending(true);
    drawUseEffects(getUseEffectArrayPtr(GFX_TOP));

    changeBlending(false);
    drawPlayer(&titleScreenPlayer);
    endEyeLayer();

    C2D_ViewScale(1/SCALE, 1/SCALE);
}

static void mainMenuExit(UIScreen *s) {
    freeParticles();
}

const UIActionDef mainMenuActions[] = {
    { "quit_game", actionOpenQuitGamePopUp }
};


extern const UIScreenDefPair mainMenuDef = {
    .name = "main_menu",
    .top = {
        .path = "romfs:/menus/main_menu_top.txt",
        .init = mainMenuInitTop,
        .draw = mainMenuDrawTop
    },
    .btm = {
        .path = "romfs:/menus/main_menu.txt",
        .init = mainMenuInit,
        .update = mainMenuUpdate,
        .draw = mainMenuDraw,
        .exit = mainMenuExit,
        .actionList = {
            .actions = mainMenuActions,
            .actionCount = ARRAY_LEN(mainMenuActions)
        }
    }
};

}