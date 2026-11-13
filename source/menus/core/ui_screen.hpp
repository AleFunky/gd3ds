#pragma once
#include "menus/core/ui_element.hpp"
#include "text.hpp"
#include "common_setters.hpp"

#define MAX_ELEMENT_PROPERTIES 64

typedef struct UIScreenPair UIScreenPair;

typedef struct {
    const Charset *charset;
    C2D_SpriteSheet *sheet;    
} LabelFont;

enum Fonts {
    FONT_PUSAB,
    FONT_CHAT,
    FONT_GOLD_PUSAB,

    NUM_FONTS
};

typedef enum {
    ANIM_NONE,
    ANIM_ZOOM,
    ANIM_ZOOM_SUBTLE,
    ANIM_BOUNCE_DOWN,
    ANIM_BOUNCE_DOWN_SLOW,
    ANIM_SLIDE_RIGHT,
    ANIM_SLIDE_DOWN,

    NUM_OPEN_ANIMS
} UIAnimation;

typedef enum {
    UI_TRANSITION_NONE,
    UI_TRANSITION_OPENING,
    UI_TRANSITION_CLOSING
} UITransitionState;

typedef enum {
    UI_DRAW_BEFORE,
    UI_DRAW_AFTER
} UIDrawPhase;

typedef struct {
    UIAnimation animation;
    UITransitionState state;

    float time;
    float duration;
    //determined based on the animation specified (only really applies to the slide down anim for out_duration = 0.5)
    float inDuration;
    float outDuration;
    //amount of time it takes the (fullscreen) darken to fade in/out relative to the screen opening animation
    float darkenFrac;

    bool done;
} UITransition;

typedef struct {
    const char* path;

    void (*init)(UIScreen *);
    void (*update)(UIScreen *, UIInput *);
    void (*draw)(UIScreen *, UIDrawPhase);
    void (*exit)(UIScreen *);

    UIActionList actionList;
} UIScreenDefinition;

typedef struct {
    const char *name;
    UIScreenDefinition top;
    UIScreenDefinition btm;
    void (*freeData)(void *data);
} UIScreenDefPair;

typedef struct UIScreen {
    UIScreenPair *pair;
    const UIScreenDefinition *def;

    UIElement **elements;
    size_t count;
    size_t capacity;

    UITransition transition;
    bool isBottom:1;
    bool disableElementUpdate:1;

    bool loaded:1;
    bool closing:1;
} UIScreen;

typedef void (*UIElementVisitor)(UIElement *element, void *userdata);
typedef bool (*UIElementPredicate)(UIElement *, void *);
typedef UIElement *(*UICreateFn)(UIScreen *screen, const UIPropertyList *);

extern C2D_SpriteSheet uiSheet;
extern C2D_SpriteSheet ui2Sheet;
extern C2D_SpriteSheet bigFontSheet;
extern C2D_SpriteSheet chatFontSheet;
extern C2D_SpriteSheet goldFontSheet;
extern C2D_SpriteSheet windowSheet;
extern C2D_SpriteSheet bgGradientSheet;
extern C2D_SpriteSheet barSheet;

extern UIScreen defaultScreen;
extern UIScreen defaultScreenTop;

extern const LabelFont fonts[NUM_FONTS];

extern const UIBitfieldEntry keybindTable[22];

void requiredLoadingScreenAssetsInit();
void uiAssetsInit();

C2D_SpriteSheet getSheet(int sheet);

void copyTagArray(UIElement *e, const char *tags);

void finishAnimation(UIScreen *screen);

void uiScreenOpen(UIScreen *screen, UIAnimation animation);
void uiScreenClose(UIScreen *screen);

UIElement *uiGetElementByTag(UIScreen *screen, const char *tag);
void uiRunFuncOnTag(UIScreen *screen, const char *tag, void (*func)(UIElement *e));

void uiSetPosOnTag(UIScreen *screen, float x, float y, const char *tag);

char* nextToken(char** cursor);
void collectProperties(UIPropertyList *props, char *token, char **cursor);

void addUIParticleSystem(ParticleSystem *particle);
void freeUIParticleSystems();

void uiLoadScreen(UIScreen* screen);
void uiLoadScreenOld(UIScreen* screen, const UIActionDef* actions, size_t actionCount, const char* path);
void uiScreenUpdateTransition(UIScreen *screen, float dt);
void uiScreenUpdate(UIScreen* screen, UIInput* touch);
void uiScreenDraw(UIScreen* screen);
void uiUnloadScreen(UIScreen *screen);