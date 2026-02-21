#pragma once
#include <stdbool.h>
#include <3ds.h>
#include <citro2d.h>

typedef enum {
    UI_BUTTON,
    UI_IMAGE,
    UI_LABEL,
    UI_CHECKBOX
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
    char text[256];
    float alignment;
    float scale;
} UILabelData;

typedef struct UIElement UIElement;

typedef void (*UIActionFn)(void* userdata);

struct UIElement {
    UIElementType type;

    int x, y;
    int w, h;

    bool visible;
    bool enabled;

    UIActionFn action;
    void* action_data;

    union {
        UIImageData image;
        UIButtonData button;
        UILabelData label;
        UICheckBoxData checkbox;
    };

    char tag[16];

    void (*update)(UIElement*, touchPosition*);
    void (*draw)(UIElement*);
};