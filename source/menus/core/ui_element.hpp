#pragma once
#include <stdbool.h>
#include <3ds.h>
#include <citro2d.h>
#include "particles/circles.hpp"
#include <malloc.h>

#include "particles/particles.hpp"
#include "ui_props.hpp"

#define TAGS_PER_ELEMENT 5
#define TAG_LENGTH 32

typedef enum {
    UI_ELEMENT,
    UI_BUTTON,
    UI_IMAGE,
    UI_LABEL,
    UI_CHECKBOX,
    UI_WINDOW,
    UI_TEXTBOX,
    UI_LIST,
    UI_DARKEN,
    UI_ICON,
    UI_COLOR_BUTTON,
    UI_WINDOW_BUTTON,
    UI_PROGRESS_BAR,
    UI_EXTERNAL_LEVEL_CARD,
    UI_STATISTIC_CARD,
    UI_PARTICLE,
    UI_USE_EFFECT,
    UI_PALETTE_ICONS,
    UI_SLIDER,
    UI_ONLINE_LEVEL_CARD,
    UI_RECTANGLE,
} UIElementType;

typedef struct UIInput UIInput;
typedef struct UITransform UITransform;
typedef struct UIElement UIElement;
typedef struct UIScreen UIScreen;

typedef void (*UIActionFn)(UIElement *e, const UIPropertyList *args);

typedef struct UIAction {
    UIActionFn action;
    UIPropertyList args;
} UIAction;

typedef struct UIActionDef {
    const char* name;
    UIActionFn fn;
} UIActionDef;

typedef struct {
    const UIActionDef *actions;
    const size_t actionCount;
} UIActionList;

void uiElementSetUserdata(UIElement *element, void *userdata);

bool uiElementBasicBoundCheck(UIElement *e, UIInput *touch, UITransform *transform);

UITransform uiTransformCombine(UITransform *parent, UIElement *e);

void uiUpdateTree(UIElement *e, UIInput *input, UITransform *parent);
void uiDrawTree(UIElement *e, UITransform *parent);
void uiDestroyTree(UIElement *e);

// Premade functions for on "ui_run_func_on_tag"
void uiEnableElement(UIElement *e);
void uiDisableElement(UIElement *e);

void uiElementAddChild(UIElement *parent, UIElement *child);
void uiElementRemove(UIElement *element);

UIElement *uiGetChildByType(UIElement *parent, UIElementType type);

UIActionFn uiFindAction(const UIActionDef* actions, size_t count, const char* name);

void uiElementApplyDefaultProperties(UIElement *e, UIScreen *screen);
void uiElementApplyProperties(UIElement *e, UIScreen *screen, const UIPropertyList *props);

void performActions(UIElement *e);

typedef struct UITransform {
    float x;
    float y;
    float scaleX;
    float scaleY;
} UITransform;

typedef struct UIInput {
    u32 down;
    u32 held;
    u32 up;
    touchPosition touchPos;
    circlePosition cpad;
    bool interacted;
    //used for lists
    bool dragging;
} UIInput;

struct UIElement {
    UIElementType type = UI_ELEMENT;

    float x = 0.0f, y = 0.0f;
    int w = 0, h = 0;

    float scaleX = 0.0f, scaleY = 1.0f;

    float opacity = 1.0f;

    bool enabled = true;

    bool drawsChildren = false;

    //array of actions to call
    UIAction *actions = nullptr;
    size_t actionCount = 0;

    //old version (not as cool)
    UIActionFn action = nullptr;

    UIScreen *screen = nullptr;

    UIPropertyList customProperties = UIPropertyList{};

    // Useful for storing data inside an element
    void *userdata = nullptr;
    void (*userdataDestroy)(void *) = nullptr;
    
    struct UIElement *parent = nullptr;

    struct UIElement *firstChild = nullptr;
    struct UIElement *lastChild = nullptr;

    struct UIElement *nextSibling = nullptr;
    struct UIElement *prevSibling = nullptr;

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH] = {};

    void (*update)(UIElement*, UIInput*, UITransform *) = nullptr;
    void (*draw)(UIElement*, UITransform *) = nullptr;
    void (*destroy)(UIElement*) = nullptr;

    void (*modifyTransform)(UIElement *, UITransform *) = nullptr;
    
    void (*onEnable)(UIElement*) = nullptr;
    void (*onDisable)(UIElement*) = nullptr;
};

typedef struct {
    C2D_Sprite sprite;
    C2D_ImageTint tint;
} ImageData;

typedef struct {
    UIElement base;

    ImageData image;

    bool useTint;
} UIImage;

typedef enum ButtonAnimType {
    BUTTON_ANIM_NORMAL,
    BUTTON_ANIM_PULL,
    //uninmplemented lel
    BUTTON_ANIM_DARKEN
} ButtonAnimType;

typedef struct {
    UIElement base;

    ImageData image;

    bool hovered;
    bool pressed;

    ButtonAnimType animType;
    float hoverTimer;
    float hoverProgress;
    float hoverFactor;

    int font;
    char text[64];

    UIActionFn preAction;

    float textScale;

    u32 keyBinds;
    int keyPressTimer;

    bool invisible;
} UIButton;

typedef struct {
    UIButton  base;

    bool checked;
} UICheckBox;

typedef struct {
    UIElement base;
    
    C2D_Image atlas;

    u32 color;
    int border;
} UIWindow;

typedef struct {
    UIElement base;

    C2D_Image atlas;

    char title[64];
    char text[128];
    int characterLimit;
    int border;
} UITextbox;

typedef struct {
    UIElement base;
    
    char text[1024];
    float alignment;

    int font;
    bool parseTags;
} UILabel;

typedef struct {
    UIButton base;

    int gamemode;
    int index;
    u32 p1Color;
    u32 p2Color;
    u32 glowColor;

    bool isSelected;
    bool glow;
} UIIcon;

typedef struct {
    UIButton base;

    int index;
    int colorIndex;

    ImageData image;

    bool isSelected;
} UIColor;

typedef struct {
    UIButton base;

    C2D_Image atlas;
    u32 color;
    int border;
} UIWindowButton;

typedef struct {
    UIElement base;
    
    ImageData image;

    float opacity;
    bool fullScreen;
} UIDarken;

typedef struct {
    UIElement base;

    ImageData frame;

    ImageData bar;

    int style;

    float value;
    float maxValue;

    bool flipOrder;
    bool useTint;
} UIProgressBar;

typedef struct {
    UIElement base;

    int scrollY;
    int contentHeight;
    int lastTouchY;

    bool dragging;

    int dpadHeldTime;

    u32 backgroundColor;
} UIList;

typedef struct {
    UIElement base;
    
    ParticleSystem particle;
} UIParticle;

typedef struct {
    UIElement base;

    UseEffectPool useEffects;
    //Use effect original positions relative to (0, 0)
    float xPos[MAX_USE_EFFECTS];
    float yPos[MAX_USE_EFFECTS];
} UIUseEffect;

typedef struct {
    UIElement base;

    float maxValue;
    float value;

    bool dragging;

    C2D_Sprite trackFrame;
    C2D_Sprite track;
    C2D_Sprite button;
} UISlider;

typedef struct {
    UIElement base;

    float spacing;
} UIPaletteIcons;

typedef struct {
    UIElement base;

    u32 color;
} UIRectangle;

typedef struct {
    UIImage base;

    float rotationSpeed;
    bool blending;
} UISpinner;
/*
typedef struct {
    
    UIImageData icon;
    UILabelData label;
    UIButtonData button;
    float button_w;
    float button_h;
    char path[256];
    bool swap_color;
} UIExternalLevelCardData;

typedef struct {
    UILabelData stat_name;
    int value;
    bool swap_color;
} UIStatisticCardData;

typedef struct {
    UILabelData name;
    UILabelData creator;
    UILabelData song;
    UILabelData length;
    int downloads;
    int likes;
    int stars;
    bool swap_color;
    UIWindowButtonData windowbutton;
    float windowbutton_w;
    float windowbutton_h;
    UIImageData difficulty;
    UIImageData stars_icon;
    UIImageData downloads_icon;
    UIImageData likes_icon;
    UIImageData length_icon;
} UIOnlineLevelCardData;
*/
