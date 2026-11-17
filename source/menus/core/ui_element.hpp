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

namespace gd3ds
{

enum class UIElementType {
    UiElement,
    UiButton,
    UiImage,
    UiLabel,
    UiCheckbox,
    UiWindow,
    UiTextbox,
    UiList,
    UiDarken,
    UiIcon,
    UiColorButton,
    UiWindowButton,
    UiProgressBar,
    UiExternalLevelCard,
    UiStatisticCard,
    UiParticle,
    UiUseEffect,
    UiPaletteIcons,
    UiSlider,
    UiOnlineLevelCard,
    UiRectangle,
};

struct UIInput;
struct UITransform;
struct UIElement;
struct UIScreen;

typedef void (*UIActionFn)(UIElement *e, const UIPropertyList *args);

struct UIAction {
    UIActionFn action;
    UIPropertyList args;
};

struct UIActionDef {
    const char* name;
    UIActionFn fn;
};

struct UIActionList {
    const UIActionDef *actions;
    const size_t actionCount;
};

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

struct UITransform {
    float x;
    float y;
    float scaleX;
    float scaleY;
};

struct UIInput {
    u32 down;
    u32 held;
    u32 up;
    touchPosition touchPos;
    circlePosition cpad;
    bool interacted;
    //used for lists
    bool dragging;
};

struct UIElement {
    UIElementType type = UIElementType::UiElement;

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

struct ImageData {
    C2D_Sprite sprite;
    C2D_ImageTint tint;
};

struct UIImage {
    UIElement base;

    ImageData image;

    bool useTint;
};

enum class ButtonAnimType {
    ButtonAnimNormal,
    ButtonAnimPull,
    //uninmplemented lel
    ButtonAnimDarken
};

struct UIButton {
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
};

struct UICheckBox {
    UIButton  base;

    bool checked;
};

struct UIWindow {
    UIElement base;
    
    C2D_Image atlas;

    u32 color;
    int border;
};

struct UITextbox {
    UIElement base;

    C2D_Image atlas;

    char title[64];
    char text[128];
    int characterLimit;
    int border;
};

struct UILabel {
    UIElement base;
    
    char text[1024];
    float alignment;

    int font;
    bool parseTags;
};

struct UIIcon {
    UIButton base;

    int gamemode;
    int index;
    u32 p1Color;
    u32 p2Color;
    u32 glowColor;

    bool isSelected;
    bool glow;
};

struct UIColor {
    UIButton base;

    int index;
    int colorIndex;

    ImageData image;

    bool isSelected;
};

struct UIWindowButton {
    UIButton base;

    C2D_Image atlas;
    u32 color;
    int border;
};

struct UIDarken {
    UIElement base;
    
    ImageData image;

    float opacity;
    bool fullScreen;
};

struct UIProgressBar {
    UIElement base;

    ImageData frame;

    ImageData bar;

    int style;

    float value;
    float maxValue;

    bool flipOrder;
    bool useTint;
};

struct UIList {
    UIElement base;

    int scrollY;
    int contentHeight;
    int lastTouchY;

    bool dragging;

    int dpadHeldTime;

    u32 backgroundColor;
};

struct UIParticle {
    UIElement base;
    
    ParticleSystem particle;
};

struct UIUseEffect {
    UIElement base;

    UseEffectPool useEffects;
    //Use effect original positions relative to (0, 0)
    float xPos[MAX_USE_EFFECTS];
    float yPos[MAX_USE_EFFECTS];
};

struct UISlider {
    UIElement base;

    float maxValue;
    float value;

    bool dragging;

    C2D_Sprite trackFrame;
    C2D_Sprite track;
    C2D_Sprite button;
};

struct UIPaletteIcons {
    UIElement base;

    float spacing;
};

struct UIRectangle {
    UIElement base;

    u32 color;
};

struct UISpinner {
    UIImage base;

    float rotationSpeed;
    bool blending;
};
/*
struct UIExternalLevelCardData {
    
    UIImageData icon;
    UILabelData label;
    UIButtonData button;
    float button_w;
    float button_h;
    char path[256];
    bool swap_color;
};

struct UIStatisticCardData {
    UILabelData stat_name;
    int value;
    bool swap_color;
};

struct UIOnlineLevelCardData {
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
};
*/

}