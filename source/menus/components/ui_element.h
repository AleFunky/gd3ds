#pragma once
#include <stdbool.h>
#include <3ds.h>
#include <citro2d.h>

#define TAGS_PER_ELEMENT 5
#define TAG_LENGTH 16

typedef enum {
    UI_BUTTON,
    UI_IMAGE,
    UI_LABEL,
    UI_CHECKBOX,
    UI_WINDOW,
    UI_TEXTBOX
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

    char text[64];
} UIButtonData;

typedef struct {
    UIImageData image;

    bool checked;
    bool hovered;
    bool pressed;

    float hoverTimer;
    float hoverScale;
} UICheckBoxData;

typedef struct {
    C2D_Image atlas;
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
    char text[256];
    float alignment;
    float scale;
} UILabelData;

typedef struct {
    touchPosition touchPosition;
    bool did_something;
} UIInput;

typedef struct UIElement UIElement;

typedef void (*UIActionFn)(void* userdata);

struct UIElement {
    UIElementType type;

    int x, y;
    int w, h;

    bool enabled;

    UIActionFn action;
    void* action_data;

    union {
        UIImageData image;
        UIButtonData button;
        UILabelData label;
        UICheckBoxData checkbox;
        UIWindowData window;
        UITextbox textbox;
    };

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH];

    void (*update)(UIElement*, UIInput*);
    void (*draw)(UIElement*);
};