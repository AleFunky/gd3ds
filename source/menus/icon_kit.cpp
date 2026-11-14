#include <3ds.h>
#include <citro2d.h>
#include "icons.hpp"
#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_textbox.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/components/ui_icon.hpp"
#include "menus/components/ui_label.hpp"
#include "save/saving.hpp"
#include "fonts/bigFont.hpp"
#include "main.hpp"
#include "easing.hpp"
#include "triggers.hpp"
#include "mp3_player.hpp"
#include "graphics.hpp"
#include "icon_kit.hpp"
#include "level/main_levels.hpp"

#include "palette_kit.hpp"

#include "save/config.hpp"
#include "state.hpp"
#include "menus/creator_menu/search_menu.hpp"

namespace gd3ds
{

static int gamemodePage = 0;

// Page handling 

static int currentCubePage = 0;
static int currentShipPage = 0;
static int currentBallPage = 0;
static int currentUfoPage  = 0;
static int currentWavePage = 0;
static int currentRobotPage = 0;
static int currentSpiderPage = 0;
static int currentTrailPage = 0;

static int lastDisplayedGamemode = 0;

int selectedCube = 1;
int selectedShip = 1;
int selectedBall = 1;
int selectedUfo  = 1;
int selectedWave = 1;
int selectedRobot = 1;
int selectedSpider = 1;
int selectedTrail = 1;

int selectedP1 = 0;
int selectedP2 = 0;
int selectedGlow = 0;

bool playerGlowEnabled = false;
bool showGlow = false;

const int gamemodeIconCount[ICON_GAMEMODE_COUNT + 1] = {
    ICON_COUNT_PLAYER,
    ICON_COUNT_SHIP,
    ICON_COUNT_PLAYER_BALL,
    ICON_COUNT_BIRD,
    ICON_COUNT_DART,
    ICON_COUNT_ROBOT,
    ICON_COUNT_SPIDER,
    TRAIL_COUNT
};

static int *currentPages[ICON_GAMEMODE_COUNT + 1] = {
    &currentCubePage,
    &currentShipPage,
    &currentBallPage,
    &currentUfoPage,
    &currentWavePage,
    &currentRobotPage,
    &currentSpiderPage,
    &currentTrailPage
};

static UIButton *gamemodeBtns[ICON_GAMEMODE_COUNT + 1];

int *currentIcons[ICON_GAMEMODE_COUNT + 1] = {
    &selectedCube,
    &selectedShip,
    &selectedBall,
    &selectedUfo,
    &selectedWave,
    &selectedRobot,
    &selectedSpider,
    &selectedTrail
};

int *currentColors[3] = {
    &selectedP1,
    &selectedP2,
    &selectedGlow
};

static const int buttonImages[8] = {
    341,
    351,
    325,
    327,
    329,
    349,
    353,
    355
};
static int iconCounter = 1;

static void setIconIndex(UIElement *e) {
    int newIndex = (*currentPages[gamemodePage] * ICONS_PER_PAGE) + iconCounter;
    if (newIndex < gamemodeIconCount[gamemodePage]) {
        e->enabled = true;
        uiIconSetSelected((UIIcon *) e, *currentIcons[gamemodePage] == newIndex);
        uiIconSetGamemodeIndex((UIIcon *) e, gamemodePage, newIndex);

        if (gamemodePage == static_cast<int>(IconGamemode::GamemodeShip) || gamemodePage == static_cast<int>(IconGamemode::GamemodeUfo) || gamemodePage == static_cast<int>(IconGamemode::GamemodeSpider)) {
            uiElementSetScale(e, .7f);
        } else if (gamemodePage == static_cast<int>(IconGamemode::GamemodeWave)) {
            uiElementSetScale(e, .95f);
        } else {
            uiElementSetScale(e, .8f);
        }

        iconCounter++;
    } else {
        e->enabled = false;
    }
}

static void setTrailIndex(UIElement *e) {
    int newIndex = iconCounter;
    if (newIndex < gamemodeIconCount[gamemodePage]) {
        e->enabled = true;
        uiIconSetSelected((UIIcon *) e, *currentIcons[gamemodePage] == newIndex);
        uiIconSetGamemodeIndex((UIIcon *) e, gamemodePage, newIndex);
        iconCounter++;
    } else {
        e->enabled = false;
    }
}

static void disableAllIconButtons(UIScreen *s) {
    iconCounter = (gamemodePage == TRAIL ? 0 : 1);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "cube"), buttonImages[0], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "ship"), buttonImages[1], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "ball"), buttonImages[2], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "ufo"),  buttonImages[3], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "dart"), buttonImages[4], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "robot"), buttonImages[5], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "spider"), buttonImages[6], 0);
    uiButtonSetImage((UIButton *) uiGetElementByTag(s, "trail"), buttonImages[7], 0);
}

static void actionSetPage(UIElement *e, const UIPropertyList *args) {
    gamemodePage = uiPropInt(args, "page", 0);
    lastDisplayedGamemode = gamemodePage;
    disableAllIconButtons(e->screen);
    uiButtonSetImage((UIButton *) e, buttonImages[gamemodePage] + 1, 0);
    uiRunFuncOnTag(e->screen, "icon", setIconIndex); 

    for(int i = 0; i < ICON_GAMEMODE_COUNT + 1; i++){
        gamemodeBtns[i]->keyBinds = 0;
        if(i == gamemodePage - 1){
            gamemodeBtns[i]->keyBinds |= KEY_L | KEY_ZL;
        } else if(i == gamemodePage + 1){
            gamemodeBtns[i]->keyBinds |= KEY_R | KEY_ZR;
        } 
    }
}

static void setTrailPage(UIElement *e, const UIPropertyList *args) {
    gamemodePage = 7;
    disableAllIconButtons(e->screen);
    uiButtonSetImage((UIButton *) e, buttonImages[7] + 1, 0);
    uiRunFuncOnTag(e->screen, "icon", setTrailIndex); 
}


static void moveIndexLeft(UIElement* e, const UIPropertyList *args) {
    *currentPages[gamemodePage] -= 1;
    if (*currentPages[gamemodePage] < 0) {
        *currentPages[gamemodePage] = (gamemodeIconCount[gamemodePage] - 2) / ICONS_PER_PAGE;
    }
    iconCounter = (gamemodePage == TRAIL ? 0 : 1);
    uiRunFuncOnTag(e->screen, "icon", setIconIndex); 
}

static void moveIndexRight(UIElement* e, const UIPropertyList *args) {
    *currentPages[gamemodePage] += 1;
    if ((*currentPages[gamemodePage] * ICONS_PER_PAGE) + 1 >= gamemodeIconCount[gamemodePage]) {
        *currentPages[gamemodePage] = 0;
    }
    iconCounter = (gamemodePage == TRAIL ? 0 : 1);
    uiRunFuncOnTag(e->screen, "icon", setIconIndex); 
}

static void actionIconSelected(UIElement *e, const UIPropertyList *args) {
    *currentIcons[((UIIcon *) e)->gamemode] = ((UIIcon *) e)->index;
    iconCounter = (gamemodePage == TRAIL ? 0 : 1);
    uiRunFuncOnTag(e->screen, "icon", setIconIndex); 
}

static void iconKitInit(UIScreen *s){
    gamemodeBtns[0] = (UIButton *) uiGetElementByTag(s, "cube");
    gamemodeBtns[1] = (UIButton *) uiGetElementByTag(s, "ship");
    gamemodeBtns[2] = (UIButton *) uiGetElementByTag(s, "ball");
    gamemodeBtns[3] = (UIButton *) uiGetElementByTag(s, "ufo");
    gamemodeBtns[4] = (UIButton *) uiGetElementByTag(s, "dart");
    gamemodeBtns[5] = (UIButton *) uiGetElementByTag(s, "robot");
    gamemodeBtns[6] = (UIButton *) uiGetElementByTag(s, "spider");
    gamemodeBtns[7] = (UIButton *) uiGetElementByTag(s, "trail");

    actionSetPage(uiGetElementByTag(s, "cube"), NULL);

    updatePlayerColors();
    showGlow = (playerGlowEnabled || ((p1Color.r | p1Color.g | p1Color.b) == 0));

    playMenuSong();
}

static void iconKitInitTop(UIScreen *s){
    char stars[32];
    snprintf(stars, sizeof(stars), "%d", totalStars);
    
    char coins[32];
    snprintf(coins, sizeof(coins), "%d", totalCoins);

    char userCoins[32];
    snprintf(userCoins, sizeof(userCoins), "%d", totalUserCoins);

    uiLabelSetText((UILabel *) uiGetElementByTag(s, "star_text"), stars);
    uiLabelSetText((UILabel *) uiGetElementByTag(s, "secretcoins_text"), coins);
    uiLabelSetText((UILabel *) uiGetElementByTag(s, "usercoins_text"), userCoins);

    if (!userCoinsCounterVisible()) {
        uiRunFuncOnTag(s, "usercoins_icon", uiDisableElement);
        uiRunFuncOnTag(s, "usercoins_text", uiDisableElement);
    }

}

static void iconKitUpdateTop(UIScreen *s, UIInput *input){
    UIIcon *icon = (UIIcon *)uiGetElementByTag(s, "big_icon");
 
    uiIconSetGamemodeIndex(icon, lastDisplayedGamemode, *currentIcons[lastDisplayedGamemode]);
    uiIconSetP1(icon, C2D_Color32(p1Color.r, p1Color.g, p1Color.b, 255));
    uiIconSetP2(icon, C2D_Color32(p2Color.r, p2Color.g, p2Color.b, 255));
    uiIconSetGlow(icon, C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255));
    icon->glow = showGlow;
}

static const UIActionDef iconKitActions[] = {
    {"action_page", actionSetPage },
    {"action_trail", setTrailPage },
    {"icons_left", moveIndexLeft },
    {"icons_right", moveIndexRight },
    {"icon_selected", actionIconSelected }
};

extern const UIScreenDefPair iconKitDef = {
    .name = "icon_kit",
    .top = {
        .path = "romfs:/menus/icon_kit_top.txt",
        .init = iconKitInitTop,
        .update = iconKitUpdateTop
    },
    .btm = {
        .path = "romfs:/menus/icon_kit.txt",
        .init = iconKitInit,
        .actionList = {
            .actions = iconKitActions,
            .actionCount = ARRAY_LEN(iconKitActions)
        }
    }
};

}