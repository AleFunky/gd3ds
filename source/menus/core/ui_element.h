#pragma once
#include <stdbool.h>
#include <3ds.h>
#include <citro2d.h>
#include "utils/gfx.h"
#include "particles/circles.h"
#include <malloc.h>

#define TAGS_PER_ELEMENT 5
#define TAG_LENGTH 32

typedef struct {
    float x;
    float y;
    float scaleX;
    float scaleY;
} UITransform;

typedef enum {
    UI_BUTTON,
    UI_IMAGE,
    UI_LABEL,
    UI_CHECKBOX,
    UI_WINDOW,
    UI_TEXTBOX,
    UI_LIST,
    UI_ACTION_AREA,
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


typedef struct {
    touchPosition touchPosition;
    bool did_something;
    bool interacted;
} UIInput;

typedef struct UIElement UIElement;
typedef struct UIScreen UIScreen;

typedef void (*UIActionFn)(UIElement* e);

struct UIElement {
    UIElementType type;

    float x, y;
    int w, h;

    float scaleX, scaleY;

    float opacity;

    bool enabled;

    bool draws_children;

    UIActionFn action;

    UIScreen *screen;

    // Useful for storing data inside an element
    void *userdata;
    void (*userdata_destroy)(void *);

    struct UIElement *parent;

    struct UIElement *first_child;
    struct UIElement *last_child;

    struct UIElement *next_sibling;
    struct UIElement *prev_sibling;

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH];

    void (*update)(UIElement*, UIInput*, UITransform *);
    void (*draw)(UIElement*, UITransform *);
    void (*destroy)(UIElement*);

    void (*modify_transform)(UIElement *, UITransform *);
    
    void (*on_enable)(UIElement*);
    void (*on_disable)(UIElement*);
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

typedef struct {
    UIElement base;

    ImageData image;

    bool hovered;
    bool pressed;

    float hoverTimer;
    float hoverScale;
    float hoverFactor;

    int font;
    char text[64];

    UIActionFn pre_action;

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
    int character_limit;
    int border;
} UITextbox;

typedef struct {
    UIElement base;
    
    char text[512];
    float alignment;

    int font;
    bool parse_tags;
} UILabel;

typedef struct {
    UIElement base;

    bool hovered;
    bool pressed;
} UIActionArea;

typedef struct {
    UIButton base;

    int gamemode;
    int index;

    bool isSelected;
} UIIcon;

typedef struct {
    UIButton base;

    int index;
    int color_index;

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

    float targetOpacity;
    float darkenTime;
    float darkenTimeElapsed;
    bool darkenOver;
    bool fullScreen;
} UIDarken;

typedef struct {
    UIElement base;

    ImageData frame;

    ImageData bar;

    int style;

    float value;
    float max_value;

    bool flip_order;
    bool useTint;
} UIProgressBar;

typedef struct {
    UIElement base;

    int scrollY;
    int contentHeight;
    int lastTouchY;

    bool dragging;

    int dpadHeldTime;

    u32 background_color;
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

    float max_value;
    float value;

    bool dragging;

    C2D_Sprite track_frame;
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
