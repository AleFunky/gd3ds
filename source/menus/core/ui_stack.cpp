#include "ui_stack.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_element.hpp"
#include "menus/settings_hub/settings.hpp"
#include "ui_screen.hpp"
#include "screen_definitions.hpp"
#include "fonts/bigFont.hpp"
#include "utils/gfx.hpp"
#include <string.h>

static UIStack *stack = NULL;

void uiStackSetStack(UIStack *setStack){
    stack = setStack;
}

static size_t uiStackMinIndex() {
    return stack->currentRoot;
}

static size_t uiStackMaxIndex() {
    if(stack->activeCount == 0){
        return 0;
    }
    return stack->currentRoot + stack->activeCount - 1;
}

static size_t uiStackNextIndex() {
    return stack->currentRoot + stack->activeCount;
}

static void unloadScreenpair(int index){
    UIScreenPair *pair = stack->screenStack[index];

    if(!pair) return;

    if(pair->freeData && pair->data){
        pair->freeData(pair->data);
    }
    pair->freeData = NULL;
    pair->data = NULL;

    for(int j = 0; j < 2; j++){
        UIScreen *screen = &pair->screens[j];
        uiUnloadScreen(screen);
    }

    free(pair);
    stack->screenStack[index] = NULL;
}

static void openRoot(){
    //unload all active layers below new root
    for(size_t i = uiStackMinIndex(); i <= uiStackMaxIndex(); i++){
        //basic unload of previous screens (do not unload entire screenpair)
        UIScreenPair *pair = stack->screenStack[i];
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &pair->screens[j];
            uiUnloadScreen(screen);
        }
    }

    size_t index = uiStackNextIndex();

    stack->currentRoot = index;
    stack->activeCount = 1;
}

static void closeRoot(){
    if(stack->currentRoot == 0) return;

    //unload all active layers above root
    for(size_t i = uiStackMinIndex(); i <= uiStackMaxIndex(); i++){
        unloadScreenpair(i);
    }

    //find previous root
    size_t newRoot = 0;
    for(size_t i = stack->currentRoot; i-- > 0; ){
        if(stack->screenStack[i]->root){
            newRoot = i;
            break;
        }
    }

    for(size_t i = newRoot; i < stack->currentRoot; i++){
        UIScreenPair *pair = stack->screenStack[i];
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &pair->screens[j];
            uiLoadScreen(screen);
            finishAnimation(screen);
        }
    }

    stack->activeCount = stack->currentRoot - newRoot;

    stack->currentRoot = newRoot;
}

static bool resizeStack(size_t newCapacity){
    if(newCapacity > 32){
        return false;
    }

    size_t capacity = stack->stackCapacity;

    if(newCapacity <= capacity){
        return true;
    }

    while(capacity < newCapacity){
        capacity += 4;
    }

    UIScreenPair **temp = (UIScreenPair **)realloc(stack->screenStack, sizeof(UIScreenPair *) * capacity);

    if(!temp){
        return false;
    }

    memset(temp + stack->stackCapacity, 0, sizeof(UIScreenPair *) * (capacity - stack->stackCapacity));

    stack->screenStack = temp;
    stack->stackCapacity = capacity;

    return true;
}

//create new screenpair and expand stack if needed
static UIScreenPair *uiStackOpen(const UIScreenDefPair *defs){
    if(!defs){
        return NULL;
    }

    int index = uiStackNextIndex();

    if(!resizeStack(index + 1)){
        return NULL;
    }

    UIScreenPair *pair = (UIScreenPair *)calloc(1, sizeof(UIScreenPair));

    if(!pair) return NULL;

    stack->screenStack[index] = pair;

    pair->name = defs->name;

    UIScreen *top = &pair->screens[SCREEN_TOP];
    UIScreen *btm = &pair->screens[SCREEN_BTM];

    *top = (UIScreen){ 0 };
    *btm = (UIScreen){ 0 };

    btm->isBottom = true;

    top->def = &defs->top;
    btm->def = &defs->btm;

    top->pair = pair;
    btm->pair = pair;

    return pair;
}

void uiStackPush(
    const UIScreenDefPair* defs, 
    UIAnimation topAnim, 
    UIAnimation btmAnim,
    UIStackPushType type
){
    //neither of these types actually push a screen
    if(type == PUSH_NONE || type == PUSH_GAME_STATE){
        return;
    }

    if(stack->push.type != PUSH_NONE){
        outputLog("Cannot queue multiple pushes!\n");
        return;
    }

    stack->push.defs = defs;
    stack->push.topAnim = topAnim;
    stack->push.btmAnim = btmAnim;
    stack->push.type = type;

    if(type == PUSH_ROOT){
        stack->rootTransition = UI_TRANSITION_OPENING;
        stack->fade = FADE_STATUS_IN;
        stack->fadeTime = 0.f;
    }
}

void uiStackPushName(
    const char* name, 
    UIAnimation topAnim, 
    UIAnimation btmAnim,
    UIStackPushType type
){
    const UIScreenDefPair *defs = uiGetScreenDef(name);

    if(!defs){
        return;
    }

    uiStackPush(defs, topAnim, btmAnim, type);
}

void uiStackPushData(void *data){
    //can't push data when there is no actual push
    if(stack->push.type == PUSH_NONE) return;


    stack->push.data = data;
}

void uiStackPushRootInstant(const UIScreenDefPair *defs){
    UIScreenPair *pair = uiStackOpen(defs);

    if (!pair) {
        return;
    }

    pair->root = true;

    openRoot();

    UIScreen *top = &pair->screens[SCREEN_TOP];
    UIScreen *btm = &pair->screens[SCREEN_BTM];

    uiLoadScreen(top);
    uiLoadScreen(btm);

    finishAnimation(top);
    finishAnimation(btm);
}

void uiStackPushGameState(int gameState){
    stack->nextGameState = gameState;
    stack->push.type = PUSH_GAME_STATE;
    stack->rootTransition = UI_TRANSITION_OPENING;
    stack->fade = FADE_STATUS_IN;
    stack->fadeTime = 0.f;
}

void uiStackPop(){
    if(stack->rootTransition != UI_TRANSITION_NONE) {
        outputLog("Cannot pop stack during transition!\n");
        return;
    }

    if(stack->activeCount == 0) return;

    UIScreenPair *pair = stack->screenStack[uiStackMaxIndex()];
    if(pair->root){
       stack->rootTransition = UI_TRANSITION_CLOSING;
       stack->fade = FADE_STATUS_IN;
       stack->fadeTime = 0.f;
    } else{
        for(int i = 0; i < 2; i++){
            uiScreenClose(&pair->screens[i]);
        }
    }
}

//pops all active screens
void uiStackPopContext(){
    if(stack->rootTransition != UI_TRANSITION_NONE) {
        outputLog("Cannot pop stack during transition!\n");
        return;
    }

    for(size_t i = uiStackMinIndex() + 1; i < uiStackMaxIndex(); i++){
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &stack->screenStack[i]->screens[j];
            uiScreenClose(screen);
        }
    }

    stack->rootTransition = UI_TRANSITION_CLOSING;
    stack->fade = FADE_STATUS_IN;
    stack->fadeTime = 0.f;
}

static void updateOpening(){
    UIStackPush *push = &stack->push;

    if(push->type == PUSH_NEXT){
        push->pushNow = true;
    }

    //do push
    if(push->pushNow && push->type != PUSH_NONE){
        push->pushNow = false;

        UIScreenPair *pair = uiStackOpen(push->defs);

        if(!pair){
            push->type = PUSH_NONE;
            return;
        }

        pair->root = (push->type == PUSH_ROOT);
        if(push->defs->freeData){
            pair->freeData = push->defs->freeData;
        }
        if(push->data){
            pair->data = push->data;
        }
        push->data = NULL;

        push->type = PUSH_NONE;

        UIScreen *top = &pair->screens[SCREEN_TOP];
        UIScreen *btm = &pair->screens[SCREEN_BTM];

        if(pair->root){
            openRoot();
        } else{
            stack->activeCount++;
        }

        uiLoadScreen(top);
        uiLoadScreen(btm);

        uiScreenOpen(top, push->topAnim);
        uiScreenOpen(btm, push->btmAnim);

        if (settingsState.fastMenu) {
            finishAnimation(top);
            finishAnimation(btm);
        }
    }
}

static void updateClosing(){
    UIScreenPair *pair = stack->screenStack[uiStackMaxIndex()];

    if(!pair) return;

    if(!pair->root){
        UIScreen *top = &pair->screens[SCREEN_TOP];
        UIScreen *btm = &pair->screens[SCREEN_BTM];

        bool topClosing = top->loaded ? top->closing : true;
        bool btmClosing = btm->loaded ? btm->closing : true;

        if(stack->rootTransition == UI_TRANSITION_NONE && topClosing && btmClosing){
            unloadScreenpair(uiStackMaxIndex());

            stack->activeCount--;

            if(stack->push.type == PUSH_AFTER_CLOSE){
                stack->push.pushNow = true;
            }
        }
    }
}

static void handleStackFading(){
    if (settingsState.fastMenu) stack->fadeTime = 255.f;

    //root transition open/close slop
    //instant if the active count is 0 (main menu doesn't fade in)
    bool transitionSwitch = false;
    if(stack->fadeTime >= 255.f || stack->activeCount == 0){
        switch(stack->fade){
            case FADE_STATUS_IN:
                stack->fadeTime = 0.f;
                stack->fade = FADE_STATUS_OUT;
                transitionSwitch = true;
                break;
            case FADE_STATUS_OUT:
                stack->fadeTime = 0.f;
                stack->fade = FADE_STATUS_NONE;
                stack->rootTransition = UI_TRANSITION_NONE;
                break;
            case FADE_STATUS_NONE:
                break;
        }
    }

    //when the fade is fully black, either go back to the previous root or open a new one depending on transition
    if(transitionSwitch) {
        if(stack->push.type == PUSH_GAME_STATE){
            gameState = stack->nextGameState;
            escapeState = true;
            stack->fadeTime = 0.f;
            stack->fade = FADE_STATUS_OUT;
            stack->push.type = PUSH_NONE;
        } else{
            switch(stack->rootTransition){
                case UI_TRANSITION_OPENING:
                    if(stack->push.type == PUSH_ROOT) stack->push.pushNow = true;
                    stack->fadeTime = 0.f;
                    stack->fade = FADE_STATUS_OUT;
                    break;
                case UI_TRANSITION_CLOSING:
                    closeRoot();
                    stack->fadeTime = 0.f;
                    stack->fade = FADE_STATUS_OUT;
                    break;
                case UI_TRANSITION_NONE:
                    break;
            }
        }
        fixedDt = true;
    } else{
        //make sure there's at least a frame of black
        stack->fadeTime += FADE_SPEED * DT;
    }
}

void uiStackUpdate(UIInput *input){
    handleStackFading();

    updateOpening();

    if(stack->activeCount > 0){
        for(size_t i = uiStackMinIndex(); i <= uiStackMaxIndex(); i++){
            UIScreenPair *pair = stack->screenStack[i];

            if(!pair) continue;

            bool updatingTopmost = i == uiStackMaxIndex() && stack->rootTransition == UI_TRANSITION_NONE;
        
            for(int j = 0; j < 2; j++) {
                UIScreen *screen = &pair->screens[j];

                UIInput dummyInput = { 0 };
                //only update top screen with input
                UIInput *inputToUse = updatingTopmost && screen->transition.done && j == SCREEN_BTM ? input : &dummyInput;

                uiScreenUpdate(screen, inputToUse);
            }
        }

        updateClosing();
    }
}

bool uiStackCheckLoadedRoot(const UIScreenDefPair *screen) {
    if (!screen) return false; 

    UIScreenPair *pair = stack->screenStack[stack->currentRoot];

    if (strcmp(screen->name, pair->name) == 0) {
        return true;
    }
    return false;
}

bool uiStackCheckLoadedScreenInRoot(const UIScreenDefPair *screen) {
    if (!screen) return false; 
    
    for (size_t i = uiStackMinIndex() + 1; i <= uiStackMaxIndex(); i++) {
        UIScreenPair *pair = stack->screenStack[i];
        if (strcmp(screen->name, pair->name) == 0) {
            return true;
        }
    }
    return false;
}

UIScreenPair *uiStackGetLoadedScreen(const UIScreenDefPair *screen) {
    if (!screen) return NULL; 

    for (size_t i = 0; i <= uiStackMaxIndex(); i++) {
        UIScreenPair *pair = stack->screenStack[i];
        if (strcmp(screen->name, pair->name) == 0) {
            return pair;
        }
    }
    return NULL;
}

void drawStackDebug(){
    char debug[2048];
    size_t pos = 0;

    for (size_t i = 0; i <= uiStackMaxIndex(); i++) {
        UIScreenPair *pair = stack->screenStack[i];

        if(!pair) continue;

        int r = 0;
        if(pair->root){
            r = 128;
        }
        if(i == stack->currentRoot){
            r = 255;
        }
        int gb = 128;
        if(i > uiStackMinIndex() && i <= uiStackMaxIndex()){
            gb = 255;
        }

        pos += snprintf(
            debug + pos,
            sizeof(debug) - pos,
            "<%d,%d,%d>%s</>\n",
            r, gb, gb,
            pair->name ? pair->name : "(unnamed)"
        );

        if (pos >= sizeof(debug)) break;
    }

    char rootSlop[] = "ROOT:....";
    snprintf(rootSlop, sizeof(rootSlop), "ROOT: %d", stack->currentRoot);

    char activeSlop[] = "Active:....";
    snprintf(activeSlop, sizeof(activeSlop), "Active: %d", stack->activeCount);

    char capacitySlop[] = "Capacity:....";
    snprintf(capacitySlop, sizeof(capacitySlop), "Capacity: %d", stack->stackCapacity);

    drawText(&bigFontFontCharset, &bigFontSheet, SCREEN_BOT_WIDTH / 2.f, SCREEN_HEIGHT / 2.f, 0.5f, 0.5f, 0, true, debug);
    drawText(&bigFontFontCharset, &bigFontSheet, 10, 10, 0.5f, 0.5f, 0, true, rootSlop);
    drawText(&bigFontFontCharset, &bigFontSheet, 10, 25, 0.5f, 0.5f, 0, true, activeSlop);
    drawText(&bigFontFontCharset, &bigFontSheet, 10, 40, 0.5f, 0.5f, 0, true, capacitySlop);
}

void drawStackFade(){
    if (settingsState.fastMenu) return;

    float fade = 0;
    if(stack->fade == FADE_STATUS_IN){
        fade = CLAMP(stack->fadeTime, 0, 255);
    } else if(stack->fade == FADE_STATUS_OUT){
        fade = CLAMP(255 - stack->fadeTime, 0, 255);
    }

    C2D_DrawRectSolid(0.f, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, fade));
}

void uiStackDraw(Screens target){
    if(stack->activeCount == 0) return;

    for(size_t i = uiStackMinIndex(); i <= uiStackMaxIndex(); i++){
        UIScreenPair *pair = stack->screenStack[i];

        if(!pair) continue;

        UIScreen *screen = &pair->screens[target];
        if(screen->loaded){
            uiScreenDraw(screen);
        }
    }
}

void uiStackClear(){
    for(size_t i = 0; i < stack->stackCapacity; i++){
        unloadScreenpair(i);
    }

    free(stack->screenStack);
    *stack = (UIStack){ 0 };
}

UIScreen *uiStackGetScreen(const char *name, Screens screen) {
    UIScreenPair *pair = NULL;

    for(size_t i = uiStackMinIndex(); i <= uiStackMaxIndex(); i++){
        UIScreenPair *pair1 = stack->screenStack[i];

        if(!pair1) continue;

        if(strcmp(pair1->name, name) == 0){
            pair = pair1;
        }
    }

    if(!pair) return NULL;
    
    return &pair->screens[screen];
}

FadeStatus uiStackGetFadeStatus(){
    return stack->fade;
}

const char *uiStackGetCurrentScreenName() {
    return stack->screenStack[uiStackMaxIndex()]->name;
}