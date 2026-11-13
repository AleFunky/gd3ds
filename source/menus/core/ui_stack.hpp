#pragma once
#include "main.hpp"
#include "ui_screen.hpp"
#include "graphics.hpp"

typedef struct UIScreenPair {
    const char* name;
    //top and bottom screen
    UIScreen screens[2];  
    //whether this ScreenPair will be treated as the backmost ScreenPair of the current UI view (menus such as the main menu, level select, etc; NOT popup ScreenPairs like settings or color select)
    bool root;

    void *data;
    void (*freeData)(void *data);
} UIScreenPair;

typedef enum {
    PUSH_NONE,
    PUSH_NEXT,
    PUSH_AFTER_CLOSE,
    PUSH_ROOT,
    PUSH_GAME_STATE
} UIStackPushType;

typedef struct {
    const UIScreenDefPair *defs;
    UIAnimation topAnim;
    UIAnimation btmAnim;
    UIStackPushType type;
    bool pushNow;
    void *data;
} UIStackPush;

typedef struct {
    //array of pointers to UIScreenPairs
    UIScreenPair **screenStack;
    size_t stackCapacity;

    //"root" ScreenPairs represent the lowest layer of the current UI context, such as the main menu, creator menu, or level select (as opposed to popups like settings, the color select, etc)
    //ScreenPairs which are below the current root will be unloaded and not updated; reloaded when the current root/context is exited
    size_t currentRoot;
    //amount of ScreenPairs to update (starts at root and goes up)
    size_t activeCount;

    //used for fade to/from black when changing roots
    UITransitionState rootTransition; //open or close
    FadeStatus fade;
    float fadeTime;

    UIStackPush push;

    int nextGameState;
} UIStack;

bool uiStackCheckLoadedRoot(const UIScreenDefPair *screen);
bool uiStackCheckLoadedScreenInRoot(const UIScreenDefPair *screen);
UIScreenPair *uiStackGetLoadedScreen(const UIScreenDefPair *screen);

void uiStackSetStack(UIStack *setStack);

void uiStackPush(const UIScreenDefPair *defs, UIAnimation topAnim, UIAnimation btmAnim, UIStackPushType type);
void uiStackPushName(const char *name, UIAnimation topAnim, UIAnimation btmAnim, UIStackPushType type);
void uiStackPushData(void *data);
void uiStackPushRootInstant(const UIScreenDefPair *defs);
void uiStackPushGameState(int gameState);

void uiStackPop();
void uiStackPopContext();

void uiStackUpdate(UIInput *input);

void drawStackDebug();
void drawStackFade();
void uiStackDraw(Screens target);

void uiStackClear();

UIScreen *uiStackGetScreen(const char *name, Screens screen);
UIScreen *uiStackGetScreenFromPair(UIScreenPair *pair, Screens screen);
UIScreen *uiStackGetScreenRelative(UIScreen *screen, Screens which);

FadeStatus uiStackGetFadeStatus();
const char *uiStackGetCurrentScreenName();