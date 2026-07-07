#pragma once
#include <stdbool.h>
#include <3ds.h>
#include <citro2d.h>
#include "utils/gfx.h"
#include "particles/circles.h"

#define TAGS_PER_ELEMENT 5
#define TAG_LENGTH 32

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
    UI_PALLETE_ICONS,
    UI_SLIDER,
    UI_ONLINE_LEVEL_CARD,
} UIElementType;

typedef struct {
    C2D_Sprite sprite;
    float scaleX;
    float scaleY;

    C2D_ImageTint tint;
    bool useTint;
} UIImageData;

typedef struct {
    UIImageData image;

    bool hovered;
    bool pressed;
    
    float hoverTimer;
    float hoverScale;

    float scaleX;
    float scaleY;
    int font;
    char text[64];

    float textScale;

    u32 keyBinds;
    int keyPressTimer;
} UIButtonData;

typedef struct {
    UIImageData image;

    bool checked;
    u8 image_id;
    
    float scaleX;
    float scaleY;

    bool hovered;
    bool pressed;

    float hoverTimer;
    float hoverScale;
} UICheckBoxData;

typedef struct {
    C2D_Image atlas;
    u32 color;
    int border;
} UIWindowData;

typedef struct {
    C2D_Image atlas;

    char title[64];
    char text[128];
    int character_limit;
    int border;
} UITextbox;

typedef struct {
    char text[512];
    float alignment;
    float scale;
    int font;
    bool parse_tags;
} UILabelData;

typedef struct {
    bool hovered;
    bool pressed;
} UIActionAreaData;

typedef struct {
    touchPosition touchPosition;
    bool did_something;
    bool interacted;
} UIInput;

typedef struct {
    int gamemode;
    int index;

    UIImageData image;

    bool hovered;
    bool pressed;
    
    float hoverTimer;
    float hoverScale;

    float scaleX;
    float scaleY;

    bool isSelected;
} UIIconData;

typedef struct {
    int index;
    int color_index;

    UIImageData image;
    UIImageData button;

    bool hovered;
    bool pressed;
    
    float hoverTimer;
    float hoverScale;

    float scaleX;
    float scaleY;

    bool isSelected;
} UIColorData;

typedef struct {
    UIWindowData window;

    bool hovered;
    bool pressed;
    
    float hoverTimer;
    float hoverScale;

    float scaleX;
    float scaleY;
    int font;
    char text[64];

    float textScale;
    
    u32 keyBinds;
    int keyPressTimer;
} UIWindowButtonData;

typedef struct {
    C2D_Sprite sprite;
    C2D_ImageTint tint;
    float targetOpacity;
    float darkenTime;
    float darkenTimeElapsed;
    bool darkenOver;
    bool fullScreen;
} UIDarken;

typedef struct {
    C2D_Sprite sprite_frame;
    C2D_ImageTint tint_frame;

    C2D_Sprite sprite;
    C2D_ImageTint tint;

    float scale;
    float fill_scaleX;
    float fill_scaleY;
    int style;

    float value;
    float max_value;

    bool flip_order;
    bool useTint;
} UIProgressBarData;

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

typedef struct UIElement UIElement;

#define UI_LIST_MAX_ITEMS 256

typedef struct {
    UIElement* items[UI_LIST_MAX_ITEMS];
    int itemCount;

    int scrollY;
    int contentHeight;
    int lastTouchY;

    bool dragging;

    int dpadHeldTime;
} UIList;

typedef struct {
    ParticleSystem particle;
} UIParticleData;

typedef struct {
    UseEffectPool useEffects;
    //Use effect original positions relative to (0, 0)
    float xPos[MAX_USE_EFFECTS];
    float yPos[MAX_USE_EFFECTS];
} UIUseEffectData;

typedef struct {
    float max_value;
    float value;

    float scale;

    bool dragging;

    C2D_Sprite track_frame;
    C2D_Sprite track;
    C2D_Sprite button;
} UISliderData;

typedef void (*UIActionFn)(UIElement* e);

struct UIElement {
    UIElementType type;

    float x, y;
    int w, h;

    float opacity;

    bool enabled;

    UIActionFn action;

    union {
        UIImageData image;
        UIButtonData button;
        UILabelData label;
        UICheckBoxData checkbox;
        UIWindowData window;
        UITextbox textbox;
        UIList list;
        UIActionAreaData action_area;
        UIDarken darken;
        UIIconData icon;
        UIColorData color;
        UIWindowButtonData window_button;
        UIProgressBarData progress_bar;
        UIExternalLevelCardData external_level_card;
        UIStatisticCardData statistic_card;
        UIOnlineLevelCardData online_level_card; 
        UIParticleData particle;
        UIUseEffectData use_effect;
        UISliderData slider;
    };

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH];

    void (*update)(UIElement*, UIInput*);
    void (*draw)(UIElement*);
};